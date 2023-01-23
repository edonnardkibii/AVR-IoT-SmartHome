import re
from typing import NamedTuple

import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient

import base64
from Crypto.Cipher import AES
from base64 import b64decode
# from Crypto.Util.Padding import unpad

import configparser

config = configparser.ConfigParser()
config.read('config.ini')

try:
    key = bytes.fromhex(config['login']['aes_key'])
    iv = bytes.fromhex(config['login']['aes_iv'])
    block_size = AES.block_size
    #cipher = AES.new(key, AES.MODE_CBC, iv)
	
except (ValueError, KeyError):
	print("Incorrect Decryption Keys")


INFLUXDB_ADDRESS = 'localhost'
INFLUXDB_USER = config['login']['influxdb_user']
INFLUXDB_PASSWORD = config['login']['influxdb_password']
INFLUXDB_DATABASE = 'sensor_data'

MQTT_ADDRESS = 'localhost'
MQTT_USER = config['login']['mqtt_user']
MQTT_PASSWORD = config['login']['mqtt_password']
MQTT_TOPIC = 'mchp/+/+'
# MQTT_REGEX = 'mchp/([^/]+)/([^/]+)'
MQTT_CLIENT_ID = 'MQTTInfluxDBBridge'

influxdb_client = InfluxDBClient(INFLUXDB_ADDRESS, 8086, INFLUXDB_USER, INFLUXDB_PASSWORD, None)

class SensorData(NamedTuple):
    location: str
    measurement: str
    value: float

def unpad(plain_text):
    last_character = plain_text[len(plain_text) - 1:]
    bytes_to_remove = ord(last_character)
    return plain_text[:-bytes_to_remove]

def decrypt(encrypted_text):
    cipher = AES.new(key, AES.MODE_CBC, iv)
    base64_decrypt = base64.b64decode(encrypted_text)
    try:    
        plain_text = cipher.decrypt(base64_decrypt).decode('utf-8')
        print('Decrypted Message: ' + str(plain_text))
        return unpad(plain_text)
    except:
        print('Undecipherable value')
        return False
    

def on_connect(client, userdata, flags, rc):
    """ The callback for when the client receives a CONNACK response from the server."""
    print('Connected with result code ' + str(rc))
    client.subscribe(MQTT_TOPIC)

def _parse_mqtt_message(payload):
    payload_values = payload.split(",")
    location = 'james_room'
    if payload_values:
        light_intensity = SensorData(location=location, measurement='light_intensity', value=float(payload_values[0]))
        temperature = SensorData(location=location, measurement='temperature', value=float(payload_values[1]))
        return light_intensity, temperature
    else:
        return None, None

def _send_sensor_data_to_influxdb(sensor_data):
    json_body = [
        {
            'measurement': sensor_data.measurement,
            'tags': {
                'location': sensor_data.location
            },
            'fields': {
                'value': sensor_data.value
            }
        }
    ]
    influxdb_client.write_points(json_body)

def on_message(client, userdata, msg):
    """The callback for when a PUBLISH message is received from the server."""
    print(msg.topic + ' ' + str(msg.payload))
    
    # ctx = b64decode(msg.payload)
    # decrypted_payload = cipher.decrypt(ctx)
    # payload_values = decrypted_payload
    if msg.payload:
        decrypted_data = decrypt(msg.payload)
        # print("Decrypted Payload: " +str(decrypted_values))
    
        if decrypted_data is not False:
            light_intensity, temperature = _parse_mqtt_message(decrypted_data)
            if light_intensity and temperature is not None:
                _send_sensor_data_to_influxdb(light_intensity)
                _send_sensor_data_to_influxdb(temperature)
        

def _init_influxdb_database():
    databases = influxdb_client.get_list_database()
    if len(list(filter(lambda x: x['name'] == INFLUXDB_DATABASE, databases))) == 0:
        influxdb_client.create_database(INFLUXDB_DATABASE)
    influxdb_client.switch_database(INFLUXDB_DATABASE)

def main():
    _init_influxdb_database()

    mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
    mqtt_client.username_pw_set(MQTT_USER, MQTT_PASSWORD)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message

    mqtt_client.connect(MQTT_ADDRESS, 1883)
    mqtt_client.loop_forever()


if __name__ == '__main__':
    print('MQTT to InfluxDB bridge')
    main()

