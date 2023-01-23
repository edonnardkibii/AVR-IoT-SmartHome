#!/bin/sh
# launcher.sh

sleep 60

cd /
cd home/openhabian
sudo python3 MQTTInfluxDBBridge.py
cd /