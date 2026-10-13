#include "support.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <set>
#include <thread>

// ============================================================================
// 命中判断（无分支：用 & 而不是 &&）
// ============================================================================
static inline bool hit_of(const Record& r, const Query& q) {
    return (q.begin <= r.timestamp) & (r.timestamp < q.end) &
           (r.latency_us >= q.min_latency_us) &
           ((r.flags & q.required_flags) == q.required_flags);
}

// ============================================================================
// 候选 B：多线程 + 两遍扫描 + 排他前缀和
//
// 稳定性保证：
//   1) 输入按连续块切分，块内仍是原顺序；
//   2) 第一遍各线程只【计数】，不写 output；
//   3) 对块计数做排他前缀和，得到每块在 output 中的起始位置；
//   4) 第二遍各线程只写自己那段连续区间，区间互不重叠 → 无写冲突且顺序正确。
//
// 额外成本（都在计时范围内）：输入扫两遍、每次调用创建线程、counts/offsets 分配。
// ============================================================================
static std::size_t filter_records(const Record* input, std::size_t count,
                                  Record* output, const Query& query) {
    if (count == 0) return 0;

    unsigned threads = std::thread::hardware_concurrency();
    if (threads == 0) threads = 1;
    if (threads > 8) threads = 8;
    if (static_cast<std::size_t>(threads) > count) threads = static_cast<unsigned>(count);

    const std::size_t chunk = (count + threads - 1) / threads;
    std::vector<std::size_t> counts(threads, 0);
    std::vector<std::size_t> offsets(threads, 0);

    // ---- 第一遍：各线程数自己块内的命中数 ----
    {
        std::vector<std::thread> pool;
        pool.reserve(threads);
        for (unsigned t = 0; t < threads; ++t) {
            pool.emplace_back([&, t]() {
                const std::size_t lo = static_cast<std::size_t>(t) * chunk;
                const std::size_t hi = std::min(lo + chunk, count);
                std::size_t c = 0;
                for (std::size_t i = lo; i < hi; ++i)
                    c += hit_of(input[i], query) ? 1u : 0u;
                counts[t] = c;
            });
        }
        for (auto& th : pool) th.join();
    }

    // ---- 排他前缀和：offsets[t] = 前 t 块的命中总数 = 第 t 块的输出起点 ----
    for (unsigned t = 1; t < threads; ++t) offsets[t] = offsets[t-1] + counts[t-1];
    const std::size_t total = offsets[threads-1] + counts[threads-1];

    // ---- 第二遍：各线程写自己那段（块内保持顺序 → 结果稳定）----
    {
        std::vector<std::thread> pool;
        pool.reserve(threads);
        for (unsigned t = 0; t < threads; ++t) {
            pool.emplace_back([&, t]() {
                const std::size_t lo = static_cast<std::size_t>(t) * chunk;
                const std::size_t hi = std::min(lo + chunk, count);
                std::size_t pos = offsets[t];
                for (std::size_t i = lo; i < hi; ++i) {
                    const Record& r = input[i];
                    // 这里必须用 if：无条件写会越过本块区间，
                    // 撞上下一块线程正在写的第一个位置（数据竞争）。
                    if (hit_of(r, query)) output[pos++] = r;
                }
            });
        }
        for (auto& th : pool) th.join();
    }

    return total;
}

