#include "support.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <set>

// ============================================================================
// 核心筛选函数（重构后抽出来的）
//
// 约定（题目要求）：
//   - input / output 不重叠；output 容量至少 count
//   - count == 0 时允许 input / output 为空指针
//   - 返回命中条数；有效结果只是 output 的前 matched 个
//   - 不打印、不读文件、不造测试数据、不调用参考实现
//   - 保持输入顺序（稳定筛选），保留全部字段，不去重
// ============================================================================
// ============================================================================
// 优化版：无分支（branchless）筛选
//
// 思路：原版每个记录都要执行一次 if 判断，命中/不命中的分支随机出现时
//       CPU 分支预测器会频繁猜错，每次猜错要清空流水线（约 10~20 周期）。
//       这里把判断结果算成 0/1，用"无条件写入 + 指针按命中数前进"代替分支。
//
// 正确性：有效结果只是 output 的前 matched 条。
//         不命中时 bit=0，会把同一条未命中记录重复写到同一个位置，
//         该位置不在有效结果范围内，因此不影响输出。
// ============================================================================
static std::size_t filter_records(const Record* input, std::size_t count,
                                  Record* output, const Query& query) {
    std::size_t matched = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const Record& r = input[i];
        // 用 & 而不是 &&：按位与不短路，避免产生分支
        const bool hit = (query.begin <= r.timestamp) & (r.timestamp < query.end) &
                         (r.latency_us >= query.min_latency_us) &
                         ((r.flags & query.required_flags) == query.required_flags);
        const std::size_t bit = hit ? 1u : 0u;
        output[matched] = r;   // 无条件写
        matched += bit;        // 只有命中才前进
    }
    return matched;
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
