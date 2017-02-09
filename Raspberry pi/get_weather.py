from urllib.request import urlopen
import json
import time
from datetime import datetime
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish

api_key = "***"
lat = "***"
lon = "***"

broker = '192.168.42.1'
mqttusername = '***'
mqttpassword = '***'

WEATHER_DATA_URL = "http://api.openweathermap.org/data/2.5/weather?lat=" + lat + "&lon=" + lon + "&mode=json&units=metric&APPID=" + api_key

client = mqtt.Client()
client.username_pw_set(mqttusername, mqttpassword)
client.connect(broker)
client.loop_start()


def get_data():
    meteo = urlopen(WEATHER_DATA_URL).read()
    meteo = meteo.decode('utf-8')
    weather = json.loads(meteo)
	
    id = weather['weather'][0]['id']
    description = weather['weather'][0]['description']

    temp = weather['main']['temp']
    pressure = weather['main']['pressure']
    humidity = weather['main']['humidity']

    wind_speed = weather['wind']['speed']
    wind_direction = weather['wind']['deg']

    clouds = weather['clouds']['all']

    sunrise = datetime.fromtimestamp(weather['sys']['sunrise']).strftime('%H:%M')
    sunset = datetime.fromtimestamp(weather['sys']['sunset']).strftime('%H:%M')

    time.sleep(2)
    client.publish('weather/id', id)
    time.sleep(2)
    client.publish('weather/description', description.title())
    time.sleep(2)

    client.publish('weather/temp', temp)
    time.sleep(2)
    client.publish('weather/pressure', pressure)
    time.sleep(2)
    client.publish('weather/humidity', humidity)
    time.sleep(2)
    client.publish('weather/wind_speed', wind_speed)
    time.sleep(2)
    client.publish('weather/wind_direction', wind_direction)
    time.sleep(2)
    client.publish('weather/clouds', clouds)
    time.sleep(2)
    client.publish('weather/sunrise', sunrise)
    time.sleep(2)
    client.publish('weather/sunset', sunset)


def main():
    get_data()


if __name__ == '__main__':
    main()
