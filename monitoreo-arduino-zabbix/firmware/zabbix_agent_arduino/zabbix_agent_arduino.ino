/*
 * Estacion de monitoreo para Zabbix con Arduino UNO
 * Sensores: PIR HC-SR501, vibracion SW-18015P, sonido (microfono) y luz BH1750
 *
 * Basado en el proyecto Arduino-Zabbix-Agent de Interlegis (licencia GPL v2):
 * https://github.com/interlegis/Arduino-Zabbix-Agent
 *
 * Dos modos de trabajo (elige uno en USAR_ETHERNET):
 *   0 = USB. El Arduino envia una linea de datos cada 5 s por el puerto serie.
 *       El agente Zabbix de la VM la publica con las mismas claves.
 *   1 = Ethernet Shield W5100. El Arduino responde como agente Zabbix
 *       pasivo en el puerto TCP 10050 (cabecera ZBXD y JSON de Zabbix 7.0).
 *
 * Claves que entrega en ambos modos:
 *   sensor.luz          luz en lux (BH1750)
 *   sensor.movimiento   1 si hubo movimiento en los ultimos 60 s
 *   sensor.vibracion    1 si hubo vibracion en los ultimos 60 s
 *   sensor.vibraciones  total de vibraciones desde el encendido
 *   sensor.ruido        pico de ruido de los ultimos 30 a 60 s (0 a 1023)
 *   sensor.uptime       segundos desde el encendido
 */

#define USAR_ETHERNET 0          // 0 = USB, 1 = Ethernet Shield

#include <Wire.h>
#if USAR_ETHERNET
#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h>
#endif

// ---------------- Pines ----------------
// D4 y D10 a D13 quedan libres para el Ethernet Shield.
#define PIN_LED_AZUL      3   // Consulta de Zabbix o envio de datos
#define PIN_LED_ROJO      5   // Movimiento
#define PIN_LED_AMARILLO  6   // Vibracion
#define PIN_PIR           7
#define PIN_VIBRACION     8
#define PIN_LED_NARANJA   9   // Ruido alto
#define PIN_SONIDO        A0
#define PIN_SD_CS         4
// BH1750: SDA en A4 y SCL en A5 (bus I2C del UNO)

// ---------------- Parametros ----------------
#define DIR_BH1750          0x23     // ADDR sin conectar o a GND
#define VENTANA_EVENTO_MS   60000UL  // Cuanto se mantiene en 1 movimiento y vibracion
#define BLOQUE_RUIDO_MS     30000UL  // El pico de ruido cubre de 30 a 60 s
#define UMBRAL_RUIDO_LED    200      // Pico que enciende el LED naranja
#define ANTIRREBOTE_MS      50
#define INTERVALO_LUZ_MS    1000UL
#define INTERVALO_SERIE_MS  5000UL

#if USAR_ETHERNET
// ---------------- Red: AJUSTAR A TU RED ----------------
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0xE3, 0x1B };  // Usa la etiqueta del shield
IPAddress ipArduino(192, 168, 1, 50);
IPAddress dnsRed(192, 168, 1, 254);
IPAddress gatewayRed(192, 168, 1, 254);
IPAddress mascara(255, 255, 255, 0);
IPAddress ipZabbix(192, 168, 1, 60);     // Solo esta IP puede consultar
#define RESTRINGIR_ORIGEN   1
#define PUERTO_AGENTE       10050
#define VERSION_AGENTE      "7.0.0"
#define TIMEOUT_LECTURA_MS  2000UL
#define TAM_ENTRADA         160
const uint8_t CABECERA[] = { 'Z', 'B', 'X', 'D', 0x01 };
EthernetServer servidor(PUERTO_AGENTE);
#endif

#define TAM_CLAVE  32
#define TAM_VALOR  24

// ---------------- Estado ----------------
float luzLux = 0;
bool luzValida = false;
unsigned long ultimaLuz = 0;

unsigned long ultimoMovimiento = 0;
bool huboMovimiento = false;

unsigned long vibraciones = 0;
unsigned long ultimaVibracion = 0;
bool huboVibracion = false;
int estadoVibracion = LOW;
unsigned long cambioVibracion = 0;

