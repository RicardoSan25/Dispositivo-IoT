/*          Universidad de Cuenca
Desarrollado por: Ricardo A. Sanchez Cordero y Juan B. Vintimilla Jaramillo

Fecha: 24 de Mayo del 2024

Descripción: Es el programa desarrollado para nuestro trabajo de titulación el cual fue desarrollado en PlataformIO con IDE de Arduino, usando un Soc ESP32-S3.
Este proyecto consiste en tener los datos de 3 sensores como DHT22, SDS011 y Detector del nivel de agua, cuyos valores seran publicados en la plataforma
de Thingsboard usando el protocolo HTTPS cada 5 minutos. Para terminos de mejora en los rendimientos y vida util del Soc, procedemos a usar los dos modos Sleep y activo.


*/

/*
Nota 23-05-2024: Se esta relaizando una prueba con el esp32-S3 dev kit, sin la parte del reinicio del primer arranque ya que nos encontramos
con algunas interferencias que suponemos que sea de su parte ya que se nos esta quedadno es dicha tarea ya que se realizo varias pruebas
y por lo general llega momentos a detectar que es el primer arranque leugo de a ver simulado un corte de luz o cuando realmente hubo una
baja de tension en la misma. En la mayoria de los casos no era asi por lo cual se procedia a reiniciar forzozamente o conectar/desconectar manulamente
dos veces con el fin de poder observar el primer arranque pero esto no es muy coveniente porque a horas de la madrugada el equipo parece que se traba
en alguna tarea de reinicio y no permite su progreso normalmente por ello ahora comenzamos a probar el demas codigo que esta realizado para momentos
de inteerrupcion o que se quede inibido en alguna tarea pueda continuar con normalidad.

Nota 24-05-2023: Dimos con el problema no era la programacion si no contrario era que no tenia suficiente potencia la señal
del wifi de la sala de capacitacion el cual solo uno de los dos dispositivos logra conectarse por el diseño arquitectonico del cuarto con respecto
al otro que no logra ya que este tiene un diseño desfavorable que permite penetracion de la señal por el grosor de paredes que debe atravesar


Nota 31-05-2024: Se esta realizando la prueba uniendo los dos dispositivos en un mismo cuarto en este caso le pasamos el dispositivo C1 al cuarto 006
ahora en el cual estan los dos dispositivos recordar que se modifico el codigo para que solo enviara temperatura,humedad y calidad del aire
*/

#define DEBUG // si descomentamos esto nos imprimiria todo los seriales caso contrario no

// Librerias
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "DHT.h"
#include <SoftwareSerial.h>
#include <Preferences.h>
#include <esp_sleep.h>

// Variables de conexión de pines ESP32-S3
#define DHTPIN 9         // I9 corresponde al GPIO9
#define DHTTYPE DHT22    // DHT 22  (AM2302), AM2321
#define SDS_RX 18        // I18 corresponde al GPIO18
#define SDS_TX 17        // I17 corresponde al GPIO17
#define pin_nivel_agua 5 // I5 corresponde al GPIO5
// Definimos variables globales
int Nivel;                                 // Variables entera para el nivel del agua
int alarma = 0;                            // Variable requerida para saber si es tiempo de enviar
unsigned long lastConnectionAttempt = 0;   // Utilizado como contador en el código
unsigned long lastResetTime = 0;           // Para el temporizador de reinicio
int send_data = 60000 * 5;                 // Tiempo de envio cada 5 minutos ya que cada minuto es igual a 60000mseg
unsigned long reconnectionTimeout = 60000; // 1 minuto en milisegundos
unsigned long reseteoGeneral = 21000000;   //  5 horas  y media en milisegundos
bool wasConnected = false;                 // Variable definida para conocer estado de la conexion WiFi

// Condiciones iniciales para las librerias
SoftwareSerial sds(SDS_RX, SDS_TX);
DHT dht(DHTPIN, DHTTYPE);

