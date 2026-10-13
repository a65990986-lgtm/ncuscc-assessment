#!/usr/bin/env python3
"""
边界测试（第三关）

公开测试 test_public.py 覆盖的是常规路径；这里补充极端/边界输入，
重点检查：
  - 0 条数据（空输入）
  - 全部命中 / 全部不命中
  - 极小规模（少于线程数）
  - 空查询区间（begin == end）
  - 恰好 1 条 / 恰好 2 条
  - 规模不是线程数的整数倍（并行分块的最后一块会偏小）

用法：
    python3 test_boundary.py ./refactored
    python3 test_boundary.py ./branchless
    python3 test_boundary.py ./parallel
"""
import subprocess
import sys

EXE = sys.argv[1] if len(sys.argv) > 1 else "./refactored"

passed = 0
failed = 0
failures = []


def run(*args):
    """跑一次，返回 (返回码, stdout, stderr)"""
    p = subprocess.run([EXE, *args], capture_output=True, text=True)
    return p.returncode, p.stdout, p.stderr


def summary(stdout):
    """从输出里抽出关键字段"""
    out = {}
    for line in stdout.splitlines():
        if line.startswith("median_core_ms="):
            for kv in line.split():
                if "=" in kv:
                    k, v = kv.split("=", 1)
                    out[k] = v
        if line.startswith("sample="):
            for kv in line.split():
                if "=" in kv:
                    k, v = kv.split("=", 1)
                    out[k] = v
    return out


def check(name, cond, detail=""):
    global passed, failed
    if cond:
        passed += 1
        print(f"  PASS  {name}")
    else:
        failed += 1
        failures.append(name)
        print(f"  FAIL  {name}  {detail}")


print(f"被测程序: {EXE}\n")

# --- 1. 0 条数据（空输入）---
print("[1] 空输入 count=0")
rc, out, err = run("--count", "0", "--repeats", "3")
check("退出码为 0", rc == 0, f"rc={rc} err={err.strip()}")
if rc == 0:
    s = summary(out)
    check("命中数为 0", s.get("matched") == "0", f"matched={s.get('matched')}")
    check("check=PASS", s.get("check") == "PASS", f"check={s.get('check')}")

# --- 2. 恰好 1 条（命中/不命中的可能性都覆盖）---
print("\n[2] 极小规模 count=1")
rc, out, err = run("--count", "1", "--repeats", "3")
check("退出码为 0", rc == 0, f"rc={rc} err={err.strip()}")
if rc == 0:
    s = summary(out)
    check("check=PASS", s.get("check") == "PASS", f"check={s.get('check')}")

# --- 3. 少于线程数（并行版会分块，块数 > 数据条数）---
print("\n[3] 数据条数少于线程数 count=3")
rc, out, err = run("--count", "3", "--repeats", "3")
check("退出码为 0", rc == 0, f"rc={rc} err={err.strip()}")
if rc == 0:
    s = summary(out)
    check("check=PASS", s.get("check") == "PASS", f"check={s.get('check')}")

# --- 4. 全部命中（hit-percent=100）---
print("\n[4] 全部命中 hit-percent=100")
rc, out, err = run("--count", "10000", "--hit-percent", "100", "--repeats", "3")
check("退出码为 0", rc == 0, f"rc={rc} err={err.strip()}")
if rc == 0:
    s = summary(out)
    check("命中数 = 输入条数", s.get("matched") == "10000", f"matched={s.get('matched')}")
    check("check=PASS", s.get("check") == "PASS", f"check={s.get('check')}")

# --- 5. 全部不命中（hit-percent=0）---
print("\n[5] 全部不命中 hit-percent=0")
rc, out, err = run("--count", "10000", "--hit-percent", "0", "--repeats", "3")
check("退出码为 0", rc == 0, f"rc={rc} err={err.strip()}")
if rc == 0:
    s = summary(out)
    check("命中数为 0", s.get("matched") == "0", f"matched={s.get('matched')}")
    check("check=PASS", s.get("check") == "PASS", f"check={s.get('check')}")

# --- 6. 空查询区间（begin == end，一条都不该命中）---
print("\n[6] 空查询区间 begin=1500 end=1500")
rc, out, err = run("--count", "10000", "--begin", "1500", "--end", "1500", "--repeats", "3")
check("退出码为 0", rc == 0, f"rc={rc} err={err.strip()}")
if rc == 0:
    s = summary(out)
    check("命中数为 0", s.get("matched") == "0", f"matched={s.get('matched')}")
    check("check=PASS", s.get("check") == "PASS", f"check={s.get('check')}")

# --- 7. 规模不是线程数的整数倍（并行最后一块偏小）---
print("\n[7] 非整块规模 count=100003")
rc, out, err = run("--count", "100003", "--repeats", "3")
check("退出码为 0", rc == 0, f"rc={rc} err={err.strip()}")
if rc == 0:
    s = summary(out)
    check("check=PASS", s.get("check") == "PASS", f"check={s.get('check')}")

# --- 8. blocks 模式 + 全命中 ---
print("\n[8] blocks 模式 + 全部命中")
rc, out, err = run("--count", "50000", "--pattern", "blocks", "--hit-percent", "100", "--repeats", "3")
check("退出码为 0", rc == 0, f"rc={rc} err={err.strip()}")
if rc == 0:
    s = summary(out)
    check("命中数 = 输入条数", s.get("matched") == "50000", f"matched={s.get('matched')}")
    check("check=PASS", s.get("check") == "PASS", f"check={s.get('check')}")

# --- 9. 宽查询区间（几乎全命中，但走的是不同条件分支）---
print("\n[9] 极宽查询区间 begin=0 end=99999999")
rc, out, err = run("--count", "20000", "--begin", "0", "--end", "99999999",
                   "--min-latency", "0", "--flags", "0", "--repeats", "3")
check("退出码为 0", rc == 0, f"rc={rc} err={err.strip()}")
if rc == 0:
    s = summary(out)
    check("命中数 = 输入条数", s.get("matched") == "20000", f"matched={s.get('matched')}")
    check("check=PASS", s.get("check") == "PASS", f"check={s.get('check')}")

print(f"\n{'='*46}")
print(f"通过 {passed} 项，失败 {failed} 项")
if failures:
    print("失败项：" + ", ".join(failures))
sys.exit(1 if failed else 0)
