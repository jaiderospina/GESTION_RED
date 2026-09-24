# Laboratorios de Docker

### Documentación fotográfica del proceso realizado

_Este documento recopila y describe, captura por captura, todo el trabajo práctico realizado a lo largo de los distintos laboratorios de Docker. El contenido está organizado siguiendo el orden de los recorridos guiados (walkthroughs) oficiales de Docker: cómo ejecutar un contenedor, el despliegue de una aplicación multi-contenedor, la contenerización de una aplicación propia, la persistencia de datos entre contenedores, el acceso a carpetas locales desde un contenedor mediante bind mounts, la ejecución de imágenes de Docker Hub y la publicación de una imagen propia._

---


## Sección 1: ¿Cómo ejecuto un contenedor? (How do I run a container?)


> El recorrido guiado "How do I run a container?" del Learning Center de Docker Desktop enseña los fundamentos de trabajar con contenedores: descargar y ejecutar una imagen ya construida, revisar sus archivos y registros mientras está activo, detenerlo cuando ya no se necesita, y —una vez construida una imagen propia— ejecutarla y ver su resultado en el navegador. Esta sección reúne las capturas correspondientes a ambos momentos del laboratorio en los que se practicó este flujo.


### 1.1 Primer contenedor y exploración con Docker Desktop


![Captura](./sec1-01.png)


*Captura 1 — Primer contenedor ejecutado correctamente (localhost:8088)*


Al ejecutar el contenedor de ejemplo `docker/welcome-to-docker` y acceder a `http://localhost:8088`, se muestra la página de felicitaciones "Congratulations!!! You ran your first container.", confirmando que el primer contenedor del laboratorio se ejecutó exitosamente.


![Captura](./assets/sec1-02.png)


*Captura 2 — Panel de contenedores en Docker Desktop y tutorial "What is a container?"*


En la sección Containers de Docker Desktop se listan los contenedores creados hasta el momento: `competent_davinci` (hello-world), `sweet_swirles` (ubuntu) y `welcome-to-docker`, este último expuesto en el puerto 8088. En el panel lateral del Learning Center, dentro del tutorial "What is a container?", se han completado los pasos "Containers on Docker Desktop" y "View the frontend", y se encuentra activo el paso 3, "Explore your container", que invita a seleccionar el contenedor y revisar su pestaña Files.


![Captura](./assets/sec1-03.png)


*Captura 3 — Contenedor detenido (Exited) y paso "Stop your container"*


Se muestra el contenedor `thirsty_merkle` (basado en la imagen `docker/welcome-to-docker`) ya detenido, con estado Exited (0), y su pestaña de Logs con el registro de cierre de los procesos internos. En el panel del tutorial, el paso "Explore your container" aparece completado y se encuentra activo el paso 4, "Stop your container", que explica que un contenedor se mantiene en ejecución hasta que se detiene manualmente pulsando el ícono de Stop.


### 1.2 Ejecución de una imagen propia


![Captura](./assets/sec1-04.png)


*Captura 4 — Tutorial "How do I run a container?": ejecutar el contenedor propio*


En un tutorial distinto del Learning Center, "How do I run a container?", ya completados los pasos de verificar el Dockerfile y construir la primera imagen, se encuentra activo el paso 5, "Run your container". El panel explica que, una vez finalizada la construcción, la imagen aparece en la pestaña Images, desde donde se selecciona y se ejecuta como contenedor indicando un puerto (en este caso, 8089). Como resultado se observa el contenedor `objective_gagarin`, creado a partir de la imagen `welcome-to-docker:latest`, corriendo y mapeado en el puerto 8089:3000.


![Captura](./assets/sec1-05.png)


*Captura 5 — Paso "View the frontend": acceso al contenedor en ejecución*


Con el contenedor `objective_gagarin` en ejecución, el tutorial avanza al paso 6, "View the frontend", que indica cómo acceder a la aplicación en vivo haciendo clic sobre el nombre del contenedor o su puerto publicado. Se muestra el contenedor `musing_pasteur`, expuesto también en el puerto 8089:3000, junto con sus logs, confirmando que el servidor está aceptando conexiones.


![Captura](./assets/sec1-06.png)


*Captura 6 — Página de felicitaciones servida desde la imagen propia (localhost:8089)*


