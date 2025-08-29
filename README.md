# Fundamentos y aplicación del módulo MFRC-522 (ESP32 + MQTT)

## 🎯 Objetivo General
Implementar y documentar un sistema RFID con MFRC-522 y ESP32, que lea tarjetas MIFARE y publique eventos en JSON vía MQTT, integrando señales visuales con LED RGB y considerando aspectos éticos.

---

## 📌 Objetivos Específicos
- Explicar el principio de RFID y su rol en sistemas inteligentes.  
- Instruir paso a paso la instalación y configuración (hardware y software).  
- Incluir ejemplos de código y resultados de prueba (Serial + MQTT).  

---

## 🧠 Competencias
- Manejo de protocolos de comunicación **SPI** y **MQTT**.  
- Integración de hardware (ESP32 + MFRC-522 + LED RGB).  
- Programación en **C++ para sistemas embebidos**.  
- Documentación técnica clara y profesional.  
- Evaluación crítica de aspectos éticos en RFID.  

---

## 📑 Tabla de Contenidos
1. [Descripción](#-descripción)  
2. [Requisitos](#-requisitos)  
3. [Instalación y Configuración](#-instalación-y-configuración)  
4. [Conexiones de Hardware](#-conexiones-de-hardware)  
5. [Parámetros Técnicos del MFRC-522](#-parámetros-técnicos-del-mfrc-522)  
6. [Uso y ejemplos de Código](#-uso-y-ejemplos-de-código)  
7. [Resultados de Prueba](#-resultados-de-prueba)  
8. [Consideraciones Éticas y de Seguridad](#-consideraciones-éticas-y-de-seguridad)  
9. [Formato de Salida (JSON)](#-formato-de-salida-json)  
10. [Solución de Problemas](#-solución-de-problemas)  
11. [Diagrama de Flujo](#-diagrama-de-flujo)  
12. [Contribuciones](#-contribuciones)  
13. [Referencias](#-referencias)  

---

## 📖 Descripción
La práctica consiste en diseñar y evaluar un sistema RFID con el módulo **MFRC-522** integrado a una **ESP32**, mostrando la autenticación de tarjetas mediante indicadores visuales (**LED RGB**) y el envío de eventos a través de **MQTT**.  

Este sistema aplica los conceptos de sistemas inteligentes para control de accesos y registro de personas u objetos.  

---

## 🛠️ Requisitos
**Hardware:**  
- ESP32 DevKit v1  
- Módulo RFID MFRC-522  
- Tarjetas o llaveros MIFARE  
- LED RGB con resistencias de 220 Ω  
- Protoboard y cables Dupont  

**Software:**  
- Arduino IDE  
- Librerías: `MFRC522`, `WiFi`, `PubSubClient`, `ArduinoJson`  
- Cliente MQTT (ej. [MQTT Explorer](https://mqtt-explorer.com/))  

---

## ⚙️ Instalación y Configuración
1. Instalar **Arduino IDE**.  
2. Añadir el **core de ESP32** desde el Gestor de tarjetas.  
3. Instalar librerías necesarias (`MFRC522`, `PubSubClient`, `ArduinoJson`).  
4. Configurar en el código:  
   - `WIFI_SSID` y `WIFI_PASS`.  
   - `EQUIPO = "JFMD-KAVG"` (topic MQTT).  
5. Subir el sketch al ESP32.  
6. Conectarse con **MQTT Explorer** al broker público:  
   - Host: `test.mosquitto.org`  
   - Port: `1883`  
   - Topic: `JFMD-KAVG`  

---

## 🔌 Conexiones de Hardware

### ESP32 ↔ MFRC-522
| MFRC-522 | ESP32 |
|----------|-------|
| SDA (SS) | GPIO 5 |
| SCK      | GPIO 18 |
| MOSI     | GPIO 23 |
| MISO     | GPIO 19 |
| RST      | GPIO 22 |
| VCC      | 3.3V |
| GND      | GND |

### LED RGB (cátodo común)
- R → GPIO 25  
- G → GPIO 26  
- B → GPIO 27  
- Común → GND  

*(Si tu LED es ánodo común: común → 3.3V y activa `LED_COMMON_ANODE = true` en el código)*  

---

## 📊 Parámetros Técnicos del MFRC-522
| Parámetro         | Valor |
|-------------------|-------|
| Voltaje           | 3.3 V |
| Corriente típica  | 13–26 mA |
| Frecuencia        | 13.56 MHz |
| Interfaz          | SPI |
| Alcance           | 2–5 cm |

---

## 💻 Uso y ejemplos de Código
1. Encender **Serial Monitor** (115200).  
2. Acercar una tarjeta MIFARE al lector.  
3. Observar en consola el JSON generado.  
4. Verificar en **MQTT Explorer** la llegada del mensaje.  

📌 El código distingue 4 escenarios:  
- Entrada  
- Salida  
- Tarjeta sin pase  
- Tarjeta rechazada  

---

## 📷 Resultados de Prueba
Se verificaron los siguientes casos:  

- ✅ **Entrada:** tarjeta autorizada, primer pase → LED color integrante → JSON `"entrada"`.  
- ✅ **Salida:** mismo UID, segundo pase → LED mismo color → JSON `"salida"`.  
- ✅ **Sin pase:** tercer intento tras ciclo → LED blanco → JSON `"tarjeta sin pase"`.  
- ✅ **Rechazada:** tarjeta no registrada → LED rojo → JSON `"tarjeta rechazada"`.  

📷 *Inserta aquí capturas de: montaje, Serial Monitor, MQTT Explorer.*  

---

## 🔒 Consideraciones Éticas y de Seguridad
- **Privacidad:** los UIDs son identificadores únicos y no deben compartirse sin consentimiento.  
- **Seguridad:** riesgo de clonación de tarjetas sin cifrado.  
- **Mitigación:** uso de protocolos seguros (TLS, cifrado en tarjetas).  
- **Uso responsable:** esta práctica es únicamente con fines académicos.  

---

## 📤 Formato de Salida (JSON)

```json
{
  "nombreEquipo": "JFMD-KAVG",
  "nombreIntegrante": "Nombre o 'Desconocido'",
  "id": "UID en HEX",
  "evento": {
    "accion": "entrada|salida|tarjeta rechazada|tarjeta sin pase",
    "fecha": "DD/MM/AAAA",
    "hora": "HH:MM:SS"
  }
}

# Diagrama

    A[Inicio] --> B[Wi-Fi · MQTT · NTP · MFRC-522 · LED]
    B --> C{¿Tarjeta detectada?}
    C -- No --> C
    C -- Sí --> D[Leer UID]

    D --> E{¿UID registrado?}
    E -- No --> F[LED Rojo · JSON: "tarjeta rechazada"]
    F --> C

    E -- Sí --> G{Estado previo}
    G -- NONE o SALIDA --> H[Acción: ENTRADA · LED color integrante]
    H --> I[Publicar JSON "entrada"]

    G -- ENTRADA --> J[Acción: SALIDA · LED color integrante]
    J --> K[Publicar JSON "salida"]

    I --> L{¿Vuelve a pasar tras ciclo?}
    J --> L
    L -- No --> C
    L -- Sí --> M[LED Blanco · JSON: "tarjeta sin pase"]
    M --> C

