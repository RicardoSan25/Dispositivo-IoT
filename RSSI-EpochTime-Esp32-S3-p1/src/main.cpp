/*          Universidad de Cuenca
Desarrollado por: Ricardo A. Sanchez Cordero y Juan B. Vintimilla Jaramillo

Fecha: 24 de Mayo del 2024

Descripción: Este codigo permite conectarse a un servidor NTP, el cual nos da una hora de referencia desde 1970 el cual nos servira
para poder encontrar latencia entre el servidor local de Thingsboard y nuestro dispositivo. Además nos mide el RSSI de AP al
cual estemos conectandonos

*/

#define DEBUG // si descomentamos esto nos imprimiria todo los seriales caso contrario no

// Librerias
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "DHT.h"
// #include <SoftwareSerial.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <stdint.h>

// Variables

unsigned long lastResetTime = 0; // Para el temporizador de reinicio

// WiFi credentials
const char *ssid = "SSID";
const char *password = "Password";
const char *httpsServer = "IP-servidor";       // IP del thingsboard en dokcer local 
const char *ACCESS_TOKEN_Sistema_C1 = "TOKEN"; // Token del dispositivo

int identificador = 0;

// Configura el cliente NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ec.pool.ntp.org", 0, 60000); // Conexion del RTC al servidor NTP de Ecuador

// Función para obtener el tiempo en milisegundos desde el epoch
uint64_t getEpochTimeInMilliseconds()
{
  struct timeval tv;                                               // Estructura para almacenar el tiempo
  gettimeofday(&tv, nullptr);                                      // Obtiene el tiempo actual
  return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000; // Devuelve el tiempo en milisegundos
}

// Función para sincronizar el reloj interno (RTC) con el tiempo NTP
void syncRTCWithNTP()
{
  if (WiFi.status() == WL_CONNECTED) // Verifica si hay conexión Wi-Fi
  {
    timeClient.update();                           // Actualiza el cliente NTP
    time_t epochTime = timeClient.getEpochTime();  // Obtiene el tiempo NTP en formato epoch
    struct tm *ptm = gmtime((time_t *)&epochTime); // Convierte el tiempo epoch a tiempo UTC

    struct timeval tv;
    tv.tv_sec = epochTime;      // Configura los segundos del tiempo actual
    tv.tv_usec = 0;             // Configura los microsegundos del tiempo actual
    settimeofday(&tv, nullptr); // Establece el tiempo del sistema

    Serial.print("RTC sincronizado con NTP: ");
    Serial.println(epochTime); // Muestra el tiempo sincronizado

    struct timeval now;
    gettimeofday(&now, NULL); // Obtiene el tiempo actual

    Serial.print("Milisegundos desde el epoch: ");
    Serial.println(getEpochTimeInMilliseconds()); // Muestra el tiempo en milisegundos desde el epoch
  }
  else
  {
    Serial.println("Wi-Fi no conectado. No se puede sincronizar RTC con NTP."); // Muestra un mensaje si no hay conexión Wi-Fi
  }
}

// Configuración inicial
void setup()
{
  Serial.begin(115200); // Inicializa la comunicación serie a 115200 baudios
  while (!Serial)
  {
    delay(10); // Espera hasta que el puerto serie esté listo
  }

  // Inicializa el Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

#ifdef DEBUG
  Serial.print("Conectando a Wi-Fi...");
#endif

  while (WiFi.status() != WL_CONNECTED) // Espera hasta que la conexión Wi-Fi esté establecida
  {
    Serial.print('.');
    delay(1000);
  }

#ifdef DEBUG
  Serial.println(" ¡Conectado!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
#endif

  lastResetTime = millis(); // Inicializa el tiempo de reinicio

  timeClient.begin();  // Inicia el cliente NTP
  timeClient.update(); // Actualiza el tiempo NTP

  syncRTCWithNTP(); // Sincroniza el RTC con el tiempo NTP
}

// Función para leer la intensidad de la señal Wi-Fi (RSSI)
int32_t RSSI_read()
{
  int32_t rssi = WiFi.RSSI(); // Obtiene el RSSI actual
  Serial.print("Potencia de la señal WiFi (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm"); // Muestra el RSSI
  return rssi;            // Devuelve el RSSI
}

// Función para enviar datos al sistema IoT
void sendData_SistemaIoT(const String &accessToken, int32_t rssi, int identificador)
{
  WiFiClient client;                                // Crea un cliente Wi-Fi
  int64_t epochTime = getEpochTimeInMilliseconds(); // Obtiene el tiempo actual en milisegundos desde el epoch
  if (client.connect(httpsServer, 80))              // Conecta al servidor HTTP
  {
    HTTPClient http;

    Serial.println("[HTTP] Iniciando...");

    if (http.begin(client, String("http://") + httpsServer + "/api/v1/" + accessToken + "/telemetry")) // Inicia la conexión HTTP
    {
      // Construye el payload JSON con los datos
      String payload = "{\"time\":\"" + String(epochTime) + "\",\"RSSI\":" + String(rssi) + ",\"ID\":" + String(identificador) + "}";
      Serial.print("Timestamp en Base64: ");
      Serial.println(epochTime);
      Serial.print("Hora del RTC: ");
      Serial.println(timeClient.getFormattedTime());
      Serial.println("[HTTP] POST...");
      int httpCode = http.POST(payload); // Envía una solicitud POST con el payload

      if (httpCode > 0) // Verifica si la solicitud fue exitosa
      {
        Serial.printf("[HTTP] Código de respuesta: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          String response = http.getString(); // Obtiene la respuesta del servidor
          Serial.println("[HTTP] Respuesta del servidor:");
          Serial.println(response);
        }
      }
      else
      {
        Serial.printf("[HTTP] Error al enviar datos: %s\n", http.errorToString(httpCode).c_str()); // Muestra el error si la solicitud falló
      }

      http.end(); // Finaliza la conexión HTTP
    }
    else
    {
      Serial.println("[HTTP] No se pudo iniciar."); // Muestra un mensaje si no se pudo iniciar la conexión HTTP
    }
  }
  else
  {
    Serial.println("[HTTP] No se pudo conectar."); // Muestra un mensaje si no se pudo conectar al servidor
  }
  Serial.println();
}

// Bucle principal
void loop()
{
  if (identificador <= 499) // Envía datos solo si el identificador es menor o igual a 499
  {
    delay(2500);                                                              // Espera 2.5 segundos
    sendData_SistemaIoT(ACCESS_TOKEN_Sistema_C1, RSSI_read(), identificador); // Envía los datos al sistema IoT
    delay(10);                                                                // Espera 10 milisegundos para evitar sobrecarga en la CPU
    identificador++;                                                          // Incrementa el contador en cada iteración
  }
}