// Credenciales de la red WiFi
// const char *ssid = "REDJB";
// const char *password = "19102024";
// const char *ssid = "Laptop-R";
// const char *password = "9876543210";
const char *ssid = "BIBLIO-IOT"; // Nombre de la red a conectarse
// const char *ssid = "BIBLIO-IOT-EXT-2";
const char *password = "Ucuenca.Biblioteca"; // Nombre de la contraseña de la red a conectarse
//  const char *ssid = "FAMILIA SANCORD";
// const char *password = "karina1975";
// const char *httpsServer = "micro-red.ucuenca.edu.ec";
const char *httpsServer = "micro-red.ucuenca.edu.ec"; // Direccion del servidor Thingsboard a conectarse
// const char *ACCESS_TOKEN_Sistema_C1 = "P3re1WwOBMEIpokNMWiZ"; // C1 - Este sera el token para el monitoreo del aula de capacitacion 006 V9RbjlosApMbkTcdCJmu
// const char *ACCESS_TOKEN_Sistema_C1 = "V9RbjlosApMbkTcdCJmu"; // C2 - Este sera el token para el monitoreo del aula de capacitacion 005 P3re1WwOBMEIpokNMWiZ
/*NOTA: Acordarse de modificar el sendData_SistemaIoT*/
const char *ACCESS_TOKEN_Sistema_C1 = "HUeXW33zc57RoedeSIuO"; // Este es el token para demo pruebas sadP24O2LqzohwOmMZoe

//  Certificacion para usar protocolo https del serevidor Thingsboard
const char *rootCACertificate =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
    "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"
    "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"
    "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"
    "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"
    "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"
    "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"
    "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"
    "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"
    "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"
    "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"
    "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"
    "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"
    "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
    "MrY=\n"
    "-----END CERTIFICATE-----\n";

