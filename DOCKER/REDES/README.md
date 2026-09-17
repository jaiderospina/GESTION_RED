# Redes en Docker con MySQL y phpMyAdmin

Este ejemplo muestra cómo Docker Networking permite conectar servicios de forma aislada y segura, resolviendo automáticamente los nombres de los servicios dentro de la misma red.

En este ejemplo, crearemos un entorno Docker con:
1. Una red personalizada para aislar nuestros servicios
2. Un contenedor con MySQL (base de datos)
3. Un contenedor con phpMyAdmin (interfaz web para administrar MySQL)
4. Conexión entre estos servicios a través de la red Docker

## Paso 1: Crear una red personalizada en Docker

Las redes en Docker permiten la comunicación entre contenedores de forma aislada. Crearemos una red llamada `mi-red-app`:

```bash
docker network create mi-red-app
```

## Paso 2: Configurar el contenedor MySQL

Vamos a crear un contenedor MySQL con las siguientes características:
- Usará nuestra red personalizada
- Tendrá variables de entorno para configuración
- Los datos persistirán en un volumen

```bash
docker run -d \
  --name mi-mysql \
  --network mi-red-app \
  -e MYSQL_ROOT_PASSWORD=my-secret-pw \
  -e MYSQL_DATABASE=mi_base_datos \
  -e MYSQL_USER=mi_usuario \
  -e MYSQL_PASSWORD=mi_contraseña \
  -v mysql_data:/var/lib/mysql \
  mysql:8.0
```

Explicación de los parámetros:
- `-d`: Ejecuta el contenedor en segundo plano (detached mode)
- `--name`: Asigna un nombre al contenedor
- `--network`: Conecta el contenedor a nuestra red personalizada
- `-e`: Establece variables de entorno (configuración de MySQL)
- `-v`: Crea un volumen para persistencia de datos

## Paso 3: Configurar phpMyAdmin

Ahora crearemos un contenedor con phpMyAdmin que se conectará a nuestro MySQL:

```bash
docker run -d \
  --name mi-phpmyadmin \
  --network mi-red-app \
  -e PMA_HOST=mi-mysql \
  -e PMA_PORT=3306 \
  -p 8080:80 \
  phpmyadmin/phpmyadmin
```

Explicación de los parámetros:
- `PMA_HOST=mi-mysql`: Le indica a phpMyAdmin el nombre del servicio MySQL (resuelto por la red Docker)
- `PMA_PORT=3306`: Puerto estándar de MySQL
- `-p 8080:80`: Mapea el puerto 80 del contenedor al 8080 de nuestro host

## Paso 4: Verificar la conexión

1. Accede a phpMyAdmin en tu navegador: http://localhost:8080
2. Inicia sesión con:
   - Usuario: root
   - Contraseña: my-secret-pw (la que configuramos en MYSQL_ROOT_PASSWORD)

## Paso 5: Conectar una aplicación (ejemplo con Node.js)

Para completar el ejemplo, aquí está cómo conectar una aplicación Node.js a nuestra base de datos MySQL:

```javascript
// app.js
const mysql = require('mysql2');

const connection = mysql.createConnection({
  host: 'mi-mysql', // Nombre del servicio MySQL en la red Docker
  user: 'mi_usuario',
  password: 'mi_contraseña',
  database: 'mi_base_datos'
});

connection.connect(err => {
  if (err) {
    console.error('Error de conexión: ' + err.stack);
    return;
  }
  console.log('Conectado a MySQL con ID ' + connection.threadId);
});

// Ejemplo de consulta
connection.query('SELECT 1 + 1 AS solution', (err, results) => {
  if (err) throw err;
  console.log('La solución es: ', results[0].solution);
});

connection.end();
```

Para ejecutar esta aplicación en Docker, crearíamos un Dockerfile:

```dockerfile
# Dockerfile
FROM node:14
WORKDIR /usr/src/app
COPY package*.json ./
RUN npm install
COPY . .
EXPOSE 3000
CMD ["node", "app.js"]
```

Y lo ejecutaríamos en nuestra red:

```bash
docker build -t mi-app-node .
docker run -it --rm --network mi-red-app mi-app-node
```

## Estructura final de la red

```
+------------------------------------------------------+
|                  Red Docker: mi-red-app              |
|                                                      |
|  +-------------+      +----------------+             |
|  | Contenedor  |      | Contenedor     |             |
|  | MySQL       |      | phpMyAdmin     |             |
|  | (mi-mysql)  |<---->| (mi-phpmyadmin)|             |
|  +-------------+      +----------------+             |
|                                                      |
|  +-------------+                                     |
|  | Contenedor  |                                     |
|  | Node.js App |                                     |
|  | (mi-app)    |                                     |
|  +-------------+                                     |
+------------------------------------------------------+
```

