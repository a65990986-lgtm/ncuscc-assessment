# 第三关：日志压缩与稳定筛选优化

## 构建

三个版本使用相同的优化等级与标准（并行版额外需要 pthread）：

    g++ -O2 -std=c++17 -Isrc -o refactored src/refactored.cpp
    g++ -O2 -std=c++17 -Isrc -o branchless src/branchless.cpp
    g++ -O2 -std=c++17 -pthread -Isrc -o parallel   src/parallel.cpp
    g++ -O2 -std=c++17 -Isrc -o baseline   src/baseline.cpp

## 运行

    ./refactored --count 100000 --pattern random --hit-percent 50 --repeats 10

常用参数：
    --count N           记录条数（默认 100000）
    --pattern random|blocks   命中分布（默认 random）
    --hit-percent 0..100      命中率（默认 50）
    --repeats N         重复测量次数（默认 5，预热 1 次不计入）
    --begin/--end/--min-latency/--flags   查询条件

## 测试

    python3 src/test_public.py   ./refactored    # 组织者提供的公开测试（7 项）
    python3 src/test_boundary.py ./refactored    # 新增边界测试（24 项）

三个版本（refactored / branchless / parallel）均应全部通过。

## 目录说明

    src/                     源码与测试
      baseline.cpp           原始程序（未改动，备份）
      refactored.cpp         重构版（抽取 filter_records 接口）
      branchless.cpp         候选 A：无分支筛选
      parallel.cpp           候选 B：多线程 + 两遍扫描 + 前缀和
      support.hpp            组织者提供
      test_public.py         组织者提供的公开测试
      test_boundary.py       新增边界测试
    results/                 原始测量数据（每组 10~20 次样本）
    第03关-日志筛选优化/
      HPC3报告.md             实验报告
    TASK.md                  组织者提供的任务说明
    report-template.md       组织者提供的报告模板
    generate_logs.py         组织者提供