Al abrir `http://localhost:8089` en el navegador se vuelve a mostrar la página "Congratulations!!! You ran your first container.", esta vez servida por el contenedor construido a partir de la imagen propia (welcome-to-docker) y no por el contenedor de ejemplo original, confirmando que la construcción y ejecución manual de la imagen fue exitosa.


## Sección 2: Aplicación multi-contenedor (Multi-container application)


> El recorrido "Multi-container applications" muestra cómo levantar, con un solo comando, una aplicación compuesta por varios servicios usando Docker Compose, en lugar de iniciar cada contenedor por separado con `docker run`. El ejemplo oficial es una aplicación de tareas (Todo App) construida con Node.js/Express que guarda su información en una base de datos MongoDB. Esta sección agrupa las capturas de las dos veces que se practicó este flujo: primero clonando el repositorio manualmente, y luego siguiendo el tutorial guiado de Docker Desktop.


### 2.1 Clonación y exploración del proyecto


![Captura](./assets/sec2-01.png)


*Captura 1 — Clonación del repositorio multi-container-app*


Se clona el repositorio oficial `https://github.com/docker/multi-container-app` dentro de la carpeta personal del usuario (`$HOME`). El log confirma la recepción de los 2318 objetos del repositorio. Después, con el comando `dir` se listan las carpetas y archivos del usuario, comprobando que la carpeta `multi-container-app` ya se encuentra creada, y se accede a ella con `cd`.


![Captura](./assets/sec2-02.png)


*Captura 2 — Archivo de configuración (compose.yaml) abierto en el Bloc de notas*


Dentro de la carpeta del proyecto se ejecuta `notepad compose.yaml` para inspeccionar el archivo de orquestación. En él se observa que la aplicación se define como dos servicios: `todo-app`, que se construye desde el Dockerfile ubicado en `./app` y depende del servicio `todo-database` (imagen oficial de MongoDB desde Docker Hub); además se configuran variables de entorno, el mapeo de los puertos 3000 y 35729, y la sección `develop/watch` que permite reconstruir automáticamente el contenedor cuando cambian archivos como `package.json`.


### 2.2 Levantamiento de contenedores y prueba de la aplicación


![Captura](./assets/sec2-03.png)


*Captura 3 — Ejecución de docker compose up -d*


Se ejecuta `docker compose up -d`, lo que descarga la imagen `mongo:6` y construye la imagen de la aplicación (`multi-container-app-todo-app`) a partir del Dockerfile de Node.js (basado en `node:19.5.0-alpine`). El log detalla cada etapa del build: carga del Dockerfile, resolución de la imagen base, instalación de dependencias con `npm install`, copia del código fuente y exportación de la imagen final. Al terminar, Docker Compose crea la red `multi-container-app_default` y levanta ambos contenedores (`todo-database-1` y `todo-app-1`), quedando ambos en estado Started.


![Captura](./assets/sec2-04.png)


*Captura 4 — Aplicación Todo App corriendo en el navegador (localhost:3000)*


Se abre el navegador en `http://localhost:3000` y se comprueba que la aplicación Todo App está funcionando correctamente. Se agregan tareas de ejemplo ("Aprender Docker Compose", "Realizar un git" y "Tomar Cap de todo"), confirmando que el formulario de creación de tareas se comunica correctamente con el backend y este, a su vez, con la base de datos MongoDB.


### 2.3 Recorrido guiado: clonación y despliegue desde el Learning Center


![Captura](./assets/sec2-05.png)


*Captura 5 — Repositorio docker/multi-container-app en GitHub*


Se visualiza en GitHub el repositorio oficial `docker/multi-container-app`, que contiene el código fuente de la aplicación de tareas usada en el laboratorio. En su README se indican las instrucciones básicas para probarlo: ejecutar `docker compose up -d` y abrir `http://localhost:3000` en el navegador.


![Captura](./assets/sec2-06.png)


*Captura 6 — Tutorial "Multi-container applications": clonar el repositorio*


En el Learning Center de Docker Desktop se inicia el tutorial "Multi-container applications". Se completa el primer paso, "Introducing Docker Compose", y se muestra activo el segundo, "Get the sample application", que indica clonar el repositorio desde `https://github.com/docker/multi-container-app` mediante el comando `git clone` proporcionado en el propio panel.


![Captura](./assets/sec2-07.png)


*Captura 7 — Corrección de un error de ruta y despliegue exitoso con docker compose up -d*


