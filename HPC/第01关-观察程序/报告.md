这里要求文件这样摆,不然不给过.一开始我直接平铺6个文件,然后终端告诉我要这样摆,类似c的格式.
HPC/
├── CMakeLists.txt          ← 构建配置文件
├── include/                ← 头文件目录
│   ├── linux_procfs.hpp
│   └── metrics_schema.hpp
└── src/                    ← 源代码目录
    ├── collector.cpp
    ├── linux_procfs.cpp
    └── workload.cpp
    
看一看 CMakeLists.txt里面是什么
cat CMakeLists.txt
(太多了,就不展示了,就是一些环境相关的)

因为前面CMakeLists.txt而且.cpp是源文件,我们需要处理一下才能用.大概是把头文件和源文件都处理,编译,得到可执行文件.
暗示我们可以用
cmake -S . -B build
 cmake --build build -j4   

拆解
cmake  -S .      -B build
  │     │          │
  │     │          └─ 构建产物放哪（B = Build 目录）
  │     └─ 源码在哪（S = Source，`.` 表示当前目录）
  └─ 用 cmake

cmake  --build build   -j4
  │        │             │
  │        │             └─ 用 4 个核心并行编译（快）
  │        └─ 去 build/ 目录里执行构建
  └─ 用 cmake

然后tree,可以发现生成了一大堆文件.其中就有题目提到的workload和collector

但是我不知道怎么做,应该用什么命令,所以看一看
(base) kim@kim:~/dsh_workspace/超算考核/HPC-重做1$ build/workload --help
usage: workload [--mode cpu] --threads N --duration DURATION [--pid-file PATH] [--load-percent 1..100]
(base) kim@kim:~/dsh_workspace/超算考核/HPC-重做1$ build/collector --help
usage: collector (--pid PID | --pid-file PATH) [--interval DURATION] [--duration DURATION] --output PATH [--overwrite]

然后我用ai查了一下单词是什么意思:
usage	用法
workload	工作负载
threads	线程
duration	持续时间
pid	进程号
load	负载
[ ]	可选的
没 [ ]	必须的
N、PATH、DURATION	占位符，要填具体值
interval	间隔
overwrite	覆盖
( ... | ... )	二选一，必须选一个

这里题目好像没有规定线程数量,这里我设置为8好了,毕竟题目要求的文件名 cpu-t8.jsonl 暗示了 t8 = 8 线程;这里运行的时间设置久一点,免得手速太慢没监控够时间,我第一次就是没弄够时间,题目里面要求end.reason = duration_reached就是想让 collector是正常退出的.

./build/workload --threads 8 --duration 60s --pid-file workload.pid
mkdir -p evidence

用--overwrite允许覆盖已存在的同名文件（不加它，程序会拒绝写）,用--output evidence/cpu-t8.jsonl决定写到哪个文件,方便自己免得找
./build/collector --pid-file workload.pid --output evidence/cpu-t8.jsonl --overwrite

分开两个终端跑,先让workload跑起来才可以监控.

完成之后有
(base) kim@kim:~/dsh_workspace/超算考核/HPC-重做1$ cd ~/dsh_workspace/超算考核/HPC-重做1
mkdir -p evidence
./build/collector --pid-file workload.pid --output evidence/cpu-t8.jsonl --overwrite
(base) kim@kim:~/dsh_workspace/超算考核/HPC-重做1$ tail -1 evidence/cpu-t8.jsonl 
{"schema":"hpc-metrics.v1","event":"end","sequence":41,"wall_time":"2026-10-12T08:56:37.741Z","monotonic_ns":21201377051050,"elapsed_ms":20000,"reason":"duration_reached","samples_written":40,"missed_deadlines":0,"unavailable_fields":[]}
(base) kim@kim:~/dsh_workspace/超算考核/HPC-重做1$ head -2 evidence/cpu-t8.jsonl | tail -1
{"schema":"hpc-metrics.v1","event":"sample","sequence":1,"wall_time":"2026-10-12T08:56:18.241Z","monotonic_ns":21181876680733,"elapsed_ms":500,"target":{"pid":36177,"start_ticks":3581726},"process":{"cpu_cores_percent":799.7,"cpu_capacity_percent":28.6,"threads":9,"rss_bytes":3833856,"read_bytes":0,"write_bytes":4096,"voluntary_context_switches_delta":0,"involuntary_context_switches_delta":0},"host":{"cpu_utilization_percent":29.6,"load1":1.70,"memory_available_bytes":26012430336},"unavailable":[]}
(base) kim@kim:~/dsh_workspace/超算考核/HPC-重做1$ wc -l evidence/cpu-t8.jsonl   
42 evidence/cpu-t8.jsonl

然后是检查 reason 是不是 duration_reached和看 threads 和 cpu_cores_percent
我是怎么做的呢?这确实是太乱了吧,所以我直接复制给ai,让ai帮忙看
或者用这一条head -2 evidence/cpu-t8.jsonl | tail -1 | python3 -m json.tool,来转换成json格式来看

检查行数,因为题目要求包含 1 条 start、至少 15 条 sample 和 1 条 end
wc -l evidence/cpu-t8.jsonl 

都没问题,就提交