#!/usr/bin/env bash
# setup-account.sh —— 建立 archive 组与 archiver 服务账号,
# 设定任务文件的归属与权限,并按任务说明生成 /workspace/flag。
# 必须以 root 运行;可重复执行(第二次运行应复用已有账号与组)。
#
# 刻意不写 set -e: 需要自己判断每个分支的成败并给出明确错误。

GROUP=archive
ACCOUNT=archiver
TASK_FILE=${TASK_FILE:-/workspace/task.txt}
FLAG=${FLAG:-/workspace/flag}
NOLOGIN=/usr/sbin/nologin

die() { printf 'setup-account.sh: %s\n' "$*" >&2; exit 1; }

[ "$(id -u)" = "0" ] || die "必须以 root 运行"
[ -f "$TASK_FILE" ] || die "找不到任务文件 $TASK_FILE"

# ---------- 1. 组: 已存在就复用,否则建系统组 ----------
if ! getent group "$GROUP" >/dev/null 2>&1; then
    groupadd --system "$GROUP" || die "创建组 $GROUP 失败"
fi
GID=$(getent group "$GROUP" | cut -d: -f3)
[ -n "$GID" ] || die "无法确定组 $GROUP 的 GID"

# ---------- 2. 账号: 已存在就核对,否则建系统账号 ----------
if getent passwd "$ACCOUNT" >/dev/null 2>&1; then
    line=$(getent passwd "$ACCOUNT")
    uid=$(printf '%s' "$line" | cut -d: -f3)
    pgid=$(printf '%s' "$line" | cut -d: -f4)
    pshell=$(printf '%s' "$line" | cut -d: -f7)
    # 已有账号必须完全符合要求,否则报错退出(不能悄悄接受)
    case "$uid" in ''|*[!0-9]*) die "已有账号 $ACCOUNT 的 UID 非法: $uid" ;; esac
    [ "$uid" -ge 200 ] && [ "$uid" -le 999 ] || die "已有账号 $ACCOUNT 的 UID=$uid 不在 200-999"
    [ "$pgid" = "$GID" ]     || die "已有账号 $ACCOUNT 的主组不是 $GROUP"
    [ "$pshell" = "$NOLOGIN" ] || die "已有账号 $ACCOUNT 的登录 shell 不是 $NOLOGIN"
else
    # 在 200-999 中挑一个空闲 UID(系统默认范围可能含 100-199,不满足要求)
    newuid=
    for u in $(seq 200 999); do
        if ! getent passwd "$u" >/dev/null 2>&1; then newuid=$u; break; fi
    done
    [ -n "$newuid" ] || die "200-999 中没有空闲 UID"
    useradd --system --no-create-home --home-dir /nonexistent \
            --shell "$NOLOGIN" --gid "$GROUP" --uid "$newuid" "$ACCOUNT" \
        || die "创建账号 $ACCOUNT 失败"
fi

# ---------- 3. 文件归属与权限 ----------
# 先 chmod 再 chown: chmod 需要"属主身份",此刻 root 仍是属主;
# 一旦 chown 给 archiver,root 就不是属主了。
mode=$(stat -c '%a' "$TASK_FILE")
[ "$mode" = "640" ] || chmod 640 "$TASK_FILE" || die "chmod $TASK_FILE 失败"
owner=$(stat -c '%U:%G' "$TASK_FILE")
[ "$owner" = "$ACCOUNT:$GROUP" ] || chown "$ACCOUNT:$GROUP" "$TASK_FILE" \
    || die "chown $TASK_FILE 失败"

# ---------- 4. 读取任务说明 ----------
# 本关 root 没有 CAP_DAC_OVERRIDE,读不了 640 的属主文件。
# 依次尝试: 直接读 → 换成 archiver 身份读 → 临时把文件交回 root 读。
read_task() {
    cat "$TASK_FILE" 2>/dev/null && return 0
    runuser -u "$ACCOUNT" -- cat "$TASK_FILE" 2>/dev/null && return 0
    su -s /bin/sh -c "cat '$TASK_FILE'" "$ACCOUNT" 2>/dev/null && return 0
    # 最后手段: 临时改回 root 属主读完再还原(先 chmod 再 chown,理由同上)
    chown root:root "$TASK_FILE" 2>/dev/null || return 1
    chmod 600 "$TASK_FILE" 2>/dev/null
    cat "$TASK_FILE" 2>/dev/null
    rc=$?
    chmod 640 "$TASK_FILE" 2>/dev/null
    chown "$ACCOUNT:$GROUP" "$TASK_FILE" 2>/dev/null
    return $rc
}

content=$(read_task) || die "无法读取 $TASK_FILE"

# ---------- 5. 取任务码,写 flag ----------
# 只用 ASCII 正则处理(不依赖 locale),并兼容全角冒号。
code=$(printf '%s\n' "$content" | head -n 1 \
       | sed -e 's/\xEF\xBC\x9A/:/g' \
       | sed -e 's/^[^:]*:[[:space:]]*//' -e 's/[[:space:]]*$//')
[ -n "$code" ] || die "在 $TASK_FILE 中找不到任务码"

printf '%s\n' "$code" > "$FLAG" || die "写 $FLAG 失败"
exit 0
