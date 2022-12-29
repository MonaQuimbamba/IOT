#!bin/python3

import paho.mqtt.client as mqtt

def message(client, userdata, message):
    print("Message: ", message.payload)

def receive_client():
    broker_url = "test.mosquitto.org"
    port = 1883
    r_client = mqtt.Client()
    r_client.on_message=message

    r_client.connect(broker_url,port)
    r_client.subscribe("hitopic")
    r_client.loop_forever()


def main():
    receive_client()

if __name__=="__main__":
    main()


