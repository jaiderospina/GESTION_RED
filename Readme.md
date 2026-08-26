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

## 6. Prueba controlada y operación responsable

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

[1]: https://www.virtualbox.org/manual/ch06.html "Oracle VirtualBox User Manual — Virtual Networking"
[2]: https://www.centos.org/download/ "The CentOS Project — Download"
[3]: https://www.zabbix.com/download?zabbix=7.4&os_distribution=centos&os_version=10&components=server_frontend_agent&db=mysql&ws=apache "Zabbix — Download and install Zabbix 7.4 for CentOS 10"
[4]: https://www.zabbix.com/documentation/7.4/en/manual/installation/install_from_packages "Zabbix 7.4 — Installation from packages"
[5]: https://www.zabbix.com/documentation/7.4/en/manual/appendix/config/zabbix_agentd "Zabbix 7.4 — Zabbix agent (UNIX)"
[6]: https://www.zabbix.com/documentation/7.4/en/manual/config/notifications/media/email "Zabbix 7.4 — Email media type"
[7]: https://www.zabbix.com/documentation/7.4/en/manual/config/notifications "Zabbix 7.4 — Notifications upon events"
[8]: https://www.zabbix.com/documentation/7.4/en/manual/installation/requirements "Zabbix 7.4 — Requirements and default ports"
[9]: https://www.zabbix.com/documentation/7.4/en/manual/encryption "Zabbix 7.4 — Encryption"
