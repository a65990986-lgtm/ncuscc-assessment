#!/usr/bin/env bash
usage() { printf 'usage: route.sh MODE ITEM  (MODE: ok|retry|fatal, ITEM: 1-32 alnum)\n'; }
[ "$#" -ne 2 ] && { usage; exit 64; }
mode=$1; item=$2
case "$mode" in ok|retry|fatal) ;; *) usage; exit 64 ;; esac
case "$item" in ''|*[!A-Za-z0-9]*) usage; exit 64 ;; esac
[ "${#item}" -gt 32 ] && { usage; exit 64; }
config="$(dirname "$(readlink -f "$0")")/job.env"
[ -f "$config" ] || config=/workspace/job.env
set -a; . "$config"; set +a
PATH="${TASK_TOOL_DIR}:${PATH}"; export PATH
output=$(task-probe "$mode" "$item"); status=$?
case "$status" in
  0)  result=$output;          rc=0  ;;
  11) result="retry:$item";    rc=75 ;;
  23) result="rejected:$item"; rc=65 ;;
  *)  exit "$status" ;;
esac
printf '%s\n' "$result" > "$ROUTE_RESULT"
exit "$rc"
