
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

/* ====================== CONFIG ====================== */
// Wi-Fi
const char* WIFI_SSID = "iPhone_de_Fernando";   
const char* WIFI_PASS = "tobias1560";         

// MQTT
const char* MQTT_BROKER = "test.mosquitto.org";
const uint16_t MQTT_PORT = 1883;

// nombreEquipo = TOPIC MQTT (requisito)
const char* EQUIPO = "JFMD-KAVG";               

// NTP (CDMX)
const char* TZ_MX = "CST6CDT,M3.2.0/2,M11.1.0/2";
const char* NTP_SERVER = "pool.ntp.org";

// MFRC522 (ESP32 VSPI)
#define PIN_SS   5
#define PIN_RST  22
// VSPI: SCK=18, MISO=19, MOSI=23

// LED RGB (PINES)
#define PIN_R 25
#define PIN_G 26
#define PIN_B 27

// Tipo de LED 
bool LED_COMMON_ANODE = false; 

/* ============== Tipos y datos de la APP ============== */

struct RGB { uint8_t r,g,b; };

// UIDs autorizados 
const char* UID1 = "35 F2 B3 02";  
const char* UID2 = "7E 20 BC 02";   

// Nombres para el JSON
const char* NOMBRE1 = "Jesus Fernando Moran Denis";
const char* NOMBRE2 = "Karen Arely Velazco Gonzalez";

// Paleta de colores por integrante
RGB COLOR1 = {0, 255, 0};    // verde
RGB COLOR2 = {0, 0, 255};    // azul
RGB COLOR_RECHAZADA = {255, 0, 0};
RGB COLOR_SIN_PASE  = {255, 255, 255};

// Estados por tarjeta
enum Accion { NONE, ENTRADA, SALIDA };
struct EstadoTarjeta { Accion ultimo; bool cicloCompleto; };
EstadoTarjeta estado1 = {NONE, false};
EstadoTarjeta estado2 = {NONE, false};
EstadoTarjeta estado3 = {NONE, false};

/* ============== Objetos globales ============== */
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
MFRC522 rfid(PIN_SS, PIN_RST);

/* ============== LED helpers============== */
static inline uint8_t inv(uint8_t v){
  return LED_COMMON_ANODE ? (uint8_t)(255 - v) : v;
}
void setRGB(uint8_t r,uint8_t g,uint8_t b){
  analogWrite(PIN_R, inv(r));
  analogWrite(PIN_G, inv(g));
  analogWrite(PIN_B, inv(b));
}
void setupLedPwm() {
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  setRGB(0,0,0);  
}
// “Adaptador” para seguir usando RGB struct en el resto del código
void setLed(RGB c){ setRGB(c.r, c.g, c.b); }

/* ============== Conectividad / utilidades ============== */
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(400); }
}
void connectMQTT() {
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  while (!mqttClient.connected()) {
    String cid = String("ESP32-") + String((uint32_t)ESP.getEfuseMac(), HEX);
    mqttClient.connect(cid.c_str());
    if (!mqttClient.connected()) delay(800);
  }
}
void setupNTP(){ configTzTime(TZ_MX, NTP_SERVER); delay(1500); }

String uidToString(MFRC522::Uid* uid) {
  String s; s.reserve(uid->size*3);
  for (byte i=0;i<uid->size;i++){ if(i)s+=' '; if(uid->uidByte[i]<0x10)s+='0'; s+=String(uid->uidByte[i],HEX); }
  s.toUpperCase(); return s;
}
bool equalsUid(const String& a, const char* b){ return a.equalsIgnoreCase(String(b)); }

