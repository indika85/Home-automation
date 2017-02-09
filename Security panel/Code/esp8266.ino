#include <ESP8266WiFi.h>                     //https://github.com/esp8266/Arduino
#include <PubSubClient.h>                    //https://github.com/knolleary/pubsubclient
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define ssid			"***"
#define password		"***"
#define mqtt_username	"***"
#define mqtt_password	"***"

#define TOPIC_COUNT 12
char* topics[TOPIC_COUNT] = {"home/security/set", "weather/id", "weather/description", "weather/temp", "weather/pressure", "weather/humidity", "weather/wind_speed", "weather/wind_direction", "weather/clouds", "weather/sunrise", "weather/sunset", "datetime/now"};

IPAddress broker(192, 168, 42, 1);          // Address of the MQTT broker

#define CLIENT_ID "client-panel"			// Client ID to send to the broker

#define MQTT_QOS 1
//Serial message format
//<TYPE#TOPIC#VALUE#DATA#>#

#define SEG_START 0x3C				//<         //Starting segment of the serial message
#define SEG_END 0x3E		        //>         //Ending segment of the serial message
#define SEG_SEPARATOR 0x23			//#			//Separator between messages.
#define MSG_TYPE_PUBLISH 0x31                   //Publish a message to a topic
#define MSG_TYPE_SUBSCRIBE 0x32                 //Subscribe to a topic
#define MSG_TYPE_RECEIVED_MSG 0x35              //Message received from a subscribed topic
#define MSG_TYPE_INFO 0x36                      //General debugging information
#define MSG_TYPE_STATUS 0x38                    //Sends the staus. 

#define STATUS_CONNECTION_ERROR 0x39           //Error connecting to WiFi. 
#define STATUS_NEW_CONNECTION 0x41			   //New connection
#define STATUS_CONNECTIONS_OK 0x42             //All connection are Ok
#define STATUS_ESTABLISHING_CONNECTIONS 0x43   //Waitong for connections to be made (WiFi and mqtt)
#define STATUS_PUBLISH_SUCCESS 0x44            //Message published successfully
#define STATUS_PUBLISH_FAILED 0x45             //Publish failed.
#define STATUS_SUBSCRIBE_SUCCESS 0x46          //Subscribed to topic successfully
#define STATUS_SUBSCRIBE_FAILED 0x47           //Subscribing failed.

#define TOPIC_SEG_SIZE 100
#define VAL_SEG_SIZE 50
#define ADDITIONAL_SEG_SIZE 50

char topicSeg[TOPIC_SEG_SIZE], valSeg[VAL_SEG_SIZE], additionalSeg[ADDITIONAL_SEG_SIZE];
char endSeg[3], typeSeg[3];

WiFiClient wificlient;
PubSubClient client(wificlient);

unsigned long mil = 10000;

