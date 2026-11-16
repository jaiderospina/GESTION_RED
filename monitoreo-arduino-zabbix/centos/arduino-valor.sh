#!/bin/bash
# Devuelve un valor del Arduino para el agente Zabbix.
# Si no hay datos de los ultimos 30 s no imprime nada y Zabbix marca el item sin datos.
ARCHIVO=/var/lib/arduino/estado.txt
[ -f "$ARCHIVO" ] || exit 1
edad=$(( $(date +%s) - $(stat -c %Y "$ARCHIVO") ))
[ "$edad" -le 30 ] || exit 1
awk -F= -v clave="$1" '$1 == clave { print $2 }' "$ARCHIVO"