En un primer intento se ejecuta `docker compose -f C:\ruta\al\docker-compose.yml up -d` usando una ruta de ejemplo no reemplazada, lo que produce un error de PowerShell y de Docker Compose al no encontrar el archivo. La propia terminal sugiere usar el asistente de IA de Docker (Gordon) para depurar el error. A continuación, se clona correctamente el repositorio `multi-container-app`, se accede a la carpeta con `cd` y se ejecuta `docker compose up -d`, que descarga la imagen de MongoDB, construye la imagen de la aplicación y levanta ambos contenedores (`todo-database-1` y `todo-app-1`) en estado Started.


![Captura](./assets/sec2-08.png)


*Captura 8 — Todo App funcionando en http://localhost:3000*


Se accede a la aplicación en el navegador y se agregan dos tareas de prueba, "a" y "b", confirmando que el backend y la base de datos MongoDB desplegados con Docker Compose funcionan correctamente en conjunto.


### 2.4 Verificación de contenedores y modo watch


![Captura](./assets/sec2-09.png)


*Captura 9 — Nueva ejecución de docker compose up -d con los contenedores ya activos*


Al finalizar el build anterior y volver a ejecutar `docker compose up -d`, Docker Compose detecta que los contenedores `multi-container-app-todo-database-1` y `multi-container-app-todo-app-1` ya se encuentran en ejecución (Running), por lo que no vuelve a crearlos, solo confirma su estado actual.


![Captura](./assets/sec2-10.png)


*Captura 10 — Activación del modo docker compose watch*


Se ejecuta `docker compose watch`, lo que reconstruye la imagen de la aplicación (18/18 pasos, usando caché en la mayoría de ellos) y deja el mensaje "Watch enabled", habilitando la reconstrucción y sincronización automática del contenedor cada vez que se modifiquen los archivos del proyecto observados por Compose.


![Captura](./assets/sec2-11.png)


*Captura 11 — Búsqueda del archivo compose.yaml en el sistema*


Como parte de la depuración del error de ruta ocurrido al iniciar este laboratorio, se utiliza el comando de PowerShell `Get-ChildItem -Path C:\ -Name "compose.yaml" -Recurse -ErrorAction SilentlyContinue` para localizar todas las copias del archivo `compose.yaml` existentes en el equipo. El resultado muestra tres coincidencias, ubicadas en las carpetas `multi-container-app` dentro de `Users\Redes5`, `Windows\System32` y `welcome-to-docker\multi-container-app`, lo que permite identificar la ruta correcta del archivo.


## Sección 3: Contenerizar tu aplicación (Containerize your application)


> El recorrido "Containerize your application" explica cómo generar, a partir de un proyecto propio, los archivos necesarios para ejecutarlo en Docker: un Dockerfile que define la imagen y un compose.yaml que define cómo ejecutarla. Para automatizar esta tarea, Docker Desktop ofrece el comando `docker init`, que hace una serie de preguntas guiadas y crea estos archivos con valores por defecto razonables. Esta sección documenta tanto el uso de `docker init` sobre un proyecto en Python como la construcción manual de una imagen a partir de un Dockerfile ya existente con `docker build`.


### 3.1 Inicialización de un proyecto con docker init


![Captura](./assets/sec3-01.png)


*Captura 1 — Pantalla de bienvenida de la CLI de docker init*


Se ejecuta el comando `docker init` desde la terminal de PowerShell. El asistente indica que creará cuatro archivos con valores por defecto razonables para el proyecto: `.dockerignore`, `Dockerfile`, `compose.yaml` y `README.Docker.md`. A continuación, pregunta qué plataforma utiliza el proyecto, mostrando un menú con opciones como Go, Python, Node, Rust, ASP.NET Core, PHP with Apache, Java u Other. En este caso se selecciona Python, ya que la aplicación está desarrollada en dicho lenguaje.


![Captura](./assets/sec3-02.png)


*Captura 2 — Respuestas al asistente y creación de los archivos Docker*


El asistente solicita la versión de Python (3.14), el puerto en el que escuchará la aplicación (8000) y el comando para ejecutarla. En el primer intento se deja el campo vacío y el asistente lo rechaza ("Value is required"); en el segundo se indica `python app.py`. Con estos datos se confirman los cuatro archivos generados y se muestra el mensaje "Your Docker files are ready!", junto con una advertencia de que no se encontró un archivo `requirements.txt`, recomendando crearlo antes de ejecutar el contenedor. Finalmente se indica el siguiente paso: ejecutar `docker compose up --build` para levantar la aplicación en `http://localhost:8000`.


