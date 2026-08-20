# CentOS Stream 10 + Zabbix 7.4

## Monitoreo autorizado y alertas por correo

Este documento explica cómo instalar **CentOS Stream 10** en una máquina virtual, desplegar **Zabbix 7.4** con MariaDB/MySQL y Apache, conectar un equipo físico mediante el agente Zabbix y enviar alertas por correo electrónico.

> **Alcance responsable.** Zabbix debe utilizarse para supervisar infraestructura, servicios, disponibilidad, rendimiento y eventos técnicos en equipos autorizados. Este procedimiento no incluye keylogging, captura encubierta de pantalla, lectura de archivos privados, interceptación de comunicaciones ni vigilancia secreta. El monitoreo debe realizarse con autorización, transparencia, finalidad definida y la mínima cantidad de datos necesaria.

## Arquitectura de referencia

```text
Equipo físico autorizado                 Máquina virtual CentOS Stream 10
+-------------------------+              +-------------------------------+
| Zabbix agent            | -- LAN ----> | Zabbix Server 7.4             |
| CPU, RAM, disco,        |              | MariaDB/MySQL + Apache        |
| servicios y procesos    |              | Panel web: /zabbix            |
+-------------------------+              +---------------+---------------+
                                                         |
                                                         | SMTP seguro
                                                         v
                                                  Buzón de alertas
```

Para esta práctica se recomienda un **adaptador puente** en VirtualBox: la VM recibe una dirección propia en la red autorizada y puede comunicarse directamente con el equipo físico. VirtualBox documenta que NAT es adecuado para salida de la VM, pero que la máquina queda aislada e inaccesible desde el exterior salvo que se configure redirección de puertos [1].

## Requisitos sugeridos

| Componente | Valor de laboratorio |
|---|---:|
| CPU de la VM | 2 vCPU |
| RAM | 4 GiB |
| Disco | 30–40 GiB |
| Sistema | CentOS Stream 10 x86_64 |
| Red | Adaptador puente o red de laboratorio equivalente |
| Base de datos | MariaDB/MySQL |
| Frontend | Apache + PHP |
| Buzón | Cuenta técnica autorizada para alertas |

CentOS publica las imágenes de Stream 10 y recomienda verificar la ISO mediante SHA-256 antes de instalarla [2]. Descarga la ISO desde un espejo oficial y comprueba su suma:

```bash
sha256sum CentOS-Stream-10-*.iso
```

En Windows PowerShell:

```powershell
Get-FileHash .\CentOS-Stream-10-*.iso -Algorithm SHA256
```

## 1. Crear la máquina virtual

En VirtualBox selecciona **Nueva**, asigna 2 vCPU, 4 GiB de RAM y un disco dinámico de 30–40 GiB. En **Configuración → Red → Adaptador 1**, activa **Adaptador puente** y selecciona la interfaz de red conectada a la LAN autorizada.

Inicia la ISO, selecciona **Install CentOS Stream 10**, configura el disco virtual, la zona horaria, una cuenta administrativa y un nombre como `zabbix-server`. Después del primer inicio:

```bash
sudo dnf update -y
sudo hostnamectl set-hostname zabbix-server
hostnamectl
ip -br address
ip route
cat /etc/centos-release
```

Anota la IP de la VM como `ZABBIX_SERVER_IP`. Usa una IP reservada por el administrador de red o una concesión DHCP estable.

## 2. Instalar MariaDB, Apache, PHP y Zabbix

La receta oficial de Zabbix 7.4 para CentOS 10, MySQL y Apache utiliza el repositorio oficial y los paquetes siguientes [3] [4]:

```bash
sudo dnf install -y mariadb-server httpd php php-fpm php-mysqlnd \
  php-gd php-xml php-bcmath php-mbstring php-ldap php-json php-opcache
sudo systemctl enable --now mariadb httpd php-fpm

sudo rpm -Uvh https://repo.zabbix.com/zabbix/7.4/release/centos/10/noarch/zabbix-release-latest-7.4.el10.noarch.rpm
sudo dnf clean all
sudo dnf install -y zabbix-server-mysql zabbix-web-mysql \
  zabbix-apache-conf zabbix-sql-scripts zabbix-selinux-policy zabbix-agent
```