int ruidoBloqueActual = 0;
int ruidoBloqueAnterior = 0;
unsigned long inicioBloqueRuido = 0;
int ruidoInstante = 0;
unsigned long ultimoRuido = 0;

unsigned long ultimoEnvioSerie = 0;
unsigned long apagarAzul = 0;

// ---------------- BH1750 ----------------
bool iniciarBh1750() {
  Wire.beginTransmission(DIR_BH1750);
  Wire.write(0x01);                    // Encender
  if (Wire.endTransmission() != 0) return false;
  Wire.beginTransmission(DIR_BH1750);
  Wire.write(0x10);                    // Medicion continua, 1 lux de resolucion
  return Wire.endTransmission() == 0;
}

void actualizarLuz() {
  if (millis() - ultimaLuz < INTERVALO_LUZ_MS) return;
  ultimaLuz = millis();
  if (Wire.requestFrom(DIR_BH1750, 2) == 2) {
    unsigned int crudo = ((unsigned int)Wire.read() << 8) | Wire.read();
    luzLux = crudo / 1.2;
    luzValida = true;
  } else {
    luzValida = false;
    iniciarBh1750();                   // Reintenta si se solto un cable
  }
}

// ---------------- PIR ----------------
void actualizarMovimiento() {
  bool ahora = digitalRead(PIN_PIR) == HIGH;
  digitalWrite(PIN_LED_ROJO, ahora ? HIGH : LOW);
  if (ahora) {
    ultimoMovimiento = millis();
    huboMovimiento = true;
  }
}

// ---------------- Vibracion ----------------
// Cuenta cada cambio de la salida DO, sin importar si el modulo
// marca la vibracion con nivel alto o bajo.
void actualizarVibracion() {
  int lectura = digitalRead(PIN_VIBRACION);
  if (lectura != estadoVibracion && millis() - cambioVibracion > ANTIRREBOTE_MS) {
    estadoVibracion = lectura;
    cambioVibracion = millis();
    vibraciones++;
    ultimaVibracion = millis();
    huboVibracion = true;
  }
  bool reciente = huboVibracion && millis() - ultimaVibracion < 2000UL;
  digitalWrite(PIN_LED_AMARILLO, reciente ? HIGH : LOW);
}

// ---------------- Sonido ----------------
// Mide la amplitud pico a pico durante 20 ms, cinco veces por segundo.
void actualizarRuido() {
  if (millis() - ultimoRuido < 200) return;
  ultimoRuido = millis();

  int minimo = 1023, maximo = 0;
  unsigned long inicio = millis();
  while (millis() - inicio < 20) {
    int v = analogRead(PIN_SONIDO);
    if (v < minimo) minimo = v;
    if (v > maximo) maximo = v;
  }
  ruidoInstante = maximo - minimo;

  if (millis() - inicioBloqueRuido >= BLOQUE_RUIDO_MS) {
    ruidoBloqueAnterior = ruidoBloqueActual;
    ruidoBloqueActual = 0;
    inicioBloqueRuido = millis();
  }
  if (ruidoInstante > ruidoBloqueActual) ruidoBloqueActual = ruidoInstante;
  digitalWrite(PIN_LED_NARANJA, ruidoInstante >= UMBRAL_RUIDO_LED ? HIGH : LOW);
}

// ---------------- Valores publicados ----------------
int movimientoReciente() {
  return huboMovimiento && millis() - ultimoMovimiento < VENTANA_EVENTO_MS;
}

int vibracionReciente() {
  return huboVibracion && millis() - ultimaVibracion < VENTANA_EVENTO_MS;
}

int ruidoPico() {
  return max(ruidoBloqueActual, ruidoBloqueAnterior);
}