![Captura](./assets/sec3-03.png)


*Captura 3 — Proceso de construcción (build) de la imagen con Docker Compose*


Se ejecuta `docker compose up --build`, que construye la imagen a partir del Dockerfile generado. El log evidencia el uso de caché para capas ya descargadas, la resolución de la imagen base `python:3.14-slim`, la descarga y extracción de sus capas, la configuración del directorio de trabajo `/app` y la creación de un usuario sin privilegios por buenas prácticas de seguridad. El proceso se cancela (CANCELED) durante la carga del contexto de construcción, que estaba transfiriendo 3.49 GB, lo que indica que el `.dockerignore` no estaba excluyendo correctamente carpetas pesadas del proyecto; por ello, al final del log el estado queda como "up 0/1", sin levantar el contenedor en ese intento.


### 3.2 Construcción manual de una imagen con docker build


![Captura](./assets/sec3-04.png)


*Captura 4 — Clonación del repositorio y construcción de la imagen con docker build*


Se clona el repositorio `https://github.com/docker/welcome-to-docker` y, dentro de la carpeta del proyecto, se ejecuta `docker build -t welcome-to-docker .` Este comando construye la imagen a partir del Dockerfile del repositorio, usando como base `node:22-alpine`, copiando los archivos `package*.json`, el código fuente (`./src`), los archivos públicos (`./public`) y el `.npmrc`, e instalando las dependencias con `npm ci` antes de compilar la aplicación (`npm run build`). Al finalizar, la imagen queda registrada localmente como `docker.io/library/welcome-to-docker:latest`. Esta misma imagen es la que se ejecuta y se explora en la Sección 1 de este documento.


## Sección 4: Persistencia de datos entre contenedores (Persist your data between containers)


> El recorrido "Persist your data between containers" muestra que, por defecto, los datos generados dentro de un contenedor viven solo mientras ese contenedor existe: si se elimina o se recrea, la información se pierde a menos que se use un mecanismo de persistencia (por ejemplo, un volumen). Las capturas de esta sección documentan tanto la comprobación de que los datos de la Todo App sobreviven al recargar el navegador, como el caso contrario: la pérdida de datos al recrear el contenedor de la base de datos durante el modo watch.


![Captura](./assets/sec4-01.png)


*Captura 1 — Verificación de persistencia de datos al reabrir la aplicación*


Al abrir una nueva pestaña y volver a cargar la aplicación, las tres tareas creadas anteriormente siguen apareciendo en la lista, lo que confirma que los datos quedaron almacenados de forma persistente en el contenedor de MongoDB y no se perdieron al recargar la página.


![Captura](./assets/sec4-02.png)


*Captura 2 — Edición del archivo todos.ejs y actualización en vivo del título*


Se abre el archivo de plantilla `app/views/todos.ejs` en el editor y se modifica el título de la aplicación, cambiándolo de "Todo App" a "Mi Todo App 🚀". Gracias al modo watch de Docker Compose (activado en la captura siguiente), el cambio se refleja automáticamente en el navegador sin necesidad de reconstruir manualmente el contenedor, y las tareas previamente creadas siguen visibles.


![Captura](./assets/sec4-03.png)


*Captura 3 — docker compose watch y reinicio de los contenedores*


Se ejecuta `docker compose watch`, que reconstruye la imagen (Building 16/16 FINISHED) y recrea el contenedor `todo-app-1` automáticamente al detectar el cambio en el archivo observado, mostrando "Watch enabled". Sin embargo, al recargar la aplicación esta aparece vacía ("Please add some task."), ya que la recreación del contenedor de la base de datos reinició su estado, evidenciando que sin un volumen persistente los datos de MongoDB no sobreviven a la recreación del contenedor. Posteriormente se vuelve a ejecutar `docker compose up -d`, recreando la red y ambos contenedores desde cero (Created / Started).


## Sección 5: Acceso a la carpeta local desde un contenedor (Access your local folder from a container)


> El recorrido "Access your local folder from a container" enseña a usar bind mounts para conectar una carpeta del equipo local directamente con una carpeta dentro del contenedor, de modo que los cambios hechos en el editor de código se reflejen de inmediato dentro de la aplicación en ejecución, sin necesidad de reconstruir ni reiniciar el contenedor. Esta sección documenta la práctica de esta técnica con el proyecto de ejemplo bindmount-apps.


