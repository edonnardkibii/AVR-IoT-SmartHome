# Smart Home
The following projects goes into details on how to automate your home. <br />
The projects implements the MQTT protocol to send data to an MQTT server. Data is sent via Port 1883.

## Publisher:
The publisher is an AVR-IoT device. <br />
Link: [AVR-IoT](https://www.microchip.com/en-us/development-tool/AC164160 <br />

The Publisher sends to light and temperature data to an MQTT broker

## Subscriber
The subscriber connects to the MQTT Broker via a Python script. The data received is then sent to an InfluxDB database <br />

## Threat Modelling:
A full threat of the possible threats is produced and analyzed.

## Embedded Cybersecurity
AES encryption is implemented by the AVR-IoT when publishing data to the MQTT-Broker. The subscriber decrypts the data using the secret key and sends the sensor data to InfluxDB database. <br />
The project also incoporates the use of BAse-64 encoding to send data via MQTT.