Comprueba las versiones y servicios:

```bash
rpm -q zabbix-server-mysql zabbix-web-mysql zabbix-agent mariadb-server
sudo systemctl --no-pager --full status mariadb httpd php-fpm
```

## 3. Crear la base de datos de Zabbix

Ejecuta `sudo mariadb` y escribe las siguientes sentencias. El texto que aparece como prompt, por ejemplo `MariaDB [(none)]>`, no forma parte del comando.

```sql
CREATE DATABASE zabbix CHARACTER SET utf8mb4 COLLATE utf8mb4_bin;
CREATE USER 'zabbix'@'localhost' IDENTIFIED BY 'CAMBIAR_POR_UN_SECRETO_LARGO';
GRANT ALL PRIVILEGES ON zabbix.* TO 'zabbix'@'localhost';
SET GLOBAL log_bin_trust_function_creators = 1;
FLUSH PRIVILEGES;
EXIT;
```

Importa el esquema inicial. El cliente solicitará la contraseña de la cuenta `zabbix`:

```bash
zcat /usr/share/zabbix/sql-scripts/mysql/server.sql.gz \
  | mysql --default-character-set=utf8mb4 -uzabbix -p zabbix
```

Después de la importación, desactiva el ajuste temporal:

```bash
sudo mariadb -e "SET GLOBAL log_bin_trust_function_creators = 0;"
```

Edita `/etc/zabbix/zabbix_server.conf` y configura la contraseña de la base:

```ini
DBName=zabbix
DBUser=zabbix
DBPassword=CAMBIAR_POR_UN_SECRETO_LARGO
```

Protege el archivo y no lo publiques:

```bash
sudo chown root:zabbix /etc/zabbix/zabbix_server.conf
sudo chmod 640 /etc/zabbix/zabbix_server.conf
```

## 4. Firewall, SELinux y servicios

No desactives SELinux como solución general. Comprueba su estado y revisa los rechazos antes de realizar cambios:

```bash
getenforce
sudo ausearch -m AVC -ts recent 2>/dev/null | tail -n 20 || true
```

Para un laboratorio que utiliza comprobaciones activas desde el equipo físico, abre HTTP y el puerto del servidor Zabbix:

```bash
sudo firewall-cmd --permanent --add-service=http
sudo firewall-cmd --permanent --add-port=10051/tcp
sudo firewall-cmd --reload
sudo firewall-cmd --list-all
```

Activa los servicios y verifica los puertos:

```bash
sudo systemctl enable --now zabbix-server zabbix-agent httpd php-fpm
sudo systemctl --no-pager --full status zabbix-server zabbix-agent httpd php-fpm
sudo ss -lntp | grep -E ':80|:10051|:10050' || true
```

Abre `http://ZABBIX_SERVER_IP/zabbix` y completa el asistente web con:

| Campo | Valor |
|---|---|
| Database type | MySQL |
| Database host | `localhost` |
| Database name | `zabbix` |
| User | `zabbix` |
| Password | La contraseña creada para la base |
| Zabbix server name | Un nombre descriptivo, por ejemplo `Global` |
| Time zone | La misma configurada en CentOS |

## 5. Conectar el equipo físico

La documentación de Zabbix define los parámetros `Hostname`, `Server` y `ServerActive` del agente UNIX [5]. El nombre configurado en `Hostname` debe coincidir con el nombre del host creado en Zabbix.

### Opción recomendada: agente activo

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

Activa el agente:

```bash
sudo systemctl enable --now zabbix-agent
sudo systemctl --no-pager --full status zabbix-agent
sudo journalctl -u zabbix-agent -b --no-pager -n 50
```

Desde el equipo físico prueba la salida hacia la VM:

```bash
nc -vz ZABBIX_SERVER_IP 10051
```

### Opción pasiva

En la modalidad pasiva el servidor consulta al agente por `TCP/10050`. Configura:

```ini
Server=ZABBIX_SERVER_IP
ServerActive=ZABBIX_SERVER_IP:10051
Hostname=MONITORED_HOST_NAME
ListenPort=10050
```