## Comandos útiles para verificar

1. Listar contenedores en ejecución:
   ```bash
   docker ps
   ```

2. Inspeccionar la red:
   ```bash
   docker network inspect mi-red-app
   ```

3. Ver logs de un contenedor:
   ```bash
   docker logs mi-mysql
   ```

4. Detener y eliminar todos los recursos:
   ```bash
   docker stop mi-mysql mi-phpmyadmin
   docker rm mi-mysql mi-phpmyadmin
   docker network rm mi-red-app
   docker volume rm mysql_data
   ```


- **TALLER BÁSICO**

https://github.com/jaiderospina/Docker20242

---

## **Ejercicio Máquina de Escribir**

En este ejemplo se crea una animación de texto tipo *"máquina de escribir"* usando Bash dentro de un contenedor Docker muy ligero basado en Alpine Linux.

## 1. **Crear el script con `cat`**

```bash
cat << 'EOF' > typewriter.sh
#!/bin/bash
mensaje="¡Hola desde un contenedor Docker animado en la terminal!"
velocidad=0.05

for ((i=0; i<${#mensaje}; i++)); do
  echo -n "${mensaje:$i:1}"
  sleep $velocidad
done

echo ""
EOF
```

### ¿Qué hace esto?

1. **`cat << 'EOF' > typewriter.sh`**: Usa redirección para crear un archivo llamado `typewriter.sh` y escribir en él el contenido que va entre `EOF` y `EOF`. La comilla simple (`'EOF'`) impide la expansión de variables durante la escritura.

2. **`#!/bin/bash`**: Indica que el script debe ejecutarse con Bash.

3. **`mensaje="..."`**: Variable que contiene el mensaje que se va a imprimir "letra por letra".

4. **`velocidad=0.05`**: Tiempo (en segundos) de espera entre cada carácter (50 milisegundos).

5. **`for ((i=0; i<${#mensaje}; i++))`**: Bucle que recorre carácter por carácter la cadena.

6. **`echo -n "${mensaje:$i:1}"`**: Imprime el carácter actual sin salto de línea.

7. **`sleep $velocidad`**: Espera antes de imprimir el siguiente carácter.

8. **`echo ""`**: Salto de línea al final del mensaje.

**Resultado:** Cuando se ejecuta, el texto se imprime como si alguien lo estuviera escribiendo en tiempo real.

---

## 2. **Crear el Dockerfile**

```bash
cat << 'EOF' > Dockerfile
FROM alpine:latest

RUN apk add --no-cache bash

COPY typewriter.sh /typewriter.sh
RUN chmod +x /typewriter.sh

CMD ["/typewriter.sh"]
EOF
```

### Explicación línea por línea:

1. **`FROM alpine:latest`**: Usa Alpine Linux como base (una distribución extremadamente ligera y rápida).

2. **`RUN apk add --no-cache bash`**: Instala Bash, ya que Alpine viene con `sh` por defecto. `--no-cache` evita guardar archivos temporales de instalación.

3. **`COPY typewriter.sh /typewriter.sh`**: Copia el script Bash al contenedor.

4. **`RUN chmod +x /typewriter.sh`**: Da permisos de ejecución al script.

5. **`CMD ["/typewriter.sh"]`**: Indica el comando predeterminado que se ejecutará cuando se inicie el contenedor (el script de animación).

---

## 3. **Construir la imagen Docker**

```bash
docker build -t animacion-texto .
```

### ¿Qué hace?

* **`docker build`**: Construye una imagen desde el `Dockerfile`.
* **`-t animacion-texto`**: Le da el nombre `animacion-texto` a la imagen.
* **`.`**: Indica que la construcción se realiza en el directorio actual (donde están el `Dockerfile` y el `typewriter.sh`).

---

## 4. **Ejecutar el contenedor**

```bash
docker run --rm animacion-texto
```

### ¿Qué hace?

* **`docker run`**: Ejecuta un contenedor basado en la imagen.
* **`--rm`**: Elimina el contenedor automáticamente cuando termina.
* **`animacion-texto`**: Es la imagen que creamos antes.

**Resultado final:** Cuando ejecutas este comando, verás en tu terminal cómo se imprime el mensaje carácter por carácter, simulando una animación tipo máquina de escribir.

## RETO No 1. INDIVIDUAL

Mejorar el script para:

- **Pasar el mensaje como variable de entorno (-e MENSAJE="Hola mundo").**
- **Cambiar la velocidad (-e VELOCIDAD=0.1).**
- **Hacer que el script lea argumentos o interactúe con el usuario.**

---



