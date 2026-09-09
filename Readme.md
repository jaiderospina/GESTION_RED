# CentOS Stream 10 + Zabbix 7.4

[![CentOS Stream 10](https://img.shields.io/badge/OS-CentOS%20Stream%2010-262577?logo=centos&logoColor=white)](https://www.centos.org/stream/)
[![Zabbix 7.4](https://img.shields.io/badge/Zabbix-7.4-d40000?logo=zabbix&logoColor=white)](https://www.zabbix.com/)
[![VirtualBox](https://img.shields.io/badge/VirtualBox-red?logo=virtualbox&logoColor=white)](https://www.virtualbox.org/)
[![Monitoreo autorizado](https://img.shields.io/badge/monitoreo-autorizado-1f6feb)](#alcance-y-privacidad)

## Laboratorio de instalación, monitoreo y alertas

Este proyecto presenta una ruta reproducible para instalar **CentOS Stream 10** en una máquina virtual, desplegar **Zabbix 7.4** con MariaDB/MySQL y Apache, conectar un equipo físico mediante el agente Zabbix y enviar alertas técnicas por correo electrónico.

> **Objetivo del laboratorio:** observar la disponibilidad, el rendimiento y los eventos técnicos de equipos autorizados, y notificar los problemas relevantes a un buzón operativo.

> **Importante:**  Este procedimiento no incluye keylogging, captura encubierta de pantalla, lectura de archivos privados, interceptación de comunicaciones ni vigilancia secreta.

## Recorrido rápido

| Fase | Resultado |
|---|---|
| 01 · Preparar | VM con CentOS Stream 10 y red accesible |
| 02 · Instalar | Zabbix Server, base de datos y frontend web |
| 03 · Conectar | Equipo físico registrado mediante el agente |
| 04 · Alertar | Medio Email, acción y prueba controlada |
| 05 · Operar | Monitoreo técnico, seguro y documentado |



## Arquitectura

```text
┌──────────────────────────┐       LAN       ┌────────────────────────────────┐
│ Equipo físico autorizado │ ──────────────> │ VM CentOS Stream 10            │
│                          │                │ Zabbix Server 7.4              │
│ Zabbix Agent             │                │ MariaDB/MySQL + Apache          │
│ CPU · RAM · disco        │                │ Panel web: /zabbix              │
│ servicios autorizados    │                └───────────────┬────────────────┘
└──────────────────────────┘                                │
                                                            │ SMTP seguro
                                                            v
                                                   ┌────────────────────┐
                                                   │ Buzón de alertas   │
                                                   └────────────────────┘
```

Para este laboratorio se recomienda configurar la VM con **adaptador puente**. Así, la máquina virtual obtiene una dirección propia en la red autorizada y puede comunicarse directamente con el equipo físico. VirtualBox documenta que NAT permite la salida de la VM, pero mantiene la máquina aislada de conexiones entrantes salvo que se configure redirección de puertos.

## Requisitos sugeridos

| Componente | Valor de laboratorio |
|---|---:|
| CPU de la VM | 2 vCPU |
| Memoria | 4 GiB |
| Disco | 30–40 GiB dinámicos |
| Sistema | CentOS Stream 10 x86_64 |
| Red | Adaptador puente o segmento de laboratorio |
| Base de datos | MariaDB/MySQL |
| Frontend | Apache + PHP |
| Correo | Cuenta técnica autorizada |

## 1. Preparar e instalar la máquina virtual

Descarga la ISO de CentOS Stream 10 desde una fuente oficial y verifica su integridad antes de utilizarla [2]. En Linux:

```bash
sha256sum CentOS-Stream-10-*.iso
```

En Windows PowerShell:

```powershell
Get-FileHash .\CentOS-Stream-10-*.iso -Algorithm SHA256
```

En VirtualBox selecciona **Nueva**, asigna 2 vCPU, 4 GiB de RAM y un disco dinámico de 30–40 GiB. En **Configuración → Red → Adaptador 1**, activa **Adaptador puente** y selecciona la interfaz conectada a la LAN autorizada.

Inicia la ISO, selecciona **Install CentOS Stream 10**, configura el disco, la zona horaria y una cuenta administrativa. Utiliza un nombre como `zabbix-server`. La secuencia visual de preparación e instalación se resume en la siguiente captura del material de apoyo:

![Preparación inicial de CentOS y del entorno de instalación](docs/assets/image1.png)

Después del primer inicio, actualiza el sistema, establece el nombre de host y registra la dirección de red:

```bash
sudo dnf update -y
sudo hostnamectl set-hostname zabbix-server
hostnamectl
ip -br address
ip route
cat /etc/centos-release
```

Anota la IP de la VM como `ZABBIX_SERVER_IP`. Utiliza una IP reservada por el administrador de red o una concesión DHCP estable.

## 2. Instalar Zabbix Server

### 2.1 Paquetes base y repositorio

Instala MariaDB, Apache, PHP y las extensiones requeridas. Después agrega el repositorio oficial de Zabbix 7.4 e instala el servidor, el frontend, los scripts SQL, la política SELinux y el agente local [3] [4].

```bash
sudo dnf install -y mariadb-server httpd php php-fpm php-mysqlnd \
  php-gd php-xml php-bcmath php-mbstring php-ldap php-json php-opcache
sudo systemctl enable --now mariadb httpd php-fpm

sudo rpm -Uvh https://repo.zabbix.com/zabbix/7.4/release/centos/10/noarch/zabbix-release-latest-7.4.el10.noarch.rpm
sudo dnf clean all
sudo dnf install -y zabbix-server-mysql zabbix-web-mysql \
  zabbix-apache-conf zabbix-sql-scripts zabbix-selinux-policy zabbix-agent
```

Comprueba los paquetes instalados y el estado inicial de los servicios:

```bash
rpm -q zabbix-server-mysql zabbix-web-mysql zabbix-agent mariadb-server
sudo systemctl --no-pager --full status mariadb httpd php-fpm
```

La captura siguiente sirve como referencia visual para la preparación del repositorio y la instalación de los paquetes:

![Instalación del repositorio y de los paquetes de Zabbix](docs/assets/image2.png)

Una vez instalados los componentes, valida que el sistema reconoce los paquetes y que los servicios base se encuentran disponibles:

![Comprobación de los componentes instalados](docs/assets/image3.png)

### 2.2 Crear la base de datos

Ejecuta `sudo mariadb` y crea una base de datos y un usuario dedicado. Sustituye el marcador por un secreto largo que no se publique en GitHub:

```sql
CREATE DATABASE zabbix CHARACTER SET utf8mb4 COLLATE utf8mb4_bin;
CREATE USER 'zabbix'@'localhost' IDENTIFIED BY 'CAMBIAR_POR_UN_SECRETO_LARGO';
GRANT ALL PRIVILEGES ON zabbix.* TO 'zabbix'@'localhost';
SET GLOBAL log_bin_trust_function_creators = 1;
FLUSH PRIVILEGES;
EXIT;
```

Importa el esquema inicial y desactiva después el ajuste temporal:

```bash
zcat /usr/share/zabbix/sql-scripts/mysql/server.sql.gz \
  | mysql --default-character-set=utf8mb4 -uzabbix -p zabbix

sudo mariadb -e "SET GLOBAL log_bin_trust_function_creators = 0;"
```

La secuencia de creación e inicialización de la base de datos puede contrastarse con estas capturas de apoyo:

![Creación de la base de datos de Zabbix](docs/assets/image4.png)

![Importación del esquema y preparación de los servicios](docs/assets/image5.png)

Edita `/etc/zabbix/zabbix_server.conf`:

```ini
DBName=zabbix
DBUser=zabbix
DBPassword=CAMBIAR_POR_UN_SECRETO_LARGO
```

Protege el archivo de configuración y evita publicarlo:

```bash
sudo chown root:zabbix /etc/zabbix/zabbix_server.conf
sudo chmod 640 /etc/zabbix/zabbix_server.conf
```

### 2.3 Firewall, SELinux y servicios

No desactives SELinux como solución general. Comprueba su estado y revisa los rechazos recientes antes de realizar ajustes:

```bash
getenforce
sudo ausearch -m AVC -ts recent 2>/dev/null | tail -n 20 || true
```

En un laboratorio con comprobaciones activas desde el equipo físico, abre únicamente HTTP y el puerto del servidor Zabbix:

```bash
sudo firewall-cmd --permanent --add-service=http
sudo firewall-cmd --permanent --add-port=10051/tcp
sudo firewall-cmd --reload
sudo firewall-cmd --list-all
```

Activa los servicios y comprueba los puertos:

```bash
sudo systemctl enable --now zabbix-server zabbix-agent httpd php-fpm
sudo systemctl --no-pager --full status zabbix-server zabbix-agent httpd php-fpm
sudo ss -lntp | grep -E ':80|:10051|:10050' || true
```

## 3. Configurar el frontend web

Abre `http://ZABBIX_SERVER_IP/zabbix` y completa el asistente con la base de datos creada:

| Campo | Valor |
|---|---|
| Database type | MySQL |
| Database host | `localhost` |
| Database name | `zabbix` |
| User | `zabbix` |
| Password | La contraseña de la base |
| Zabbix server name | Un nombre descriptivo, por ejemplo `Global` |
| Time zone | La misma configurada en CentOS |

La pantalla de requisitos previos del frontend permite confirmar que Apache, PHP y los módulos necesarios están disponibles:

![Requisitos previos del frontend web de Zabbix](docs/assets/image6.png)

En la pantalla de base de datos introduce el host, el nombre de la base, el usuario y la contraseña definidos anteriormente. No reutilices el usuario `root` para la conexión de Zabbix:

![Configuración de la conexión entre Zabbix y MariaDB](docs/assets/image7.png)

A continuación define el nombre visible del servidor y la zona horaria. Mantener una zona horaria coherente facilita la interpretación de históricos y eventos:

![Nombre visible y zona horaria del servidor](docs/assets/image8.png)

Revisa el resumen antes de finalizar. Comprueba especialmente el tipo de base de datos, el nombre del servidor y la zona horaria:

![Resumen de la preinstalación de Zabbix](docs/assets/image9.png)

Al terminar, el asistente confirma la instalación y permite acceder al panel:

![Confirmación de la instalación de Zabbix](docs/assets/image10.png)

El panel inicial sirve como evidencia visual de que el frontend quedó operativo:

![Panel inicial de Zabbix](docs/assets/image11.png)

## 4. Conectar el equipo físico

La documentación de Zabbix define los parámetros `Hostname`, `Server` y `ServerActive` del agente UNIX [5]. El valor de `Hostname` debe coincidir exactamente con el nombre del host creado en la interfaz web.

### 4.1 Modalidad activa, recomendada para comenzar

En el equipo físico autorizado instala el agente oficial correspondiente a su sistema operativo. Para otro CentOS Stream 10, el ejemplo es:

```bash
sudo rpm -Uvh https://repo.zabbix.com/zabbix/7.4/release/centos/10/noarch/zabbix-release-latest-7.4.el10.noarch.rpm
sudo dnf clean all
sudo dnf install -y zabbix-agent
```

Edita `/etc/zabbix/zabbix_agentd.conf`:

```ini
Server=ZABBIX_SERVER_IP
ServerActive=ZABBIX_SERVER_IP:10051
Hostname=MONITORED_HOST_NAME
```

Activa el agente y revisa sus registros:

```bash
sudo systemctl enable --now zabbix-agent
sudo systemctl --no-pager --full status zabbix-agent
sudo journalctl -u zabbix-agent -b --no-pager -n 50
```

Desde el equipo físico prueba la salida hacia la VM:

```bash
nc -vz ZABBIX_SERVER_IP 10051
```

### 4.2 Modalidad pasiva

En la modalidad pasiva el servidor consulta al agente por `TCP/10050`:

```ini
Server=ZABBIX_SERVER_IP
ServerActive=ZABBIX_SERVER_IP:10051
Hostname=MONITORED_HOST_NAME
ListenPort=10050
```

Si el equipo físico usa `firewalld`, permite el acceso únicamente desde la IP del servidor:

```bash
sudo firewall-cmd --permanent \
  --add-rich-rule='rule family="ipv4" source address="ZABBIX_SERVER_IP/32" port port="10050" protocol="tcp" accept'
sudo firewall-cmd --reload
```

Desde la VM prueba la conexión:

```bash
nc -vz MONITORED_HOST_IP 10050
```

### 4.3 Registrar el host en Zabbix

En la interfaz web selecciona **Data collection → Hosts → Create host** y utiliza los siguientes valores:

| Campo | Ejemplo |
|---|---|
| Host name | `equipo-fisico-01` |
| Host groups | `Laboratorio autorizado` |
| Interface | Zabbix agent |
| IP | `MONITORED_HOST_IP` |
| Port | `10050` |
| Template | `Linux by Zabbix agent` o la plantilla del sistema |

Zabbix documenta `10050/TCP` para el agente, `10051/TCP` para el servidor, proxy o trapper, `80/TCP` para HTTP y `443/TCP` para HTTPS [8]. En redes no confiables, configura TLS con certificados o PSK y nunca publiques la clave en el repositorio [9].

## 5. Configurar alertas por correo

Zabbix utiliza un **medio** para entregar el correo y una **acción** para decidir cuándo enviarlo [6] [7].

1. Entra en **Alerts → Media types** y crea o edita el tipo **Email**.
2. Selecciona `Generic SMTP` o el proveedor recomendado.
3. Configura el servidor, puerto, remitente, seguridad de conexión y autenticación.
4. Utiliza OAuth, un relay SMTP o una contraseña de aplicación conforme a la política del proveedor.
5. En **Users → Users**, asigna el medio a un usuario o grupo autorizado y define `ALERT_EMAIL` como destinatario.
6. En **Alerts → Actions → Trigger actions**, crea una acción con condiciones como `Host group = Laboratorio autorizado` y `Trigger severity >= Warning`.
7. Añade la operación **Send message** al medio Email y habilita el mensaje de recuperación.

La configuración del proveedor debe realizarse sin publicar credenciales. La captura de apoyo muestra el flujo de generación de una contraseña de aplicación; el valor visible fue redactado antes de incorporarlo al repositorio:

![Configuración segura de una contraseña de aplicación](docs/assets/image12.png)

La cuenta de aplicación y sus datos identificables también deben sustituirse por valores de ejemplo antes de compartir el material:

![Cuenta de aplicación redactada](docs/assets/image13.png)

Para probar el medio, ve a **Alerts → Media types → Email → Test**, introduce un destinatario autorizado, asunto y mensaje, y pulsa **Test**. La documentación oficial describe esta prueba [6].

Un mensaje técnico y mínimo puede utilizar las siguientes macros:

```text
Asunto: [Zabbix][{TRIGGER.SEVERITY}] {HOST.NAME}: {TRIGGER.NAME}

Se detectó un evento técnico en un equipo autorizado.
Host: {HOST.NAME}
Problema: {TRIGGER.NAME}
Severidad: {TRIGGER.SEVERITY}
Estado: {TRIGGER.STATUS}
Hora: {EVENT.DATE} {EVENT.TIME}
Evento: {EVENT.ID}
```

La evidencia de recepción debe revisarse eliminando direcciones personales y cualquier secreto antes de publicarla:

![Correo de alarma recibido con datos redactados](docs/assets/image14.png)

Finalmente, la prueba del tipo de medio debe mostrar un envío exitoso al buzón autorizado:

![Prueba del medio Email en Zabbix](docs/assets/image15.png)

## 6. Alertas de tamaño de carpetas, tarjeta de red y Telegram

Esta sección se agrega a la secuencia existente. Los controles son determinísticos: el agente calcula el tamaño de carpetas autorizadas y consulta el estado operativo de interfaces Linux; Zabbix evalúa los umbrales y entrega las notificaciones por correo y Telegram. Zabbix 7.4 documenta la clave nativa `vfs.dir.size` para tamaños de directorios y `net.if.discovery` para descubrir interfaces [10] [11]. Para mantener un comportamiento explícito y portable en este laboratorio se incorporan dos `UserParameter` pequeños y auditables.

![Arquitectura de alertas de carpetas, tarjeta de red y Telegram](docs/assets/arquitectura-alertas-telegram.png)

### 6.1 Copiar e instalar los checks en el equipo monitoreado

Desde el equipo autorizado, copia la carpeta `monitoring/zabbix-agent` de este repositorio. No copies tokens ni contraseñas al repositorio. Ejecuta:

```bash
cd monitoring/zabbix-agent
sudo ./install.sh
```

El instalador coloca los scripts en `/usr/local/libexec/zabbix`, registra los parámetros en `/etc/zabbix/zabbix_agentd.d/gestion-red.conf`, aplica permisos restrictivos y reinicia el agente. Comprueba que los checks respondan:

```bash
sudo zabbix_agentd -t 'gestion.folder.size[/var/log]'
sudo zabbix_agentd -t 'gestion.if.status[eth0]'
```

Sustituye `eth0` por la interfaz real obtenida con `ip -br link`. El check de interfaz devuelve `1` cuando el estado operativo es `up` o `unknown`, y `0` cuando está `down`, `dormant`, `lowerlayerdown` o no existe. La interfaz `unknown` puede ser normal para `lo`; para una tarjeta física utiliza el nombre mostrado por `ip -br link`.

Si una carpeta no es legible por el usuario `zabbix`, concede únicamente el acceso mínimo al árbol autorizado. Por ejemplo, para una carpeta operativa específica:

```bash
sudo setfacl -m u:zabbix:rx /ruta
sudo setfacl -R -m u:zabbix:rx /ruta/carpeta-monitoreada
```

No otorgues permisos amplios sobre `/home`, directorios privados ni árboles que no formen parte del objetivo autorizado.

Los archivos agregados para esta fase son:

| Archivo | Propósito |
|---|---|
| `monitoring/zabbix-agent/dir_size.sh` | Devuelve el tamaño de una carpeta en bytes y valida que la ruta sea absoluta. |
| `monitoring/zabbix-agent/iface_status.sh` | Devuelve el estado operativo de una interfaz Linux como `1`, `0` o `2`. |
| `monitoring/zabbix-agent/gestion-red.conf` | Registra las claves `gestion.folder.size[*]` y `gestion.if.status[*]`. |
| `monitoring/zabbix-agent/install.sh` | Instala los archivos con permisos restrictivos y reinicia el agente. |
| `monitoring/zabbix-server/media_telegram.yaml` | Definición oficial del medio webhook de Telegram para importar en Zabbix 7.4. |

### 6.2 Crear los ítems en Zabbix

En **Data collection → Hosts → equipo-fisico-01 → Items**, crea un ítem por cada carpeta y por cada tarjeta que deban monitorearse. Usa tipo **Zabbix agent (active)** si el host trabaja en modalidad activa; si se configuró la modalidad pasiva, usa **Zabbix agent**. El tipo de dato de ambos checks es **Numeric (unsigned)**.

| Nombre sugerido | Key | Intervalo | Unidad |
|---|---|---:|---|
| Tamaño de `/var/log` | `gestion.folder.size[/var/log]` | `5m` | `B` |
| Estado de `eth0` | `gestion.if.status[eth0]` | `1m` | — |

Para varias carpetas, crea ítems separados, por ejemplo `gestion.folder.size[/var/lib/mysql]` o `gestion.folder.size[/opt/app-demo]`. Usa únicamente rutas que hayan sido autorizadas y evita colocar secretos dentro de los nombres o parámetros.

### 6.3 Crear los triggers y umbrales

En el host define macros de usuario para que los límites puedan cambiarse desde la interfaz sin editar scripts:

| Macro | Ejemplo | Significado |
|---|---:|---|
| `{$FOLDER_VAR_LOG_LIMIT}` | `5368709120` | 5 GiB expresados en bytes. |
| `{$FOLDER_MYSQL_LIMIT}` | `10737418240` | 10 GiB expresados en bytes. |
| `{$IFACE_DOWN_FOR}` | `3m` | Tiempo continuo antes de alertar por una interfaz caída. |

Crea un trigger de carpeta con esta expresión, ajustando el nombre del host y la macro al ítem correspondiente:

```text
last(/equipo-fisico-01/gestion.folder.size[/var/log])>{$FOLDER_VAR_LOG_LIMIT}
```

Usa severidad **Warning** o **Average** según la política del laboratorio. Para la tarjeta de red, crea el siguiente trigger:

```text
min(/equipo-fisico-01/gestion.if.status[eth0],{$IFACE_DOWN_FOR})=0
```

El primer trigger avisa cuando la carpeta supera el límite; el segundo exige que el estado haya permanecido en `0` durante el periodo configurado. Activa **OK event generation** para recibir la recuperación cuando la carpeta vuelva a estar por debajo del umbral o la interfaz vuelva a estar operativa.

### 6.4 Configurar Telegram en Zabbix 7.4

Zabbix mantiene un medio oficial de tipo webhook para Telegram 7.4, con soporte para notificaciones personales y de grupos [12]. El procedimiento es el siguiente:

1. En Telegram abre `@BotFather`, envía `/newbot`, completa el asistente y guarda el token en un gestor de secretos. El token nunca debe publicarse en GitHub, capturas ni logs.
2. Para una notificación personal, obtén el chat ID mediante `@myidbot` siguiendo el procedimiento documentado por Zabbix y envía `/start` al bot nuevo. Para un grupo, agrega el bot y obtiene el ID del grupo; los grupos suelen usar un ID negativo.
3. En Zabbix entra en **Alerts → Media types → Import** e importa `monitoring/zabbix-server/media_telegram.yaml`.
4. Abre el medio **Telegram**, activa **Enabled**, configura `api_token` con el token del bot y selecciona `html` o `markdown` como `api_parse_mode`. No guardes el token en archivos del repositorio.
5. En **Users → Users**, abre el usuario operativo, selecciona **Media → Add**, elige **Telegram** y coloca el chat ID en **Send to**. Si se usa un tópico de supergrupo, utiliza el formato `-1001234567890:2`.
6. Crea una acción separada en **Alerts → Actions → Trigger actions**. Filtra por el grupo `Laboratorio autorizado` y por severidad igual o mayor que **Warning**. Agrega la operación **Send message** al medio Telegram y activa el mensaje de recuperación.
7. Mantén la acción de Email existente separada de la acción de Telegram. Así se conserva el formato de correo actual y se evitan etiquetas Markdown o HTML sin interpretar en otros medios.

Un mensaje recomendado para la acción es:

```text
<b>{EVENT.SEVERITY}</b> · {HOST.NAME}
Problema: {TRIGGER.NAME}
Estado: {TRIGGER.STATUS}
Hora: {EVENT.DATE} {EVENT.TIME}
Evento: {EVENT.ID}
```

La guía oficial de Zabbix identifica `api_token`, `api_parse_mode` y `{ALERT.SENDTO}` como parámetros del webhook [12]. En consecuencia, el token se configura únicamente en la interfaz de Zabbix y el destinatario queda asociado al medio del usuario.

### 6.5 Pruebas controladas

Primero prueba el medio sin generar una falla: entra en **Alerts → Media types → Telegram → Test**, selecciona un usuario o destinatario autorizado y confirma que el mensaje llega al chat. Después valida los checks desde el equipo monitoreado:

```bash
sudo zabbix_agentd -t 'gestion.folder.size[/var/log]'
sudo zabbix_agentd -t 'gestion.if.status[eth0]'
```

Para una prueba de carpeta, reduce temporalmente el valor de `{$FOLDER_VAR_LOG_LIMIT}` por debajo del dato observado; espera el intervalo de actualización, comprueba el problema en **Monitoring → Problems** y confirma Email y Telegram. Restaura el umbral original y verifica el evento de recuperación.

Para probar una tarjeta de red, utiliza únicamente una interfaz de laboratorio y una ventana acordada. Desconecta el enlace o deshabilita temporalmente la interfaz, verifica el evento y restáurala de inmediato:

```bash
sudo nmcli device disconnect eth0
sudo nmcli device connect eth0
```

No realices esta prueba sobre la interfaz que mantiene la sesión remota ni sobre equipos o servicios no autorizados. Revisa **Reports → Action log** para correlacionar el evento, la recuperación y el envío a Telegram.

### 6.6 Solución de problemas

| Síntoma | Comprobación |
|---|---|
| El ítem aparece como unsupported | Revisa `sudo journalctl -u zabbix-agent -n 100 --no-pager`, la ruta absoluta, los permisos del árbol y que `gestion-red.conf` esté en `zabbix_agentd.d`. |
| El tamaño devuelve error de permisos | Comprueba el acceso efectivo del usuario `zabbix` y aplica ACL mínima solo al directorio autorizado. |
| La interfaz no genera recuperación | Confirma que el key usa el nombre real de `ip -br link` y que el trigger no tenga un periodo excesivo. |
| Telegram no recibe mensajes | Usa **Test** del medio, verifica el token, el chat ID y que el usuario haya enviado `/start` al bot. |
| Telegram devuelve error de formato | Cambia `api_parse_mode` a `html` y usa etiquetas válidas; mantén la acción separada del Email. |
| Se filtró un token | Revoca el token en `@BotFather`, genera uno nuevo y revisa el historial del repositorio antes de compartirlo. |

## 7. Prueba controlada y operación responsable

Realiza la prueba únicamente sobre un equipo autorizado y un servicio de laboratorio no crítico. Detén el servicio durante un intervalo acordado, verifica que aparece el problema en Zabbix y comprueba la recepción del correo. Después restáuralo y confirma el mensaje de recuperación. Revisa **Reports → Action log** para correlacionar el evento y la notificación.

Ejemplos apropiados de alarmas técnicas son los siguientes:

| Evento | Mensaje operativo |
|---|---|
| Agente sin respuesta | El equipo físico no responde durante 5 minutos. |
| Disco lleno | El punto de montaje `/` supera 85 %. |
| Servicio detenido | `app-demo.service` está detenido. |
| Proceso autorizado ausente | El proceso `app-demo` no está ejecutándose. |

No utilices Zabbix para registrar actividades personales de forma encubierta. Si una organización necesita un control de cumplimiento, documenta el propósito, la autorización, las métricas recogidas, los permisos de acceso y el tiempo de retención.

## Alcance y privacidad

El monitoreo debe limitarse a métricas técnicas necesarias para el objetivo definido: disponibilidad, CPU, memoria, almacenamiento, estado de servicios y procesos expresamente autorizados. No deben recopilarse pulsaciones, contenido de archivos, capturas de pantalla, historial privado ni interpretaciones sobre la conducta de una persona.

Antes de publicar o compartir este repositorio, revisa que no contenga contraseñas de MariaDB, contraseñas de aplicación SMTP, tokens, claves TLS/PSK, archivos `.env`, logs ni direcciones privadas. Si una credencial del material original fue utilizada, revócala y genera una nueva.



## Referencias

 1.https://www.virtualbox.org/manual/ch06.html "Oracle VirtualBox User Manual — Virtual Networking"
 
 2.https://www.centos.org/download/ "The CentOS Project — Download"
 
 3.https://www.zabbix.com/download?zabbix=7.4&os_distribution=centos&os_version=10&components=server_frontend_agent&db=mysql&ws=apache "Zabbix — Download and install Zabbix 7.4 for CentOS 10"

 4.https://www.zabbix.com/documentation/7.4/en/manual/installation/install_from_packages "Zabbix 7.4 — Installation from packages"
 
 5.https://www.zabbix.com/documentation/7.4/en/manual/appendix/config/zabbix_agentd "Zabbix 7.4 — Zabbix agent (UNIX)"
 
 6.https://www.zabbix.com/documentation/7.4/en/manual/config/notifications/media/email "Zabbix 7.4 — Email media type"
 
 7.https://www.zabbix.com/documentation/7.4/en/manual/config/notifications "Zabbix 7.4 — Notifications upon events"
 
 8.https://www.zabbix.com/documentation/7.4/en/manual/installation/requirements "Zabbix 7.4 — Requirements and default ports"
 
 9.https://www.zabbix.com/documentation/7.4/en/manual/encryption "Zabbix 7.4 — Encryption"


10. https://www.zabbix.com/documentation/7.4/en/manual/config/items/itemtypes/zabbix_agent "Zabbix 7.4 — Zabbix agent item keys"

11. https://www.zabbix.com/documentation/7.4/en/manual/discovery/low_level_discovery/examples/network_interfaces "Zabbix 7.4 — Discovery of network interfaces"

12. https://www.zabbix.com/integrations/telegram "Zabbix — Telegram webhook integration"


---
# Windows 11 + SNMP + Zabbix 7.4

[![Windows 11](https://img.shields.io/badge/OS-Windows%2011-0078D4?logo=windows&logoColor=white )](https://www.microsoft.com/windows/windows-11 )
[![CentOS Stream 10](https://img.shields.io/badge/Servidor-CentOS%20Stream%2010-262577?logo=centos&logoColor=white )](https://www.centos.org/stream/ )
[![Zabbix 7.4](https://img.shields.io/badge/Zabbix-7.4-d40000?logo=zabbix&logoColor=white )](https://www.zabbix.com/ )
[![SNMP v2c](https://img.shields.io/badge/Protocolo-SNMP%20v2c-1f6feb )](#seguridad-y-alcance)
[![Monitoreo autorizado](https://img.shields.io/badge/monitoreo-autorizado-1f6feb )](#alcance-y-privacidad)

## Laboratorio de monitorización de Windows sin agente Zabbix

Este proyecto documenta la incorporación de un equipo **Windows 11 24H2** a **Zabbix 7.4.13**, utilizando el servicio SNMP incluido como característica opcional de Windows y sin instalar el agente nativo de Zabbix en el equipo monitorizado.

> **Objetivo del laboratorio:** observar de forma autorizada la disponibilidad, la identidad del sistema, las interfaces de red y un conjunto básico de recursos de Windows mediante consultas SNMPv2c.

> **Importante:** este procedimiento no incluye keylogging, captura encubierta de pantalla, lectura de archivos privados, interceptación de comunicaciones ni vigilancia secreta. Solo se consultan los OIDs expuestos por el agente SNMP del equipo autorizado.

## Recorrido rápido

| Fase | Resultado |
|---|---|
| 01 · Preparar | Servidor Zabbix operativo, con soporte SNMP y herramientas `net-snmp` |
| 02 · Instalar | Servicio SNMP habilitado en Windows 11 |
| 03 · Configurar | Comunidad de solo lectura, servicios del agente y firewall UDP/161 |
| 04 · Validar | Respuesta directa confirmada con `snmpwalk` |
| 05 · Conectar | Equipo creado en Zabbix con interfaz SNMP y plantilla `Windows by SNMP` |
| 06 · Restringir | Acceso SNMP limitado a los orígenes autorizados |
| 07 · Operar | Verificación de datos, mantenimiento y resolución de incidencias |

## Arquitectura

```text
┌──────────────────────────────┐       UDP/161        ┌──────────────────────────────┐
│ Windows 11 24H2              │ <─────────────────── │ VM CentOS Stream 10          │
│ DESKTOP-FK5S8HJ              │   Consultas SNMP     │ Zabbix Server 7.4.13         │
│ Servicio SNMP                 │ ──────────────────> │ MariaDB + Apache + PHP-FPM   │
│ MIBs system / interfaces     │     Respuestas       │ Plantilla Windows by SNMP    │
└──────────────────────────────┘                     └──────────────────────────────┘
              │                                                   │
              │ Firewall: UDP/161 entrante                        │
              │                                                   │
              └────────────── Red autorizada / NAT VMware ────────┘
```

El servidor Zabbix origina las consultas hacia el equipo Windows. Por ello, la regla principal del firewall se configura en Windows como tráfico **entrante** al puerto UDP/161. No es necesario abrir un puerto SNMP entrante en el servidor Zabbix para este escenario de consulta.

Durante la implementación, el servidor utilizó la dirección `192.168.159.129` y el equipo Windows la dirección `172.17.47.225`. La comunicación atravesó NAT de VMware, aspecto que debe considerarse al restringir los hosts autorizados.

## Requisitos sugeridos

| Componente | Valor utilizado |
|---|---:|
| Servidor de monitorización | Zabbix 7.4.13 |
| Sistema del servidor | CentOS 10 |
| Dirección del servidor | `192.168.159.129` |
| Equipo monitorizado | Windows 11 24H2, build 26200 |
| Nombre del equipo | `DESKTOP-FK5S8HJ` |
| Dirección del equipo | `172.17.47.225` |
| Protocolo | SNMPv2c |
| Puerto | UDP/161 |
| Comunidad de ejemplo | `zbx_monitor` |
| Plantilla | `Windows by SNMP` |
| Permisos de la comunidad | Solo lectura |
| Acceso requerido | Root en CentOS y administrador local en Windows |
| Red | Conectividad entre el servidor y el equipo Windows |

Se recomienda utilizar una dirección IP estática o una reserva DHCP para el equipo monitorizado. Si la dirección cambia, Zabbix puede dejar de recolectar datos aunque el servicio SNMP continúe funcionando.

## 1. Preparar el servidor Zabbix

Antes de modificar Windows, comprueba que Zabbix y sus servicios asociados están activos:

```bash
systemctl status zabbix-server httpd php-fpm mariadb --no-pager
zabbix_server -V | head -3
```

La implementación de referencia utilizó Zabbix 7.4.13. Verifica también que el servidor fue compilado con soporte SNMP:

```bash
systemctl status zabbix-server --no-pager | grep "snmp poller"
```

La salida debe incluir un proceso similar a:

```text
/usr/sbin/zabbix_server: snmp poller #1 [got 0 values, queued 0 in 5 sec, awaiting 0]
```

Zabbix requiere soporte SNMP en el servidor para ejecutar las comprobaciones. La documentación oficial indica que la compilación debe incluir soporte para `net-snmp` [1].

Instala las herramientas de diagnóstico:

```bash
sudo dnf install -y net-snmp-utils
which snmpwalk snmpget
```

Estas herramientas no son necesarias para que el daemon de Zabbix funcione, pero permiten validar el agente de Windows sin involucrar todavía a la interfaz de Zabbix. En distribuciones basadas en Debian o Ubuntu, el paquete equivalente es:

```bash
sudo apt install -y snmp
```

Comprueba la conectividad básica:

```bash
ping -c3 172.17.47.225
ip route
```

No abras puertos SNMP entrantes en el firewall del servidor solo para realizar esta integración. Las consultas se originan en el servidor y el tráfico de respuesta pertenece a una conexión ya iniciada.

## 2. Instalar el servicio SNMP en Windows 11

Todos los comandos de esta sección deben ejecutarse desde **PowerShell abierto con Ejecutar como administrador**. Tener una cuenta perteneciente al grupo de administradores no garantiza que una consola normal tenga el token elevado necesario.

### 2.1 Consultar la característica opcional

```powershell
Get-WindowsCapability -Online -Name "SNMP*"
```

Busca la característica `SNMP.Client~~~~0.0.1.0`. El estado inicial esperado es `NotPresent`.

No asumas el identificador sin consultar la salida. El nombre puede variar entre compilaciones de Windows.

### 2.2 Instalar SNMP

```powershell
Add-WindowsCapability -Online -Name "SNMP.Client~~~~0.0.1.0"
```

La instalación puede tardar varios minutos y la barra de progreso puede permanecer quieta durante periodos prolongados. No cierres la consola mientras el comando siga ejecutándose.

Si el título de la consola muestra el prefijo **Seleccionar**, PowerShell puede haber entrado en modo de selección de texto después de un clic accidental. Pulsa `Esc` o haz clic derecho para liberar la consola.

Puedes abrir una segunda ventana para comprobar el estado sin interrumpir la instalación:

```powershell
Get-WindowsCapability -Online -Name "SNMP*" | Select Name,State
Get-Process TiWorker,TrustedInstaller -ErrorAction SilentlyContinue | Select Name,CPU
```

Si el consumo de CPU de `TiWorker` cambia entre consultas, la instalación puede continuar progresando aunque la barra no avance de forma uniforme.

### 2.3 Confirmar el servicio

```powershell
Get-WindowsCapability -Online -Name "SNMP*" | Select Name,State
Get-Service SNMP | Select Name,Status,StartType
```

El resultado esperado es:

```text
Name        : SNMP
Status      : Running
StartType   : Automatic
```

`SNMP` y `SNMPTrap` no son equivalentes. `SNMP` es el servicio que responde a las consultas; `SNMPTrap` se utiliza para recibir notificaciones asíncronas.

Si la instalación indica `RestartNeeded : True`, reinicia Windows antes de continuar.

Microsoft documenta la instalación de `SNMP.Client` mediante `Add-WindowsCapability` y considera SNMP una característica obsoleta, aunque todavía disponible en este escenario [3].

## 3. Configurar la comunidad y el agente

La configuración del servicio se realiza desde `services.msc`, no desde la aplicación moderna de Configuración de Windows 11.

Abre la consola:

```powershell
services.msc
```

Localiza **Servicio SNMP**, abre sus propiedades y configura las pestañas **Seguridad** y **Agente**.

### 3.1 Definir la comunidad

En **Seguridad → Nombres de comunidad aceptados**, agrega:

| Campo | Valor |
|---|---|
| Nombre de comunidad | `zbx_monitor` |
| Derechos | `SOLO LECTURA` |

Durante la primera validación marca temporalmente **Aceptar paquetes SNMP de cualquier host**. Esta apertura solo debe permanecer activa hasta comprobar la respuesta de `snmpwalk` y la recolección en Zabbix.

No utilices `public` como comunidad. En SNMPv2c, la comunidad funciona como una credencial transmitida sin cifrado. El nombre distingue mayúsculas y minúsculas, por lo que debe coincidir exactamente con la macro configurada en Zabbix.

### 3.2 Seleccionar los servicios del agente

En la pestaña **Agente**, marca los cinco servicios disponibles:

- Físico.
- Aplicaciones.
- Vínculo de datos y subred.
- Internet.
- De extremo a extremo.

Cada selección habilita una rama distinta de OIDs. Si no se marcan, el agente puede responder con un conjunto reducido de datos y la plantilla de Zabbix puede dejar elementos sin soporte.

### 3.3 Reiniciar el servicio

Después de aplicar los cambios, reinicia el servicio:

```powershell
Restart-Service SNMP
```

El reinicio es obligatorio porque el agente SNMP de Windows lee la comunidad y los hosts permitidos durante el arranque del servicio.

## 4. Verificar el firewall y validar SNMP

### 4.1 Comprobar las reglas de Windows

```powershell
Get-NetFirewallRule -DisplayName "*SNMP*" | Select DisplayName,Enabled,Profile
```

Las reglas relevantes son las de **Servicio SNMP** para UDP entrante. Las reglas de captura de SNMP corresponden a traps y no son necesarias para esta integración basada en consultas.

Si no existen reglas adecuadas, crea una regla específica:

```powershell
New-NetFirewallRule -DisplayName "SNMP-In-UDP161" `
  -Direction Inbound `
  -Protocol UDP `
  -LocalPort 161 `
  -Action Allow `
  -Profile Any
```

El perfil `Any` evita que la regla deje de aplicar si Windows clasifica la conexión como pública, algo habitual en algunas conexiones Wi-Fi.

### 4.2 Ejecutar `snmpwalk` desde CentOS

La herramienta debe ejecutarse en el servidor Linux, no en Windows:

```bash
snmpwalk -v2c -c zbx_monitor 172.17.47.225 .1.3.6.1.2.1.1
```

El comando consulta el grupo `system` y debe devolver información similar a la siguiente:

| OID | Resultado de referencia | Confirmación |
|---|---|---|
| `sysDescr` | `Windows Version 6.3 (Build 26200 )` | El agente responde. El valor 6.3 es heredado y no representa necesariamente la versión real de Windows. |
| `sysName` | `DESKTOP-FK5S8HJ` | La consulta corresponde al equipo previsto. |
| `sysServices` | `79` | Los cinco servicios del agente están activos. |
| `sysUpTime` | Un valor bajo tras reiniciar | La configuración recién aplicada está respondiendo. |

Si la consulta expira, revisa en este orden:

1. El servicio `SNMP` está en ejecución.
2. La comunidad coincide exactamente.
3. El equipo está autorizado temporalmente como origen.
4. La regla de firewall permite UDP/161.
5. Existe conectividad entre las direcciones y rutas utilizadas.

No continúes con la configuración de Zabbix hasta obtener respuesta directa con `snmpwalk`.

Puedes consultar otras ramas del árbol SNMP:

```bash
# Recorrido completo
snmpwalk -v2c -c zbx_monitor 172.17.47.225

# Interfaces de red
snmpwalk -v2c -c zbx_monitor 172.17.47.225 .1.3.6.1.2.1.2

# Nombre del equipo
snmpget -v2c -c zbx_monitor 172.17.47.225 .1.3.6.1.2.1.1.5.0
```

Zabbix recomienda utilizar `snmpwalk` para localizar y verificar los OIDs disponibles antes de crear elementos SNMP [1].

## 5. Registrar el equipo en Zabbix

El alta se realiza desde **Recopilación de datos → Equipos → Crear equipo**. La sección **Monitorización → Equipos** es una vista de consulta y no contiene el formulario de creación.

### 5.1 Crear el host

Utiliza estos valores:

| Campo | Valor |
|---|---|
| Nombre del equipo | `Win11-SNMP` |
| Grupo | `Windows servers` |
| Plantilla | `Windows by SNMP` |
| Interfaz | SNMP, no Agente |
| Dirección IP | `172.17.47.225` |
| Puerto | `161` |
| Versión SNMP | `SNMPv2` |
| Comunidad | `{$SNMP_COMMUNITY}` |

Si el grupo `Windows servers` no existe, escribe el nombre y selecciona la opción `(nuevo)` ofrecida por Zabbix.

No confundas `Windows by SNMP` con `Windows by Zabbix agent`. La primera plantilla consulta mediante SNMP; la segunda requiere el agente nativo de Zabbix.

### 5.2 Definir la macro de comunidad

En la pestaña **Macros** del host define:

| Macro | Valor |
|---|---|
| `{$SNMP_COMMUNITY}` | `zbx_monitor` |

Este es un paso crítico. Si la macro no se define, la plantilla puede utilizar `public` como valor heredado. En ese caso, `snmpwalk` funcionará, pero los elementos de Zabbix fallarán por utilizar una comunidad diferente.

El uso de una macro facilita cambiar la comunidad sin modificar la interfaz SNMP y permite reutilizar la plantilla con diferentes credenciales.

### 5.3 Confirmar la recolección

Guarda el host y espera entre dos y tres minutos. Comprueba lo siguiente:

- El indicador de la interfaz SNMP aparece en verde.
- El host muestra la plantilla `Windows by SNMP`.
- Los elementos comienzan a recibir datos.
- El número de elementos activos se aproxima a los **68** observados en la implementación de referencia.

Si un elemento aparece como **No soportado**, abre **Recopilación de datos → Equipos → Win11-SNMP → Elementos** y revisa el mensaje de error y el OID correspondiente.

Las plantillas preconfiguradas de Zabbix están diseñadas para simplificar la incorporación de objetivos de monitorización [2].

## 6. Restringir el acceso SNMP

Una vez confirmada la recolección, cierra la apertura temporal de **Aceptar paquetes SNMP de cualquier host**.

En **services.msc → Servicio SNMP → Seguridad**, selecciona **Aceptar paquetes SNMP de estos hosts** y agrega únicamente los orígenes autorizados.

En una instalación directa puede ser suficiente la dirección del servidor Zabbix. En la implementación documentada existía NAT de VMware, por lo que se consideraron estas direcciones:

- `192.168.159.129`, dirección del servidor Zabbix.
- `172.17.47.225`, dirección de salida considerada en la traducción NAT.

Reinicia el servicio:

```powershell
Restart-Service SNMP
```

Vuelve a ejecutar la validación desde CentOS:

```bash
snmpwalk -v2c -c zbx_monitor 172.17.47.225 .1.3.6.1.2.1.1
```

La consulta debe continuar respondiendo y Zabbix debe mantener el indicador SNMP en verde.

`netstat` solo confirma que el socket UDP/161 está escuchando; no identifica de forma fiable el origen de las consultas UDP:

```powershell
netstat -an | findstr ":161"
```

Si no se conoce la dirección real de origen, identifica el tráfico con una captura filtrada por `udp.port == 161` o utiliza el registro de autenticación SNMP conforme a la política del entorno.

## 7. Prueba controlada y operación responsable

La implementación de referencia finalizó con los siguientes resultados:

| Prueba | Resultado |
|---|---|
| Servicio SNMP de Windows | En ejecución y con inicio automático |
| Comunidad | `zbx_monitor`, solo lectura |
| Respuesta `snmpwalk` | Correcta |
| `sysName` | `DESKTOP-FK5S8HJ` |
| `sysServices` | `79` |
| Plantilla | `Windows by SNMP` |
| Interfaz Zabbix | Verde |
| Elementos activos | 68 |
| Restricción por origen | Aplicada y validada |

Para una operación estable:

- Configura una reserva DHCP por dirección MAC o una dirección IP estática.
- Considera que Wi-Fi puede producir pérdidas de paquetes, suspensiones y cambios de punto de acceso.
- Ajusta el tiempo de espera SNMP si la red lo requiere.
- Configura varios fallos consecutivos antes de generar una alerta de indisponibilidad.
- Define periodos de mantenimiento cuando el equipo se apague fuera del horario operativo.
- Utiliza conexión cableada cuando sea posible.
- Revisa periódicamente los elementos en estado **No soportado**.
- Mantén la comunidad fuera de repositorios y registros compartidos públicamente.

### Solución de problemas

| Síntoma | Causa probable | Acción |
|---|---|---|
| “La operación solicitada requiere elevación” | PowerShell no se abrió con privilegios elevados. | Abrir PowerShell con **Ejecutar como administrador**. |
| La instalación parece congelada | Consola en modo de selección de texto. | Pulsar `Esc` o hacer clic derecho; verificar desde una segunda consola. |
| Solo aparece `SNMPTrap` | La característica `SNMP.Client` no está instalada. | Instalar SNMP y comprobar el servicio `SNMP`. |
| `snmpwalk` no se reconoce | Se ejecutó en Windows o no se instaló `net-snmp-utils`. | Ejecutarlo en CentOS después de instalar las herramientas. |
| `snmpwalk` expira | Servicio detenido, comunidad incorrecta o UDP/161 bloqueado. | Revisar servicio, comunidad, firewall y conectividad en ese orden. |
| No existe el botón “Crear equipo” | Se está en la vista de monitorización. | Ir a **Recopilación de datos → Equipos**. |
| Zabbix no recibe datos pero `snmpwalk` funciona | Comunidad de la interfaz diferente a la del agente. | Definir `{$SNMP_COMMUNITY}` con `zbx_monitor`. |
| La consulta falla tras restringir hosts | El tráfico llega desde una dirección traducida por NAT. | Autorizar el origen real o capturar el tráfico UDP/161. |
| Error `0x800f0954` durante la instalación | WSUS no distribuye la característica opcional. | Usar temporalmente Windows Update o un medio de instalación autorizado. |

## Seguridad y alcance

SNMPv2c no cifra la comunidad ni el contenido de las consultas. Las medidas aplicadas reducen la exposición, pero no eliminan el riesgo de captura en tránsito:

1. Se sustituyó la comunidad predeterminada `public`.
2. Se asignaron permisos de solo lectura.
3. Se permitió cualquier host únicamente durante la validación inicial.
4. Se restringió el acceso a orígenes autorizados después de confirmar la funcionalidad.
5. Se evitó abrir puertos innecesarios en el servidor Zabbix.

El agente SNMP de Windows expone un conjunto reducido de MIBs. La solución es apropiada para métricas básicas de sistema, red, disponibilidad y recursos expuestos por el agente. No sustituye al agente nativo de Zabbix cuando se requiere monitorizar con detalle servicios de Windows, procesos, eventos, rendimiento avanzado de discos o comprobaciones personalizadas.

Microsoft mantiene SNMP como una característica obsoleta y recomienda evaluar tecnologías de administración como CIM/WMI para nuevos diseños [3]. Antes de desplegar esta solución a gran escala, valida la compatibilidad con las futuras versiones de Windows y las políticas de seguridad de la organización.

## Alcance y privacidad

Este procedimiento está limitado a equipos y redes con autorización expresa. El monitoreo debe configurarse con el mínimo privilegio necesario y únicamente para fines de disponibilidad, capacidad, inventario técnico y diagnóstico operativo.

No deben incorporarse al repositorio comunidades reales, contraseñas, tokens, capturas con datos personales, direcciones innecesarias ni registros que expongan información sensible. Sustituye los valores del laboratorio por variables o secretos gestionados fuera del repositorio antes de publicar el documento.

## Referencias

[1]: https://www.zabbix.com/documentation/current/en/manual/config/items/itemtypes/snmp "Zabbix 7.4 — SNMP agent monitoring"

[2]: https://www.zabbix.com/documentation/7.4/en/manual/config/templates_out_of_the_box "Zabbix 7.4 — Templates out of the box"

[3]: https://learn.microsoft.com/en-us/troubleshoot/windows-client/networking/cannot-install-snmp-wmisnmpprovider "Microsoft Learn — Can't add the SNMP and WMI SNMP Provider features in Windows 10 or Windows 11"

[4]: ./ZABBIX.pdf "Informe de implementación de monitorización SNMP de Windows 11 con Zabbix"