void setup() {
	Serial.begin(57600);
	sendStatus(STATUS_ESTABLISHING_CONNECTIONS);
	sendInfo("Booting");
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	sendInfo("WiFi begun");
	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
		sendInfo("Connection Failed! Rebooting...");
		delay(5000);
		ESP.restart();
	}

	ArduinoOTA.setHostname("esp8266-panel");

	ArduinoOTA.onStart([]() {
		Serial.println("OTA Starting");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nOTA End");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();

	sendInfo("IP address: ");
	sendInfo(IPtoCharArray(WiFi.localIP()));

	/* Prepare MQTT client */
	client.setServer(broker, 1883);
	client.setCallback(callback);

}

void loop() {

	ArduinoOTA.handle();
	if (WiFi.status() != WL_CONNECTED)
	{
		sendStatus(STATUS_ESTABLISHING_CONNECTIONS);
		sendInfo("Connecting to ");
		sendInfo(ssid);
		WiFi.begin(ssid, password);

		if (WiFi.waitForConnectResult() != WL_CONNECTED)
			return;
		sendInfo("WiFi connected");
	}

	if (WiFi.status() == WL_CONNECTED) {
		if (!client.connected()) {
			sendStatus(STATUS_ESTABLISHING_CONNECTIONS);
			reconnect();
		}
	}

	delay(10);

	if (client.connected())
	{
		client.loop();
	}
	readIncomingMessages();

	if (millis() - mil > 10000) {					//Send an update every 10 seconds.
		sendConnectionStatus();
		mil = millis();
	}
}

void readIncomingMessages() {
	delay(5);
	bool validMsg = 0;

	topicSeg[0] = '\0';
	endSeg[0] = '\0';
	typeSeg[0] = '\0';
	valSeg[0] = '\0';
	additionalSeg[0] = '\0';
	if (Serial.find(SEG_START)) {															//Check if the start segment is there
		uint8_t len = 0;

		Serial.readBytesUntil(SEG_SEPARATOR, typeSeg, sizeof(typeSeg)-1);
		delay(2);

		//Serial.print("Type seg: "); Serial.println(typeSeg);

		if (!isNumeric(typeSeg)) return;

		len = Serial.readBytesUntil(SEG_SEPARATOR, topicSeg, sizeof(topicSeg)-1);            //Store the topic segment
		delay(2);
		if (len<TOPIC_SEG_SIZE) {
			topicSeg[len] = '\0';
		}

		//Serial.print("Topic seg: "); Serial.println(topicSeg);

		len = Serial.readBytesUntil(SEG_SEPARATOR, valSeg, sizeof(valSeg)-1);					//Store value segment
		delay(2);
		if (len<VAL_SEG_SIZE) {
			valSeg[len] = '\0';
		}

		//Serial.print("Val seg: "); Serial.println(valSeg);

		len = Serial.readBytesUntil(SEG_SEPARATOR, additionalSeg, sizeof(additionalSeg)-1);    //Store any info (ip address)
		delay(2);
		if (len<ADDITIONAL_SEG_SIZE) {
			additionalSeg[len] = '\0';
		}

		//Serial.print("Add. seg: "); Serial.println(additionalSeg);

		Serial.readBytesUntil(SEG_SEPARATOR, endSeg, sizeof(endSeg)-1);						//Store the end segment
		endSeg[1] = '\0';
		delay(2);

		//Serial.print("End seg: "); Serial.println(endSeg);
	}
	if (endSeg[0] == SEG_END) {																//Check if the end segment matches
		validMsg = 1;																		//If end segment matches it's valid data
	}
	if (validMsg) {
		if (typeSeg[0] == MSG_TYPE_PUBLISH) {
			publishMessage(topicSeg, valSeg, additionalSeg);
		}
	}
}
//********* Function to publish a message to a topic *********
void publishMessage(char* topic, char* msg, char* retained) {
	bool rt = false;

	//Serial.print("Retained value: ");
	//Serial.println(retained[0]);
	if(retained[0]== 1){
	  rt = true;
	}

	if (client.publish(topic, msg, rt)) {                         //int publish (topic, payload, retained)
		//sendStatus(STATUS_PUBLISH_SUCCESS);
	}
	else {
		//sendStatus(STATUS_PUBLISH_FAILED);
	}
}

//********* Function to subscribe to a topic *********
void subscribeTo(char *topic) {

	client.subscribe(topic, MQTT_QOS);

}

void reconnect() {
	// Loop until we're reconnected
	uint8_t count = 0;
	while (!client.connected()) {
		sendInfo("Attempting MQTT connection...");
		// Attempt to connect
		if (client.connect(CLIENT_ID, mqtt_username, mqtt_password)) {
			sendInfo("connected to MQTT");

			//Subscribe to topics
			for (int i = 0; i < TOPIC_COUNT; i++) {
				subscribeTo(topics[i]);
				client.loop();
				delay(10);
			}
			sendStatus(STATUS_CONNECTIONS_OK);
			sendStatus(STATUS_NEW_CONNECTION);
		}
		else {
			sendInfo("MQTT failed");
			//sendInfo(client.state());
			sendInfo(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			count++;
			if (count > 10) {
				sendStatus(STATUS_CONNECTION_ERROR);
				sendInfo("Rebooting");
				ESP.restart();
			}
			delay(5000);
		}
	}
}

//*********** Function that will be called when a message is received from a topic *********
void callback(char* topic, byte* payload, unsigned int length) {
	char p[20];
	p[0] = '\0';
	for (int i = 0;i<length;i++) {
		p[i] = (char)payload[i];
	}
	p[length] = '\0';

	Serial.write(SEG_START);
	Serial.write(MSG_TYPE_RECEIVED_MSG);
	Serial.write(SEG_SEPARATOR);
	Serial.write(topic);
	Serial.write(SEG_SEPARATOR);
	Serial.write(p);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_END);
	Serial.write(SEG_SEPARATOR);

	Serial.flush();
	delay(20);

}

//********* Function to send information through the serial port. **********
void sendInfo(char* msg) {

	Serial.write(SEG_START);
	Serial.write(MSG_TYPE_INFO);
	Serial.write(SEG_SEPARATOR);
	Serial.write(msg);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_END);
	Serial.write(SEG_SEPARATOR);

	Serial.flush();
	delay(10);
}

void sendCommand(uint8_t msg_type) {
	Serial.write(SEG_START);
	Serial.write(msg_type);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_END);
	Serial.write(SEG_SEPARATOR);

	Serial.flush();
	delay(50);
}

void sendStatus(uint8_t st) {
	Serial.write(SEG_START);
	Serial.write(MSG_TYPE_STATUS);
	Serial.write(SEG_SEPARATOR);
	Serial.write(st);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_SEPARATOR);
	Serial.write(SEG_END);
	Serial.write(SEG_SEPARATOR);

	Serial.flush();
	delay(5);
}

void sendConnectionStatus() {

	if (client.connected() && (WiFi.status() == WL_CONNECTED)) {             //If both WiFi and mqtt are connected.
		sendStatus(STATUS_CONNECTIONS_OK);
	}
	else {
		sendStatus(STATUS_ESTABLISHING_CONNECTIONS);
	}
	delay(5);
}

bool isNumeric(char *str) {
	for (byte i = 0; str[i]; i++)
	{
		if (!isDigit(str[i]))
			return false;
	}
	return true;
}
char* IPtoCharArray(IPAddress ip)
{
	static char szRet[20];
	String str = String(ip[0]);
	str += ".";
	str += String(ip[1]);
	str += ".";
	str += String(ip[2]);
	str += ".";
	str += String(ip[3]);
	str.toCharArray(szRet, 20);
	return szRet;
}