// STARTER CODE: main 仍然负责 setup / execution / validation / reporting。
// 已经把核心筛选抽成上面的 filter_records，其余职责保持不变。
int main(int argc, char** argv) try {
    std::uint64_t count=100000, max_records=10000000;
    std::uint32_t seed=2026;
    unsigned hit_percent=50, repeats=5;
    std::string pattern="random", input_path, output_path, export_path;
    Query query;
    std::set<std::string> seen;
    for (int i=1; i<argc; ++i) {
        const std::string key=argv[i];
        if (key=="--help") {
            std::cout << "log_filter [--count N | --input FILE] [--hit-percent 0..100]\n"
                      << " [--pattern random|blocks] [--seed U32] [--repeats 1..100]\n"
                      << " [--begin U64] [--end U64] [--min-latency U32] [--flags U32]\n"
                      << " [--output FILE] [--export-input FILE] [--max-records N]\n"
                      << "Defaults: count=100000 seed=2026 hit=50 repeats=5 max-records=10000000\n"
                      << "Query: [1000,2000), latency>=100, all bits of flags=3. Decimal arguments.\n";
            return 0;
        }
        if (!seen.insert(key).second || i+1==argc) throw std::runtime_error("duplicate option or missing value");
        const std::string value=argv[++i];
        if (key=="--count") count=number(value);
        else if (key=="--max-records") max_records=number(value);
        else if (key=="--seed") seed=narrow32(number(value));
        else if (key=="--hit-percent") { const auto v=number(value); if(v>100) throw std::runtime_error("invalid hit percent"); hit_percent=static_cast<unsigned>(v); }
        else if (key=="--repeats") { const auto v=number(value); if(v<1 || v>100) throw std::runtime_error("invalid repeats"); repeats=static_cast<unsigned>(v); }
        else if (key=="--begin") query.begin=number(value);
        else if (key=="--end") query.end=number(value);
        else if (key=="--min-latency") query.min_latency_us=narrow32(number(value));
        else if (key=="--flags") query.required_flags=narrow32(number(value));
        else if (key=="--pattern") pattern=value;
        else if (key=="--input") input_path=value;
        else if (key=="--output") output_path=value;
        else if (key=="--export-input") export_path=value;
        else throw std::runtime_error("unknown option: "+key);
    }
    if (query.begin>query.end || (pattern!="random" && pattern!="blocks")) throw std::runtime_error("invalid query or pattern");
    if (seen.count("--input") && input_path.empty()) throw std::runtime_error("empty input path");
    if ((seen.count("--output") && output_path.empty()) || (seen.count("--export-input") && export_path.empty())) throw std::runtime_error("empty output path");
    if (!input_path.empty() && (seen.count("--count") || seen.count("--seed") || seen.count("--hit-percent") || seen.count("--pattern") || seen.count("--export-input")))
        throw std::runtime_error("input file cannot be combined with generator options");
    if (input_path.empty() && (count>max_records || count>std::numeric_limits<std::size_t>::max()/sizeof(Record)))
        throw std::runtime_error("generated count exceeds limit");
    // Generation and file decoding are deliberately outside core timing.
    auto input=input_path.empty() ? generate(static_cast<std::size_t>(count),hit_percent,pattern,seed) : load(input_path,max_records);
    if (!export_path.empty()) save(export_path,input,input.size());
    auto expected=reference_filter(input,query);
    std::vector<Record> output(input.size());
    std::vector<double> times;
    std::size_t matched=0;
    std::cout << std::setprecision(10)
              << "implementation=baseline threads=1 generator=v1 source=" << (input_path.empty()?"generated":"file")
              << " count=" << input.size() << " sizeof_record=" << sizeof(Record)
              << " seed=" << seed << " pattern=" << pattern << " requested_hit_percent=" << hit_percent
              << " begin=" << query.begin << " end=" << query.end << " min_latency=" << query.min_latency_us
              << " flags=" << query.required_flags << " warmup=1 repeats=" << repeats << '\n';
    for (int run=-1; run<static_cast<int>(repeats); ++run) {
        matched=0;
        const auto start=std::chrono::steady_clock::now();
        // BEGIN CORE: 调用抽出来的筛选函数；计时范围不变（含全部输出写入）。
        matched = filter_records(input.data(), input.size(), output.data(), query);
        // END CORE: include all output writes, exclude validation and console output.
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        if (matched!=expected.size()) throw std::runtime_error("validation: wrong count");
        for (std::size_t i=0; i<matched; ++i)
            if (!equal_record(output[i],expected[i])) throw std::runtime_error("validation: wrong record/order at "+std::to_string(i));
        if (run>=0) {
            times.push_back(ms);
            std::cout << "sample=" << run << " core_ms=" << ms << " matched=" << matched << " check=PASS\n";
        }
    }
    std::sort(times.begin(),times.end());
    const std::size_t middle=times.size()/2;
    const double median=times.size()%2 ? times[middle] : (times[middle-1]+times[middle])/2;
    std::cout << "median_core_ms=" << median << " actual_hit_percent=" << (input.empty()?0:100.0*matched/input.size()) << " input_Mrecords_per_s=";
    if(median>0) std::cout << input.size()/(median*1000.0);
    else std::cout << "unavailable(timer_resolution)";
    std::cout << '\n';
    // Serialize only the final validated result. File writing is NOT core timing.
    if (!output_path.empty()) save(output_path,output,matched);
    return 0;
} catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 2;
}
