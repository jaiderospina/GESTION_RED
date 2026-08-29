#!/usr/bin/env bash
set -euo pipefail

# Return Linux operational state: 1=up, 0=down, 2=unknown.
iface="${1:-}"
if [[ -z "$iface" || "$iface" == *[!A-Za-z0-9_.:-]* ]]; then
  echo "Invalid interface name" >&2
  exit 2
fi

state="$(cat "/sys/class/net/${iface}/operstate" 2>/dev/null || true)"
case "$state" in
  up|unknown) echo 1 ;;
  down|dormant|lowerlayerdown|notpresent|"") echo 0 ;;
  *) echo 2 ;;
esac
