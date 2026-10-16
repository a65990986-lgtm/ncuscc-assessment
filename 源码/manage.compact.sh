#!/usr/bin/env bash
WORKER=${WORKER:-/opt/recruitment/bin/slow-worker}
WORK_DIR=${WORK_DIR:-/workspace}
PIDFILE="$WORK_DIR/worker.pid"
NAME=$(basename "$WORKER")
usage() { printf 'usage: manage.sh start|status|report|stop\n'; }
worker_alive() {
  pid=$1
  [ -n "$pid" ] || return 1
  [ -r "/proc/$pid/stat" ] || return 1
  st=$(awk '{print $3}' "/proc/$pid/stat" 2>/dev/null)
  [ "$st" = "Z" ] && return 1
  tr '\0' ' ' < "/proc/$pid/cmdline" 2>/dev/null | grep -q -- "$NAME"
}
read_pid() {
  [ -f "$PIDFILE" ] || return 1
  p=$(tr -dc '0-9' < "$PIDFILE" 2>/dev/null)
  [ -n "$p" ] || return 1
  printf '%s' "$p"
}
running_pid() {
  p=$(read_pid) || return 1
  worker_alive "$p" || return 1
  printf '%s' "$p"
}
cmd_start() {
  if running_pid >/dev/null 2>&1; then return 0; fi
  mkdir -p "$WORK_DIR" 2>/dev/null
  rm -f "$PIDFILE"
  export WORK_DIR
  cd "$WORK_DIR" 2>/dev/null
  nohup "$WORKER" </dev/null >>/dev/null 2>&1 &
  echo $! > "$PIDFILE"
  return 0
}
cmd_status() {
  if running_pid >/dev/null 2>&1; then echo running; return 0; fi
  echo stopped
  return 1
}
cmd_report() {
  pid=$(running_pid) || return 1
  {
    printf 'PID=%s\n' "$pid"
    ps -o pid,ppid,user,stat,etime,time,args -p "$pid"
    top -b -n 1 -p "$pid"
  } > "$WORK_DIR/resource.txt"
  return 0
}
cmd_stop() {
  pid=$(running_pid) || { rm -f "$PIDFILE"; return 0; }
  kill -TERM "$pid" 2>/dev/null
  i=0
  while [ "$i" -lt 100 ]; do
    worker_alive "$pid" || break
    sleep 0.1
    i=$((i + 1))
  done
  rm -f "$PIDFILE"
  return 0
}
case "${1:-}" in
  start)  cmd_start  ;;
  status) cmd_status ;;
  report) cmd_report ;;
  stop)   cmd_stop   ;;
  *)      usage; exit 64 ;;
esac
exit $?