// Inciio del programa esto solo se ejecutara una vez
void setup()
{
  Serial.begin(115200); // Inicio de la comunicacion Serial
  while (!Serial)
  {
    delay(10); // Espera hasta que el puerto serie esté listo
  }
  // Inicializa WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); // Inserta las credenciales antes definidas
#ifdef DEBUG
  Serial.print("Conectando a Wi-Fi...");
#endif
  // Esto permite que intente conectarse a la red
  while (WiFi.status() != WL_CONNECTED)
  {
#ifdef DEBUG
    Serial.print('.');
    delay(1000);
#endif
  }
#ifdef DEBUG
  Serial.println(" Conectado!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
#endif
  lastResetTime = millis(); // Inicializa el tiempo de reinicio
  dht.begin();
  sds.begin(9600);
}

// Función para lectura DHT22
void DHT22_read(float &temperatura, float &humedad)
{

  humedad = dht.readHumidity();
  temperatura = dht.readTemperature();

  // Verificar si la lectura falló
  if (isnan(humedad) || isnan(temperatura))
  {
#ifdef DEBUG
    Serial.println("Error al leer desde el sensor DHT22.");
    return;
#endif
  }
  delay(10); // Sirve para evitar desbordamiento en la CPU

#ifdef DEBUG
             // Imprimimos resultados de la lectura de este sensor
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.print("%  Temperatura: ");
  Serial.print(temperatura);
  Serial.println("°C");
#endif
}

// Función para lectura SDS011
void SDS011_read(float &pm2_5, float &pm_10)
{
  // Verificar si la lectura falló
  while (sds.available() && sds.read() != 0xAA)
  {
  }
  if (sds.available())
  {
    Serial.println("Data available from SDS011...");
  }
  // Una vez que tenemos el byte inicial, intentamos leer los siguientes 9 bytes
  byte buffer[10];
  buffer[0] = 0xAA; // El byte inicial que ya hemos encontrado
  if (sds.available() >= 9)
  {
    sds.readBytes(&buffer[1], 9);
    // Comprueba si el último byte es el byte final correcto
    if (buffer[9] == 0xAB)
    {
      // Usamos desplazamiento de 8 bits a la izquierda y combinarlo con el siguiente byte
      int pm25int = (buffer[3] << 8) | buffer[2];
      int pm10int = (buffer[5] << 8) | buffer[4];
      pm2_5 = pm25int / 10.0;
      pm_10 = pm10int / 10.0;

#ifdef DEBUG
      // Imprimimos resultados de la lectura de este sensor
      Serial.print("PM2.5: ");
      Serial.print(pm2_5, 2); // 2 decimales
      Serial.print(" µg/m³   ");
      Serial.print("PM10: ");
      Serial.print(pm_10, 2); // 2 decimales
      Serial.println(" µg/m³   ");
#endif
    }
    else
    {
      Serial.println("Byte final no válido de SDS011.");
    }
  }
  else
  {
    Serial.println("No hay suficientes datos de SDS011.");
  }
  delay(10); // Sirve para evitar desbordamiento en la CPU
}

// Función para lectura del detector nivel de agua
void nivel_agua_read()
{
  // Iniciamos con la lectura analogica
  int adc_level = analogRead(pin_nivel_agua);
  // Esteblecemo algunos caso segun su lectura
  if (adc_level < 500)
  {
    Nivel = 1;
  }
  if (adc_level > 500)
  {
    Nivel = 2;
  }
  if (adc_level > 1800)
  {
    Nivel = 3;
  }
  delay(10); // Sirve para evitar desbordamiento en la CPU
#ifdef DEBUG
  // Imprimimos resultados de la lectura de este sensor
  Serial.print("Nivel de tanque: ");
  Serial.println(Nivel);
  Serial.println(adc_level);
#endif
}

// Funcion para envio de informacion hacia el servidor de Thingsboard
void sendData_SistemaIoT(const String &accessToken, float temperatura, float humedad, float pm2_5, float pm_10, int Nivel, int adc_level)
{
  WiFiClientSecure *client = new WiFiClientSecure;

  if (client)
  {
    client->setCACert(rootCACertificate); // Verifica si contamos con la certificación del sitio web
    HTTPClient https;                     // Uso del procolo https seguro

    Serial.println("[HTTPS] Iniciando...");

    if (https.begin(*client, "https://micro-red.ucuenca.edu.ec/api/v1/" + accessToken + "/telemetry")) // Ingreso del dominio o URL
    {
      // Enviamos en formato JSON al Thingsboard la informacion
      String payload = "{\"tem\":" + String(temperatura) + ",\"hum\":" + String(humedad) + ",\"pm25\":" + String(pm2_5) + ",\"pm10\":" + String(pm_10) + ",\"nwl\":" + String(Nivel) + ",\"lvl\":" + String(adc_level) + "}";

      Serial.println("[HTTPS] POST...");
      int httpCode = https.POST(payload);
      // Esta condicion sirve para ver si estamos conectados al Thingsboard
      if (httpCode > 0)
      {

        Serial.printf("[HTTPS] Código de respuesta: %d\n", httpCode);
        // Esta condicion se cumple al respondernos el servidor un code 200, indica conexión exitosa
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          String response = https.getString();
          Serial.println("[HTTPS] Respuesta del servidor:");
          Serial.println(response);
        }
      }
      else
      {
        Serial.printf("[HTTPS] Error al enviar datos: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    }
    else
    {
      Serial.println("[HTTPS] No se pudo iniciar.");
    }
  }
  else
  {
    Serial.println("[HTTPS] No se pudo conectar.");
  }
  Serial.println();
}

void loop()
{
  // Definicion de variables locales
  float temperatura, humedad;
  float pm2_5, pm_10;
  unsigned long currentMillis = millis();

  // Esta condicion se ingresa cada cada 3 minutos para analizar si tenemos conexión WiFi
  if (alarma == 180000)
  {
    // Analiza el estado de la red
    if (WiFi.status() == WL_CONNECTED)
    {
      wasConnected = true;
      lastConnectionAttempt = millis(); // Actualiza el tiempo del último intento exitoso
    }
    else
    {
#ifdef DEBUG
      Serial.println("Wi-Fi desconectado...");
#endif
      // Esta condicion permite reconectarse a la red WiFi
      if (wasConnected)
      {
        // Intentar reconectar inmediatamente si se acaba de desconectar
        WiFi.begin(ssid, password); // Entramos a ala libreria WiFi para compara nuestras credenciales
        wasConnected = false;
        lastConnectionAttempt = millis(); // Actualiza el tiempo del último intento de reconexión
      }
      // Verificar si ha pasado un minuto desde el último intento de conexión
      if (wasConnected == false && currentMillis - lastConnectionAttempt >= reconnectionTimeout)
      {
#ifdef DEBUG
        Serial.println("No se pudo reconectar en un minuto. Reiniciando...");
#endif
        esp_sleep_enable_timer_wakeup(3 * 60 * 1000000); // 3 minutos en microsegundos
        esp_deep_sleep_start();                          // Se levanta el dispositivo
      }
    }
  }

  // Verificar si ha pasado una hora desde el último reinicio entra en modo ahorro de energia
  if (currentMillis - lastResetTime >= reseteoGeneral)
  {
    esp_sleep_enable_timer_wakeup(2 * 60 * 1000000); // 2 minutos en microsegundos de descanso
    esp_deep_sleep_start();                          // Se levanta el dispositivo
  }

  // Esta condicion permite que se envie un primer dato para comprobar la conexión del Dispositivo
  if (alarma < 1000)
  {
    delay(2000);                                                                                        // Retardo
    DHT22_read(temperatura, humedad);                                                                   // LLamamos a la función de lectura del sensor DHT22
    SDS011_read(pm2_5, pm_10);                                                                          // LLamamos a la función de lectura del sensor SDS011
    nivel_agua_read();                                                                                  // LLamamos a la función de lectura del sensor SE045
    int adc_level = analogRead(pin_nivel_agua);                                                         // Se define una variable de lectura analogica como entero
    sendData_SistemaIoT(ACCESS_TOKEN_Sistema_C1, temperatura, humedad, pm2_5, pm_10, Nivel, adc_level); // LLamamos a la funcion de envio a Thingsboard integrando los campos requeridos
  }
  // Esta condicion permite que se envie el dato cada 5 minutos
  if (alarma == send_data)
  {
    delay(2000);                                // Retardo
    DHT22_read(temperatura, humedad);           // LLamamos a la función de lectura del sensor DHT22
    SDS011_read(pm2_5, pm_10);                  // LLamamos a la función de lectura del sensor SDS011
    nivel_agua_read();                          // LLamamos a la función de lectura del sensor SE045
    int adc_level = analogRead(pin_nivel_agua); // Se define una variable de lectura analogica como entero
    alarma = 1000;                              // Definimos a la variable de alarma, con un valor que no ingrese nuevamente a la condicion anterior
    delay(10);                                  // Sirve para evitar desbordamiento en la CPU

    // Esta condicion me sirve si detecta que el nivel del desumificador esta por llenarse y nos sirve para entender su comportamiento
    if (Nivel >= 2)
    {
      // Analiza el estado de la red
      if (WiFi.status() == WL_CONNECTED)
      {
        wasConnected = true;
        sendData_SistemaIoT(ACCESS_TOKEN_Sistema_C1, temperatura, humedad, pm2_5, pm_10, Nivel, adc_level); // LLamamos a la funcion de envio a Thingsboard integrando los campos requeridos
      }
      else
      {
        esp_sleep_enable_timer_wakeup(1 * 60 * 1000000); // 3 minutos en microsegundos de descanso
        esp_deep_sleep_start();                          // Se levanta el dispositivo
      }
    }
  }

  delay(3000);            // Retardo
  alarma = alarma + 2500; // Definimos el incremento de la variable alarma para cumplir con lo requerido
#ifdef DEBUG
  Serial.print("Valor de la alarma es: ");
  Serial.println(alarma);
  Serial.print("Valor de la millises: ");
  Serial.println(currentMillis);
#endif
  delay(10); // Sirve para evitar desbordamiento en la CPU
}