// Devuelve true y llena valor; devuelve false y llena error.
bool resolverClave(const char *clave, char *valor, const char *&error) {
  if (strcmp(clave, "sensor.luz") == 0) {
    if (!luzValida) { error = "BH1750 sin lectura valida."; return false; }
    dtostrf(luzLux, 1, 1, valor);
  } else if (strcmp(clave, "sensor.movimiento") == 0) {
    itoa(movimientoReciente(), valor, 10);
  } else if (strcmp(clave, "sensor.vibracion") == 0) {
    itoa(vibracionReciente(), valor, 10);
  } else if (strcmp(clave, "sensor.vibraciones") == 0) {
    ultoa(vibraciones, valor, 10);
  } else if (strcmp(clave, "sensor.ruido") == 0) {
    itoa(ruidoPico(), valor, 10);
  } else if (strcmp(clave, "sensor.uptime") == 0 || strcmp(clave, "system.uptime") == 0) {
    ultoa(millis() / 1000UL, valor, 10);
  } else if (strcmp(clave, "agent.ping") == 0) {
    strcpy(valor, "1");
  } else if (strcmp(clave, "agent.version") == 0) {
    strcpy(valor, "arduino-2.0.0");
  } else {
    error = "Unsupported item key.";
    return false;
  }
  return true;
}

void encenderAzul() {
  digitalWrite(PIN_LED_AZUL, HIGH);
  apagarAzul = millis() + 150;
}

#if !USAR_ETHERNET
// ---------------- Modo USB ----------------
// Formato: DATOS;luz=123.4;movimiento=0;vibracion=1;vibraciones=8;ruido=45;uptime=120
// Si el BH1750 falla, se omite la luz para que Zabbix no guarde un valor falso.
void enviarSerie() {
  if (millis() - ultimoEnvioSerie < INTERVALO_SERIE_MS) return;
  ultimoEnvioSerie = millis();
  encenderAzul();

  char valor[TAM_VALOR];
  Serial.print(F("DATOS"));
  if (luzValida) {
    Serial.print(F(";luz="));
    Serial.print(dtostrf(luzLux, 1, 1, valor));
  }
  Serial.print(F(";movimiento="));
  Serial.print(movimientoReciente());
  Serial.print(F(";vibracion="));
  Serial.print(vibracionReciente());
  Serial.print(F(";vibraciones="));
  Serial.print(vibraciones);
  Serial.print(F(";ruido="));
  Serial.print(ruidoPico());
  Serial.print(F(";uptime="));
  Serial.println(millis() / 1000UL);
}
#endif

#if USAR_ETHERNET
// ---------------- Modo Ethernet: protocolo Zabbix ----------------
int leerByte(EthernetClient &cliente, unsigned long limite) {
  while (!cliente.available()) {
    if (!cliente.connected() || (long)(millis() - limite) >= 0) return -1;
    wdt_reset();
  }
  return cliente.read();
}

// Lee la solicitud con cabecera ZBXD y deja el contenido en entrada.
int leerSolicitud(EthernetClient &cliente, char *entrada) {
  unsigned long limite = millis() + TIMEOUT_LECTURA_MS;
  uint8_t cabecera[13];
  for (byte i = 0; i < sizeof(cabecera); i++) {
    int b = leerByte(cliente, limite);
    if (b < 0) return -1;
    cabecera[i] = b;
    if (i < 4 && b != CABECERA[i]) return -1;
  }
  if (cabecera[4] != 0x01) return -1;
  if (cabecera[7] != 0 || cabecera[8] != 0) return -1;
  unsigned int longitud = cabecera[5] | ((unsigned int)cabecera[6] << 8);
  if (longitud == 0 || longitud >= TAM_ENTRADA) return -1;
  for (unsigned int i = 0; i < longitud; i++) {
    int b = leerByte(cliente, limite);
    if (b < 0) return -1;
    entrada[i] = (char)b;
  }
  entrada[longitud] = '\0';
  return longitud;
}

// Acepta el JSON de Zabbix 7.0 o la clave sola del protocolo anterior.
bool extraerClave(char *entrada, char *clave, bool &esJson) {
  esJson = (entrada[0] == '{');
  char *inicio = entrada;
  if (esJson) {
    inicio = strstr(entrada, "\"key\":\"");
    if (inicio == NULL) return false;
    inicio += 7;
  }
  byte i = 0;
  while (inicio[i] != '\0' && inicio[i] != '"' && inicio[i] != '\n' && inicio[i] != '\r') {
    if (i >= TAM_CLAVE - 1) return false;
    clave[i] = inicio[i];
    i++;
  }
  clave[i] = '\0';
  return i > 0;
}

void agregar(char *destino, unsigned int &pos, const char *texto) {
  while (*texto) destino[pos++] = *texto++;
}

