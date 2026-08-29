#!/usr/bin/env bash
set -euo pipefail

# Return the directory size in bytes for a path supplied by a Zabbix item.
# The path must be absolute and must not contain a newline.
path="${1:-}"
if [[ -z "$path" || "$path" != /* || "$path" == *$'\n'* || "$path" == *$'\r'* ]]; then
  echo "Invalid absolute directory path" >&2
  exit 2
fi

if [[ ! -d "$path" ]]; then
  echo "Directory does not exist: $path" >&2
  exit 3
fi

du -sB1 -- "$path" 2>/dev/null | awk 'NR == 1 { print $1; found = 1 } END { if (!found) exit 1 }'
