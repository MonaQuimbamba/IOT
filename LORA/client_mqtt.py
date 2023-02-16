#!/bin/python3
import paho.mqtt.client as mqtt
import os, ssl, binascii,jwt, base64, subprocess
import colorama
from colorama import Fore
# generate the key with this cmd line :  xxd -p -l 16 -c 16 /dev/urandom

def on_message(client, obj, msg):
    ## clair data from mosquitoo_pub
    data=msg.topic + " " + str(msg.qos) + " " + str(msg.payload)
    print(Fore.YELLOW +"Data received from ESP8266 => ", data)
    print("")
    data =msg.payload 
    data =jwt.encode( {'data':data.decode('utf-8') }, "TMC", algorithm='HS256')
    print(Fore.WHITE + "data converted into JSON => ",data)
    print("")
    cmd = subprocess.Popen("sudo ./RadioHead/examples/raspi/rf95/rf95_client %s "%data, shell=True,stdout=subprocess.PIPE)
    (resultat, ignorer) = cmd.communicate()
    print(Fore.GREEN+"Data sent via LORA")
    print("")
    #print(resultat)  #Data sent 


client_mqtt = mqtt.Client()
# Assign event callbacks
client_mqtt.on_message = on_message
# variables for connexion 
rasp_key  = "/home/pi/CERTIFICATES/ecc.raspberry.key.pem"
rasp_cert = "/home/pi/CERTIFICATES/ecc.raspberry.pem"
ca_cert   = "/home/pi/CERTIFICATES/ecc.ca.pem"
url = 'mqtt.com'
port=8883
topic =  '/esp8266'
# set uo the variables 
client_mqtt.username_pw_set("tmc", "iot")
client_mqtt.tls_set(ca_certs=ca_cert, certfile=rasp_cert, keyfile=rasp_key, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS, ciphers=None)
# connect to 
client_mqtt.connect(url,port)
client_mqtt.subscribe(topic, 0)

# Network loop 
while True:
    client_mqtt.loop()
