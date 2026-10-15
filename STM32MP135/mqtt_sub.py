import paho.mqtt.client as mqtt
 
BROKER ="broker.hivemq.com"
PORT =1883
TOPIC ="testing/PES/Sub"
 
def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code==0:
                print("connected to broker")
                client.subscribe(TOPIC, qos=1)
        else:
                print(f"commect failed: {reason_code}")
 
def on_message(client, userdata, msg):
        print(msg.topic, msg.payload.decode())
 
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="plc-subscriber")
client.on_connect =on_connect
client.on_message = on_message
 
client.connect(BROKER, PORT, keepalive=60)
client.loop_forever()
