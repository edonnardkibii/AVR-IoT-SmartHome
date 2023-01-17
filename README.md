# Smart Home
The following project delves into the automation of data collection of a smart home. <br />
The project implements the MQTT protocol to send data to an MQTT server and the data is sent via Port 1883.

## Publisher:
The publisher is an AVR-IoT. <br />
Link: [AVR-IoT](https://www.microchip.com/en-us/development-tool/AC164160)<br />

The Publisher sends light and temperature data to an MQTT broker

## Subscriber
The subscriber connects to the MQTT Broker via a Python script. The data received is then sent to an InfluxDB database for visualization. <br />

## Threat Modelling:
A full threat analysis of possible risks analyzed using Microsoft's [Threat Modelling Tool](https://www.microsoft.com/en-us/securityengineering/sdl/threatmodeling).

## Embedded Cybersecurity
AES encryption is implemented by the AVR-IoT when publishing data to the MQTT-Broker. The subscriber decrypts the data using the secret key and sends the sensor data to InfluxDB database. <br />
The project also incoporates the use of Base-64 encoding to prevent data loss via the MQTT medium.