# 1. Introducción teórica

## 1.1 Contexto

Hoy las redes de datos cubren más que computadores y servidores. Sensores, cámaras y controladores (IoT) también comparten la infraestructura, y su estado hay que vigilarlo con las mismas herramientas de gestión de red. Este proyecto integra una estación de sensores construida sobre Arduino UNO con **Zabbix 7.0 LTS**, una plataforma de monitoreo de código abierto. Zabbix recoge las mediciones, las guarda en una base de datos, las muestra en un tablero y avisa por **Telegram** cuando algo se sale de lo normal.

## 1.2 Zabbix

Zabbix es un sistema de monitoreo distribuido con estos componentes:

| Componente | Función en el proyecto |
|---|---|
| **Zabbix Server** | Proceso central: consulta los agentes, evalúa iniciadores (triggers) y ejecuta acciones. |
| **Base de datos (MariaDB)** | Guarda la configuración, el historial y las tendencias. |
| **Frontend web (Apache + PHP)** | Interfaz para configurar y ver tableros, gráficos y problemas. |
| **Zabbix Agent** | Recoge valores en el equipo monitoreado y responde en el puerto TCP 10050. |

Conceptos utilizados:

- **Equipo (host):** el dispositivo monitoreado. Aquí es "Estacion Arduino".
- **Métrica (item):** un valor que se recoge periódicamente, identificado por una *clave* (por ejemplo `sensor.luz`).
- **Iniciador (trigger):** una expresión lógica sobre las métricas que, al cumplirse, genera un *problema* con una severidad.
- **Plantilla (template):** un conjunto reutilizable de métricas, iniciadores, gráficos y macros.
- **Macro de usuario:** una variable (`{$RUIDO.ALTO}`) que permite cambiar umbrales sin editar los iniciadores.
- **Acción y tipo de medio:** definen *a quién* y *por qué medio* se notifica (aquí, un webhook de Telegram).
- **Dependencia de iniciadores:** evita alertas en cascada. Si la estación deja de enviar datos, solo se informa ese problema.

### Agente pasivo y protocolo

En modo **pasivo**, el servidor abre una conexión TCP al puerto 10050 del agente y envía la clave solicitada. El agente responde con la cabecera `ZBXD\x01`, la longitud del dato en 8 bytes *little-endian* y el valor. Desde Zabbix 7.0 la petición viaja en formato JSON (`{"request":"passive checks","data":[{"key":"..."}]}`). El firmware del proyecto implementa ambos formatos en el modo Ethernet.

## 1.3 Arduino UNO

El Arduino UNO es una placa de desarrollo basada en el microcontrolador ATmega328P (16 MHz, 32 KB de Flash, 2 KB de SRAM). Ofrece 14 pines digitales, 6 analógicos (ADC de 10 bits, 0 a 1023), bus I2C (A4/A5) y comunicación serie por USB. Su programación se hace en C/C++ con el IDE de Arduino.

## 1.4 Sensores utilizados

| Sensor | Variable | Tipo de señal | Principio |
|---|---|---|---|
| **HC-SR501 (PIR)** | Movimiento | Digital (D7) | Detecta cambios de radiación infrarroja de cuerpos calientes. Tiene potenciómetros de sensibilidad y tiempo, y un jumper de disparo simple o repetitivo. |
| **SW-18015P** | Vibración | Digital (D8) | Interruptor de resorte que se cierra momentáneamente con golpes o vibraciones. El módulo incluye un comparador LM393. |
| **Módulo de sonido (micrófono electret)** | Nivel de ruido | Analógica (A0) | El micrófono y el amplificador entregan una señal de audio; se mide su amplitud pico a pico. |
| **BH1750 (GY-302)** | Iluminancia | I2C (dirección 0x23) | Fotodiodo con ADC de 16 bits que entrega la iluminancia directamente en lux (1 a 65535 lx). |

> El sensor de temperatura y humedad DHT11 previsto inicialmente se omitió con aprobación del docente. Los cuatro sensores anteriores fueron aceptados como conjunto final.

## 1.5 Conexión Arduino – Zabbix

La guía del curso propone conectar el Arduino a la red con un **Ethernet Shield W5100**, de forma que la placa actúe como agente Zabbix. Durante el desarrollo, el **docente aprobó una alternativa por USB**: el Arduino envía sus lecturas por el puerto serie al servidor, y el agente Zabbix de la propia máquina virtual las publica con las mismas claves. El firmware mantiene los dos modos, seleccionables con una constante (`USAR_ETHERNET`).

## 1.6 Telegram como canal de alertas

Zabbix 7.0 incluye un tipo de medio **webhook** para Telegram que llama a la API de bots (`https://api.telegram.org/bot<token>/sendMessage`). Solo se necesitan el **token** del bot, entregado por @BotFather, y el **chat ID** del destinatario. El docente dio a elegir entre correo Gmail y Telegram; el grupo eligió **Telegram** porque los avisos llegan en segundos al celular y la configuración no depende de contraseñas de aplicación ni de servidores SMTP.
