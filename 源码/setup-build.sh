#!/bin/sh
# setup-build.sh —— 配置 C 构建环境:安装 build-essential,
# 并处理妨碍安装的练习包 ncuscc-build-guard。可重复执行。
#
# 只做"环境配置":编译由 /workspace/build.sh 负责。
# 末尾的编译+生成 flag 是"尽力而为",任何失败都不影响本脚本的退出码。

set -eu
export DEBIAN_FRONTEND=noninteractive

# 1) 移除妨碍 build-essential 安装的练习包(已移除则跳过)
if dpkg-query -W -f='${Status}' ncuscc-build-guard 2>/dev/null | grep -q 'install ok installed'; then
    apt-get remove -y ncuscc-build-guard
fi

# 2) 安装 build-essential(已安装时 apt 直接成功返回,天然幂等)
if ! apt-get install -y build-essential; then
    # 兜底:仅当软件包索引缺失时才刷新(APT 指向镜像内离线仓库)
    apt-get update -qq || true
    apt-get install -y build-essential
fi

# 3) 校验:基础包仍在,且 gcc 可用
for p in apt libc6; do
    if ! dpkg-query -W -f='${Status}' "$p" 2>/dev/null | grep -q 'install ok installed'; then
        echo "setup-build.sh: 基础包 $p 缺失" >&2
        exit 1
    fi
done
if ! command -v gcc >/dev/null 2>&1; then
    echo "setup-build.sh: gcc 不可用" >&2
    exit 1
fi

# 4) 尽力而为:源码就绪时编译并产出 flag(失败一律忽略,不影响退出码)
if [ -f /workspace/hello.c ] && [ -f /workspace/build.sh ]; then
    bash /workspace/build.sh >/dev/null 2>&1 || true
    if [ -x /workspace/hello ]; then
        /workspace/hello foundation > /workspace/flag 2>/dev/null || true
    fi
fi

exit 0
