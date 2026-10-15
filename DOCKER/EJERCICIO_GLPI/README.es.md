# Laboratorio académico de GLPI 11

Este proyecto crea un laboratorio didáctico reproducible de GLPI 11 en un solo host, con MariaDB, almacenamiento persistente, inventario nativo mediante GLPI Agent y una segunda ranura de plugin para Formcreator.

## Aplicabilidad de DHI

La migración a DHI no aplica al directorio vacío inicial: no existía ningún Dockerfile ni imagen base que migrar. El flujo correcto es crear primero la pila de la aplicación y, después, evaluar una imagen personalizada posterior para su compatibilidad con DHI. Las imágenes de GLPI y de los plugins son imágenes de aplicación, no objetivos de migración de imágenes base DHI.

## Inicio

1. Copia `.env.example` a `.env` y cambia las contraseñas.
2. Ejecuta `docker compose build` y luego `docker compose up -d --no-build`.
3. Abre <http://localhost:8080> y completa el instalador de GLPI si no se completa automáticamente.
4. Usa el host de base de datos `db`, la base de datos `glpi`, el usuario `glpi` y la contraseña configurada.
5. Ejecuta el SQL de datos iniciales (seed) después de la instalación:

```powershell
docker compose exec -T db mariadb -uglpi -pglpi-lab-change-me glpi < bootstrap/seed.sql
```

Los volúmenes con nombre `db_data`, `glpi_files`, `glpi_plugins` y `agent_state` hacen persistente el laboratorio.

## Plugins e inventario

FusionInventory es un plugin heredado (legacy) y no es la vía de inventario soportada para GLPI 11. Usa GLPI Agent con el endpoint de inventario nativo de GLPI. Formcreator es el plugin de flujos de trabajo adicional; instala una versión marcada explícitamente como compatible con GLPI 11, verifica su checksum y colócala en el volumen persistente de plugins antes de habilitarla.

## Ejercicio ITSM reproducible

Crea estos registros en secuencia:

1. Entidad: Academic Lab.
2. Ubicaciones: Room A-101 y Room A-102.
3. Usuarios: Student, Lab Technician e Instructor.
4. Activo: portátil LAB-001 en Room A-101.
5. Ticket: «Laptop cannot join campus Wi-Fi».
6. Categoría: Network > Wireless.
7. Asigna el ticket a Lab Technician.
8. Añade un seguimiento solicitando el SSID y el mensaje de error del estudiante.
9. Resuelve con una actualización de controlador documentada y cierra tras la validación.
10. Crea un formulario de solicitud en Formcreator para la aprobación de nuevo equipo de laboratorio.

## Validación

```powershell
docker compose config --quiet
docker compose build
docker compose up -d --no-build
docker compose ps
docker compose logs --tail=100 db glpi
```

No incluyas secretos de producción en la imagen ni subas el archivo `.env` al repositorio.