EstadoTarjeta* getEstadoPtr(const String& s){
  if (equalsUid(s, UID1)) return &estado1;
  if (equalsUid(s, UID2)) return &estado2;
  if (equalsUid(s, UID3)) return &estado3;
  return nullptr;
}
const char* getNombre(const String& s){
  if (equalsUid(s, UID1)) return NOMBRE1;
  if (equalsUid(s, UID2)) return NOMBRE2;
  return "Desconocido";
}
RGB getColorIntegrante(const String& s){
  if (equalsUid(s, UID1)) return COLOR1;
  if (equalsUid(s, UID2)) return COLOR2;
  return COLOR_RECHAZADA;
}
Accion proximaAccionValida(EstadoTarjeta* e){
  if (e->ultimo == NONE || e->ultimo == SALIDA) return ENTRADA;
  return SALIDA;
}
void getDateTime(String& fecha, String& hora){
  time_t now=time(nullptr); struct tm tinfo; localtime_r(&now,&tinfo);
  char f[11], h[9]; strftime(f,sizeof(f),"%d/%m/%Y",&tinfo); strftime(h,sizeof(h),"%H:%M:%S",&tinfo);
  fecha=f; hora=h;
}

// Publica JSON (Serial + MQTT, topic = EQUIPO)
void publicarEvento(const String& nombreIntegrante, const String& uid, const char* accion){
  String fecha, hora; getDateTime(fecha, hora);
  StaticJsonDocument<256> doc;
  doc["nombreEquipo"] = EQUIPO;
  doc["nombreIntegrante"] = nombreIntegrante;
  doc["id"] = uid;
  JsonObject ev = doc.createNestedObject("evento");
  ev["accion"] = accion;    
  ev["fecha"]  = fecha;     
  ev["hora"]   = hora;        

  char buffer[384]; size_t n = serializeJson(doc, buffer);
  Serial.println(buffer);
  mqttClient.publish(EQUIPO, (uint8_t*)buffer, n);
}

/* ====================== SETUP / LOOP ====================== */
void setup() {
  Serial.begin(115200);
  setupLedPwm(); setLed({0,0,0});

  connectWiFi();
  setupNTP();
  connectMQTT();

  // SPI hardware (VSPI) con pines explícitos
  SPI.begin(18, 19, 23, PIN_SS);
  rfid.PCD_Init(PIN_SS, PIN_RST);
  delay(50);

  Serial.println("RFID listo. Acerca una tarjeta...");
}

void loop() {
  if (WiFi.status()!=WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected())     connectMQTT();
  mqttClient.loop();

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uidStr = uidToString(&rfid.uid); uidStr.trim();
  EstadoTarjeta* e = getEstadoPtr(uidStr);
  const char* nombre = getNombre(uidStr);

  // No registrada -> rechazada
  if (!e) {
    setLed(COLOR_RECHAZADA);
    publicarEvento(nombre, uidStr, "tarjeta rechazada");
    rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
    delay(700); setLed({0,0,0}); return;
  }

  // Tras ciclo ENTRADA->SALIDA, cualquier pase = “tarjeta sin pase”
  if (e->cicloCompleto) {
    setLed(COLOR_SIN_PASE);
    publicarEvento(nombre, uidStr, "tarjeta sin pase");
    rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
    delay(700); setLed({0,0,0}); return;
  }

  // Validación: no permitir dos ENTRADAS o dos SALIDAS seguidas
  Accion siguiente = proximaAccionValida(e);
  if (siguiente == ENTRADA) {
    if (e->ultimo == ENTRADA) {
      setLed(getColorIntegrante(uidStr));
      publicarEvento(nombre, uidStr, "entrada"); 
    } else {
      e->ultimo = ENTRADA;
      setLed(getColorIntegrante(uidStr));
      publicarEvento(nombre, uidStr, "entrada");
    }
  } else { // SALIDA
    if (e->ultimo == SALIDA) {
      setLed(getColorIntegrante(uidStr));
      publicarEvento(nombre, uidStr, "salida");
    } else {
      e->ultimo = SALIDA;
      setLed(getColorIntegrante(uidStr));
      publicarEvento(nombre, uidStr, "salida");
      e->cicloCompleto = true; 
    }
  }

  rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
  delay(600); setLed({0,0,0});
}
