#!/bin/python3
import paho.mqtt.client as mqtt

def client():
    broker_url = "test.mosquitto.org"
    port = 1883
    client =  mqtt.Client()
    client.connect(broker_url, port)
    data = input("Saisisez votre message :")
    topic = "hitopic"
    client.publish(topic,data)
    client.disconnect()
    


def main():
    print("cc")
    client()

if __name__=="__main__":
    main()

