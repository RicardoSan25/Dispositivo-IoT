# Dispositivo-IoT
Este programa ha sido desarrollado para nuestro trabajo de titulación utilizando PlataformIO con el IDE de Arduino y un SoC ESP32-S3. El proyecto consiste en obtener los datos de tres sensores: DHT22, SDS011 y un detector de nivel de agua. Los valores de estos sensores se publican en la plataforma Thingsboard cada 5 minutos usando el protocolo HTTPS.


Para mejorar el rendimiento y la vida útil del SoC ESP32-S3, se utilizan los modos Sleep y Active. En el modo Sleep, el dispositivo entra en un estado de bajo consumo de energía entre mediciones, mientras que en el modo Active, el dispositivo realiza las mediciones de los sensores y envía los datos a Thingsboard.


Este enfoque permite optimizar el uso de recursos del SoC y prolongar la duración de la batería en aplicaciones autónomas.

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
Esta carpeta contiene toda la programación de nuestro dispositivo IoT. Se han utilizado técnicas de programación avanzadas para optimizar el código y mejorar la eficiencia energética del dispositivo. Para ello, tenemos los diagramas de flujo que dan una mejor explicación de manera didáctica del funcionamiento de nuestro trabajo.
<figure>
  <figcaption>Figura 1: Diagrama de Flujo del Dispositivo IoT</figcaption>
  <img src="Diagrama de Flujo-Dispositivo IoT.png" alt="Diagrama de Flujo del Dispositivo IoT">
  </figure>


<figure>
  <figcaption>Figura 2: Diagrama de Flujo de lectura de los sensores</figcaption>
  <img src="Diagrama de flujo-Lectura sensores.png" alt="Diagrama de Flujo de lectura de los sensores">
</figure>