![Captura](./assets/sec5-01.png)


*Captura 1 — Clonación del repositorio bindmount-apps*


Se clona un nuevo repositorio, `https://github.com/docker/bindmount-apps`, orientado a practicar el uso de bind mounts. Tras acceder a la carpeta con `cd bindmount-apps`, se lista su contenido con `dir`, encontrando los archivos `.gitignore`, `.npmrc`, `compose.yaml`, `README.md` y la carpeta `app`, que contiene el código fuente de la aplicación.


![Captura](./assets/sec5-02.png)


*Captura 2 — Levantamiento de los contenedores con docker compose up -d*


Se ejecuta `docker compose up -d` dentro del proyecto `bindmount-apps`, levantando correctamente los dos contenedores definidos: `bindmount-apps-todo-app-1` y `bindmount-apps-todo-database-1`, ambos en estado Started.


![Captura](./assets/sec5-03.png)


*Captura 3 — Edición en vivo del archivo todos.ejs gracias al bind mount*


Se edita el archivo `todos.ejs`, cambiando el texto del formulario a "Holiiii:)". A diferencia del ejemplo anterior con watch, aquí el cambio se refleja de inmediato en el navegador (lado izquierdo) sin necesidad de reconstruir ni reiniciar el contenedor, ya que la carpeta del proyecto está montada directamente dentro del contenedor mediante un bind mount, permitiendo que cualquier edición en el sistema de archivos local se vea reflejada en tiempo real.


## Sección 6: Ejecutar imágenes de Docker Hub (Run Docker Hub images)


> El recorrido "Run Docker Hub images" muestra cómo aprovechar Docker Hub, el registro público de imágenes de Docker, para descargar (pull) y ejecutar imágenes ya construidas por otros, sin necesidad de escribir un Dockerfile propio. Esta sección documenta la descarga de la imagen de ejemplo `docker/welcome-to-docker` que se usa como base en varios de los laboratorios anteriores.


![Captura](./assets/sec6-01.png)


*Captura 1 — Descarga de la imagen welcome-to-docker y listado de imágenes locales*


Se descarga la imagen `docker/welcome-to-docker` con `docker pull` y, posteriormente, se listan todas las imágenes disponibles localmente con `docker images`, donde se pueden ver, entre otras, `bindmount-apps-todo-app`, `docker/welcome-to-docker`, `hello-world`, `mongo:6`, `multi-container-app-todo-app` y `ubuntu`, junto con su tamaño en disco y su tamaño de contenido.


## Sección 7: Publicar tu imagen (Publish your image)


> El recorrido "Publish your image" enseña a compartir una imagen propia en Docker Hub para que pueda ser descargada y ejecutada por otras personas o en otros equipos. El proceso consiste en renombrar (tag) la imagen local con el nombre de usuario de Docker Hub y luego subirla (push) al repositorio personal. Esta sección documenta la publicación de la imagen construida en la Sección 3 en la cuenta personal de Docker Hub.


![Captura](./assets/sec7-01.png)


*Captura 1 — Publicación (push) de la imagen en el Docker Hub personal*


Finalmente, se ejecuta `docker push juliiana/welcome-to-docker`, publicando la imagen en el repositorio personal del usuario en Docker Hub (`juliiana`). El log confirma que cada capa de la imagen ya existía en el repositorio de origen (Mounted from `docker/welcome-to-docker`) y que la operación finaliza correctamente, generando el digest final de la imagen publicada.


## Conclusión general


A lo largo de estos siete bloques de trabajo se documentó el ciclo completo de aprendizaje de Docker, siguiendo el mismo orden que proponen los recorridos guiados oficiales: primero ejecutar un contenedor ya construido y explorarlo con Docker Desktop; luego orquestar una aplicación real de varios servicios (Node.js y MongoDB) con Docker Compose; contenerizar una aplicación propia generando sus archivos base con `docker init` y construyendo manualmente una imagen con `docker build`; comprobar cómo se comporta —y cómo se pierde— la persistencia de datos entre recreaciones de contenedores; usar bind mounts para editar código en caliente accediendo a una carpeta local desde el contenedor; descargar y ejecutar imágenes públicas desde Docker Hub; y, finalmente, publicar una imagen propia en el Docker Hub personal.
