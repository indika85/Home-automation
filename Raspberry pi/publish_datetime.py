from datetime import datetime
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish


broker = '192.168.42.1'
mqttusername = '***'
mqttpassword = '***'

client = mqtt.Client()
client.username_pw_set(mqttusername, mqttpassword)
client.connect(broker)
client.loop_start()


def publish_datetime():
    now = datetime.now()
    ts = now.strftime('%I:%M %p-%a, %B %d, %Y')
    client.publish('datetime/now', ts)
    #print("Now is: " + ts)

def main():
    publish_datetime()


if __name__ == '__main__':
    main()
