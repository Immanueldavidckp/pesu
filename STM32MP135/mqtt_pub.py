import paho.mqtt.client as mqtt
import time
 
BROKER ="broker.hivemq.com"
PORT = 1883
TOPIC ="testing/PES"
 
def on_connect(client, userdata, flags, reason_code, properties):
        if reason_code==0:
                print("Connected to broker")
        else:
                print(f"Connect faild:{reason_code}")
 
def on_publish(client, userdata, mid, reason_code, properties):
        print(f"Pblished, mid={mid}")
 
client =mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="plc-publisher")
client.on_connect=on_connect
client.on_publish =on_publish
 
client.connect(BROKER, PORT, keepalive=60)
client.loop_start()
 
try:
        payload ="hello from emo"
        client.publish(TOPIC, payload, qos=1)
except KeyboardInterrupt:
        pass
finally:
        client.loop_stop()
        client.disconnect()
