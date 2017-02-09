import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import time
import math
import RPi.GPIO as GPIO
import spidev
from picamera import PiCamera
import time

# ------------------------------ Variables ------------------------------------
broker = '192.168.42.1'
livingroom_light_topic = 'home/livingroom/lightlevel'
livingroom_air_topic = 'home/livingroom/airquality'
livingroom_switch_topic = 'home/livingroom/light'
livingroom_switch_cmd_topic = 'home/livingroom/light/set'
outside_temp_topic = 'home/outside/temperature'

turnOffLightAfter = 120  #seconds
pir_pin = 4

light_percent = 55      #Turn lighs on if light level is less than
light_state = 0
delay = 0.1

mqttusername = '***'
mqttpassword = '***'

counter = 0
motion_detected = False

#https://learn.adafruit.com/thermistor/using-a-thermistor
THERMISTORNOMINAL = 10000  #resistance at 25 degrees C
TEMPERATURENOMINAL = 25  #temp. for nominal resistance (almost always 25 C)
NUMSAMPLES = 3  #how many samples to take and average, more takes longer
BCOEFFICIENT = 3950  #The beta coefficient of the thermistor (usually 3000-4000)
SERIESRESISTOR = 10000  #the value of the 'other' resistor
TEMPOFFSET = 1.9
#---------------------------- Setup -----------------------------------------------
camera = PiCamera()

GPIO.setmode(GPIO.BCM)
GPIO.setup(pir_pin, GPIO.IN)

bus = 0
device = 0

motion_end_time = time.time()

spi = spidev.SpiDev()
spi.open(bus, device)
client = mqtt.Client()
client.username_pw_set(mqttusername, mqttpassword)

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    #print("Connected with result code "+str(rc))
    client.subscribe(livingroom_switch_topic)


# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global light_state
    payload = msg.payload.decode("utf-8")
    #print(msg.topic+" " + payload)
    if msg.topic == livingroom_switch_topic:
        if payload == "ON":
            light_state = 1
        elif payload == "OFF":
            light_state = 0


def on_publish(client, userdata, mid):
    return
    print("mid: " + str(mid))


client.on_connect = on_connect
client.on_message = on_message
client.on_publish = on_publish

client.connect(broker)

client.loop_start()


def take_images():
    camera.resolution = (640, 480)
    camera.start_preview()
    time.sleep(.1)
    camera.capture('/home/pi/.homeassistant/Camera/image.jpg', format='jpeg', quality=10)
    camera.stop_preview()


def readadc(adcnum):
    # read SPI data from MCP3008 chip, 8 possible adc's (0 thru 7)
    #http://tightdev.net/SpiDev_Doc.pdf
    if adcnum > 7 or adcnum < 0:
        return -1
    r = spi.xfer2([1, 8 + adcnum << 4, 0])
    adcout = ((r[1] & 3) << 8) + r[2]
    return adcout


def turn_switch(state):
    client.publish(livingroom_switch_cmd_topic, state)


def process_switch(lightPer):
    if motion_detected and light_state == 0:
        if lightPer < light_percent:
            turn_switch("ON")
    if not motion_detected and light_state == 1:
        duration = time.time() - motion_end_time
        if duration > turnOffLightAfter:
            turn_switch("OFF")


def detect_motion():
    global motion_detected
    global motion_end_time
    if GPIO.input(pir_pin):
        if not motion_detected:
            #print("Motion detected!")
            motion_detected = True
    else:
        if motion_detected:
            #print("Motion ended!")
            motion_detected = False
            motion_end_time = time.time()


def readLightLevel():
    return round((readadc(0) / 1024) * 100, 1)


def readSmokeLevel():
    smkLevel = 1024 - readadc(1)
    return round((smkLevel / 1024) * 100, 1)


def readOutsideTemp():
    average = 0.0
    for i in range(0, NUMSAMPLES):
        average += readadc(2)
        time.sleep(0.008)

    average /= NUMSAMPLES
    average = 1023 / average - 1
    if average == 0:
        average = 1

    average = SERIESRESISTOR / average

    steinhart = 0.0
    steinhart = average / THERMISTORNOMINAL  # (R/Ro)
    steinhart = math.log(steinhart)  # ln(R/Ro)
    steinhart /= BCOEFFICIENT  # 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15)  # + (1/To)

    if steinhart == 0:
        steinhart = 1

    steinhart = 1.0 / steinhart  # Invert
    steinhart -= 273.15  # convert to C

    return round(steinhart - TEMPOFFSET, 2)


while True:
    counter += 1
    lightPer = readLightLevel()
    if counter >= 35:
        smkPer = readSmokeLevel()
        outTemp = readOutsideTemp()

        #print("Outside temperature: %d C", outTemp)
        #print ("Light percent: %d", lightPer)
        #print("Air quality level: %d", smkPer)

        client.publish(livingroom_light_topic, lightPer)
        client.publish(livingroom_air_topic, smkPer)
        client.publish(outside_temp_topic, outTemp)
        take_images()
        counter = 0
    time.sleep(delay)
    detect_motion()
    process_switch(lightPer)