void responder(EthernetClient &cliente, bool esJson, bool correcto, const char *contenido) {
  char salida[110];
  unsigned int n = 13;
  if (esJson) {
    agregar(salida, n, "{\"version\":\"" VERSION_AGENTE "\",\"variant\":1,\"data\":[{\"");
    agregar(salida, n, correcto ? "value" : "error");
    agregar(salida, n, "\":\"");
    agregar(salida, n, contenido);
    agregar(salida, n, "\"}]}");
  } else if (correcto) {
    agregar(salida, n, contenido);
  } else {
    agregar(salida, n, "ZBX_NOTSUPPORTED");
    salida[n++] = '\0';
    agregar(salida, n, contenido);
  }
  unsigned int longitud = n - 13;
  memcpy(salida, CABECERA, 5);
  salida[5] = longitud & 0xFF;
  salida[6] = (longitud >> 8) & 0xFF;
  memset(salida + 7, 0, 6);
  cliente.write((const uint8_t *)salida, n);
  cliente.flush();
}

void atenderCliente(EthernetClient &cliente) {
#if RESTRINGIR_ORIGEN
  if (cliente.remoteIP() != ipZabbix) {
    cliente.stop();
    return;
  }
#endif
  encenderAzul();
  char entrada[TAM_ENTRADA];
  char clave[TAM_CLAVE];
  char valor[TAM_VALOR];
  const char *error = "Invalid request.";
  bool esJson = false;
  if (leerSolicitud(cliente, entrada) > 0) {
    bool claveValida = extraerClave(entrada, clave, esJson);
    bool correcto = claveValida && resolverClave(clave, valor, error);
    responder(cliente, esJson, correcto, correcto ? valor : error);
  }
  delay(1);
  cliente.stop();
}
#endif

// ---------------- Programa principal ----------------
void setup() {
  pinMode(PIN_LED_AZUL, OUTPUT);
  pinMode(PIN_LED_ROJO, OUTPUT);
  pinMode(PIN_LED_AMARILLO, OUTPUT);
  pinMode(PIN_LED_NARANJA, OUTPUT);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_VIBRACION, INPUT);
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);       // Desactiva la microSD del shield

  Serial.begin(9600);
  Wire.begin();
  if (!iniciarBh1750()) Serial.println(F("# Aviso: no se detecta el BH1750"));
  estadoVibracion = digitalRead(PIN_VIBRACION);

  // Prueba de LED al encender: se prenden los cuatro durante 1 s
  digitalWrite(PIN_LED_AZUL, HIGH);
  digitalWrite(PIN_LED_ROJO, HIGH);
  digitalWrite(PIN_LED_AMARILLO, HIGH);
  digitalWrite(PIN_LED_NARANJA, HIGH);
  delay(1000);
  digitalWrite(PIN_LED_AZUL, LOW);
  digitalWrite(PIN_LED_ROJO, LOW);
  digitalWrite(PIN_LED_AMARILLO, LOW);
  digitalWrite(PIN_LED_NARANJA, LOW);

#if USAR_ETHERNET
  MCUSR = 0;
  wdt_disable();
  Ethernet.begin(mac, ipArduino, dnsRed, gatewayRed, mascara);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println(F("# ERROR: no se detecta el Ethernet Shield"));
  }
  servidor.begin();
  Serial.print(F("# Agente Zabbix en "));
  Serial.print(Ethernet.localIP());
  Serial.println(F(":10050"));
  wdt_enable(WDTO_8S);                 // Reinicia si el programa se cuelga
#else
  Serial.println(F("# Modo USB: una linea DATOS cada 5 s"));
#endif

  // El PIR necesita hasta 60 s para estabilizarse tras encender
}

void loop() {
  actualizarLuz();
  actualizarMovimiento();
  actualizarVibracion();
  actualizarRuido();

  if (apagarAzul && (long)(millis() - apagarAzul) >= 0) {
    digitalWrite(PIN_LED_AZUL, LOW);
    apagarAzul = 0;
  }

#if USAR_ETHERNET
  wdt_reset();
  Ethernet.maintain();
  EthernetClient cliente = servidor.available();
  if (cliente) atenderCliente(cliente);
#else
  enviarSerie();
#endif
}
