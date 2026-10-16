#!/usr/bin/env bash
GROUP=archive
ACCOUNT=archiver
TASK_FILE=${TASK_FILE:-/workspace/task.txt}
FLAG=${FLAG:-/workspace/flag}
NOLOGIN=/usr/sbin/nologin
die() { printf 'setup-account.sh: %s\n' "$*" >&2; exit 1; }
[ "$(id -u)" = "0" ] || die "必须以 root 运行"
[ -f "$TASK_FILE" ] || die "找不到任务文件 $TASK_FILE"
if ! getent group "$GROUP" >/dev/null 2>&1; then groupadd --system "$GROUP" || die "创建组失败"; fi
GID=$(getent group "$GROUP" | cut -d: -f3)
[ -n "$GID" ] || die "无法确定组 $GROUP 的 GID"
if getent passwd "$ACCOUNT" >/dev/null 2>&1; then
  line=$(getent passwd "$ACCOUNT")
  uid=$(printf '%s' "$line" | cut -d: -f3)
  pgid=$(printf '%s' "$line" | cut -d: -f4)
  pshell=$(printf '%s' "$line" | cut -d: -f7)
  case "$uid" in ''|*[!0-9]*) die "已有账号 $ACCOUNT UID 非法" ;; esac
  [ "$uid" -ge 200 ] && [ "$uid" -le 999 ] || die "已有账号 UID=$uid 不在 200-999"
  [ "$pgid" = "$GID" ] || die "已有账号主组不是 $GROUP"
  [ "$pshell" = "$NOLOGIN" ] || die "已有账号 shell 不是 $NOLOGIN"
else
  newuid=
  for u in $(seq 200 999); do
    if ! getent passwd "$u" >/dev/null 2>&1; then newuid=$u; break; fi
  done
  [ -n "$newuid" ] || die "200-999 没有空闲 UID"
  useradd --system --no-create-home --home-dir /nonexistent --shell "$NOLOGIN" --gid "$GROUP" --uid "$newuid" "$ACCOUNT" || die "创建账号失败"
fi
mode=$(stat -c '%a' "$TASK_FILE")
[ "$mode" = "640" ] || chmod 640 "$TASK_FILE" || die "chmod 失败"
owner=$(stat -c '%U:%G' "$TASK_FILE")
[ "$owner" = "$ACCOUNT:$GROUP" ] || chown "$ACCOUNT:$GROUP" "$TASK_FILE" || die "chown 失败"
read_task() {
  cat "$TASK_FILE" 2>/dev/null && return 0
  runuser -u "$ACCOUNT" -- cat "$TASK_FILE" 2>/dev/null && return 0
  su -s /bin/sh -c "cat '$TASK_FILE'" "$ACCOUNT" 2>/dev/null && return 0
  chown root:root "$TASK_FILE" 2>/dev/null || return 1
  chmod 600 "$TASK_FILE" 2>/dev/null
  cat "$TASK_FILE" 2>/dev/null
  rc=$?
  chmod 640 "$TASK_FILE" 2>/dev/null
  chown "$ACCOUNT:$GROUP" "$TASK_FILE" 2>/dev/null
  return $rc
}
content=$(read_task) || die "无法读取 $TASK_FILE"
code=$(printf '%s\n' "$content" | head -n 1 | sed -e 's/\xEF\xBC\x9A/:/g' -e 's/^[^:]*:[[:space:]]*//' -e 's/[[:space:]]*$//')
[ -n "$code" ] || die "找不到任务码"
printf '%s\n' "$code" > "$FLAG" || die "写 flag 失败"
exit 0
