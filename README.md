# Dispositivo-IoT
  Es el programa desarrollado para nuestro trabajo de titulación el cual fue desarrollado en PlataformIO con IDE de Arduino, usando un Soc ESP32-S3.
Este proyecto consiste en tener los datos de 3 sensores como DHT22, SDS011 y Detector del nivel de agua, cuyos valores seran publicados en la plataforma
de Thingsboard usando el protocolo HTTPS cada 5 minutos. Para terminos de mejora en los rendimientos y vida util del Soc, procedemos a usar los dos modos Sleep y Active.

## 1. Análisis_de_datos_Tesis-Python
  En esta carpeta contendremos los programas en Python, que fueron de utilidad para generar las gráficas de cada medición y correlación entre cada una de sus mediciones. Estas gráficas se generaron a partir de archivos en formato JSON.
## 2. RSSI-EpochTime-ESP32-S3-p1
  En esta carpeta se encuentra el programa desarrollado en PlataformIO con el IDE de Arduino, para el SoC ESP32-S3. El programa se encarga de enviar una cantidad de paquetes de información al Docker de Thingsboard utilizando el protocolo HTTP.  

  
  Cada paquete enviado contiene la siguiente información:
* Identificador
* Potencia de la red WiFi
* Epoch timeen milisegundos
Toda esta información se envía en formato JSON.


El proceso de ejecución del programa es el siguiente:
1. Conexión a la red WiFi
2. Sincronización con el servidor NTP "ec.pool.ntp.org"
3. Envío de una cantidad de paquetes con su respectivo identificador al servidor de Thingsboard

## 3. V2-Tesis
Esta carpeta contiene toda la programación de nuestro dispositivo IoT. Se han utilizado técnicas de programación avanzadas para optimizar el código y mejorar la eficiencia energética del dispositivo.
<figure>
  <figcaption>Figura 1: Diagrama de Flujo del Dispositivo IoT</figcaption>
  <img src="Diagrama de Flujo-Dispositivo IoT.png" alt="Diagrama de Flujo del Dispositivo IoT">
  </figure>


<figure>
  <figcaption>Figura 2: Diagrama de Flujo de lectura de los sensores</figcaption>
  <img src="Diagrama de flujo-Lectura sensores.png" alt="Diagrama de Flujo de lectura de los sensores">
</figure>
