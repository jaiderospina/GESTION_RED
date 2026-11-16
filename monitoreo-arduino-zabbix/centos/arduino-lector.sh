#!/bin/bash
# Lee las lineas DATOS del Arduino por USB y guarda el ultimo valor de cada sensor.
# Si no se indica puerto, usa el primer /dev/ttyACM* o /dev/ttyUSB* disponible
# (al reconectar el Arduino el puerto puede cambiar de ttyACM0 a ttyACM1).
PUERTO="${1:-$(ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | head -1)}"
DIR=/var/lib/arduino
mkdir -p "$DIR"
stty -F "$PUERTO" 9600 raw -echo || exit 1
while IFS= read -r linea; do
  linea="${linea%$'\r'}"
  [[ "$linea" == DATOS\;* ]] || continue
  printf '%s\n' "${linea#DATOS;}" | tr ';' '\n' > "$DIR/estado.tmp"
  mv -f "$DIR/estado.tmp" "$DIR/estado.txt"
done < "$PUERTO"