Abre el puerto en el firewall del equipo físico únicamente para la IP del servidor:

```bash
sudo firewall-cmd --permanent \
  --add-rich-rule='rule family="ipv4" source address="ZABBIX_SERVER_IP/32" port port="10050" protocol="tcp" accept'
sudo firewall-cmd --reload
```

Desde la VM prueba la conexión:

```bash
nc -vz MONITORED_HOST_IP 10050
```

### Crear el host en la interfaz web

En Zabbix selecciona **Data collection → Hosts → Create host** y configura:

| Campo | Ejemplo |
|---|---|
| Host name | `equipo-fisico-01` |
| Host groups | `Laboratorio autorizado` |
| Interface | Zabbix agent |
| IP | `MONITORED_HOST_IP` |
| Port | `10050` |
| Template | `Linux by Zabbix agent` o la plantilla del sistema |

Zabbix documenta `10050/TCP` para el agente, `10051/TCP` para el servidor/proxy/trapper, `80/TCP` para HTTP y `443/TCP` para HTTPS [8]. En redes no confiables, configura TLS con certificados o PSK; no publiques la clave en Git [9].

## 6. Configurar alertas por correo

Zabbix utiliza un **medio** para entregar el correo y una **acción** para decidir cuándo enviarlo [6] [7].

1. Entra en **Alerts → Media types** y crea o edita el tipo **Email**.
2. Selecciona `Generic SMTP` o el proveedor recomendado.
3. Configura el servidor, puerto, remitente, seguridad de conexión y autenticación.
4. Usa OAuth, relay SMTP o una contraseña de aplicación según la política del proveedor.
5. En **Users → Users**, asigna el medio a un usuario o grupo autorizado y define `ALERT_EMAIL` como destinatario.
6. En **Alerts → Actions → Trigger actions**, crea una acción con una condición como `Host group = Laboratorio autorizado` y `Trigger severity >= Warning`.
7. Añade la operación **Send message** al medio Email y habilita el mensaje de recuperación.

Para probar el medio, ve a **Alerts → Media types → Email → Test**, introduce un destinatario autorizado, asunto y mensaje, y pulsa **Test**. La documentación oficial describe esta prueba [6].

Usa un mensaje técnico y mínimo:

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

No incluyas contraseñas, contenido de archivos, pulsaciones, capturas de pantalla, historial privado ni interpretaciones sobre la conducta de una persona.

## 7. Prueba controlada de alarma

Realiza la prueba únicamente sobre un equipo autorizado y un servicio de laboratorio no crítico. Detén el servicio durante un intervalo acordado, verifica que aparece el problema en Zabbix y comprueba la recepción del correo. Después restáuralo y confirma el mensaje de recuperación. Revisa **Reports → Action log** para correlacionar el evento y la notificación.

Ejemplos apropiados de alarmas:

| Evento | Mensaje técnico |
|---|---|
| Agente sin respuesta | El equipo físico no responde durante 5 minutos. |
| Disco lleno | El punto de montaje `/` supera 85 %. |
| Servicio detenido | `app-demo.service` está detenido. |
| Proceso autorizado ausente | El proceso `app-demo` no está ejecutándose. |

No utilices Zabbix para registrar actividades personales de forma encubierta. Si una organización necesita un control de cumplimiento, documenta el propósito, la autorización, las métricas recogidas, el acceso y el tiempo de retención.

## Seguridad antes de publicar

Nunca subas contraseñas de MariaDB, contraseñas de aplicación SMTP, tokens, claves TLS/PSK, archivos `.env`, direcciones privadas ni logs sin revisar. Si una credencial del material original fue utilizada, revócala y genera una nueva. No expongas el panel ni el agente directamente a Internet.

## Referencias

[1]: https://www.virtualbox.org/manual/ch06.html "Oracle VirtualBox User Manual — Virtual Networking"
[2]: https://www.centos.org/download/ "The CentOS Project — Download"
[3]: https://www.zabbix.com/download?zabbix=7.4&os_distribution=centos&os_version=10&components=server_frontend_agent&db=mysql&ws=apache "Zabbix — Download and install Zabbix 7.4 for CentOS 10"
