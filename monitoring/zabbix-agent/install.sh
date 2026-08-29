#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
TARGET_DIR="/usr/local/libexec/zabbix"
CONF_DIR="/etc/zabbix/zabbix_agentd.d"

if [[ "${EUID}" -ne 0 ]]; then
  echo "Ejecuta este instalador con sudo." >&2
  exit 1
fi

install -d -m 0750 -o root -g zabbix "$TARGET_DIR"
install -m 0750 -o root -g zabbix "$SCRIPT_DIR/dir_size.sh" "$TARGET_DIR/gestion-red-dir-size.sh"
install -m 0750 -o root -g zabbix "$SCRIPT_DIR/iface_status.sh" "$TARGET_DIR/gestion-red-iface-status.sh"
install -d -m 0750 -o root -g zabbix "$CONF_DIR"
install -m 0640 -o root -g zabbix "$SCRIPT_DIR/gestion-red.conf" "$CONF_DIR/gestion-red.conf"

zabbix_agentd -t 'gestion.folder.size[/var/log]' || true
zabbix_agentd -t 'gestion.if.status[lo]' || true
systemctl restart zabbix-agent
systemctl --no-pager --full status zabbix-agent

echo "Checks instalados. Valida con:"
echo "  zabbix_agentd -t 'gestion.folder.size[/var/log]'"
echo "  zabbix_agentd -t 'gestion.if.status[lo]'"
