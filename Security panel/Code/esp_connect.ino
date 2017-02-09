void readEspPort() {
	//readIncomingMessages();return;

	if (readIncomingMessages()) {
		switch (typeSeg[0]) {                                    //Check the type segment of the message
		case MSG_TYPE_RECEIVED_MSG:
			//Serial.println("Message received!");
			takeAction(topicSeg, valSeg);
			break;
		case MSG_TYPE_INFO:
			//Serial.print(F("Info received- "));
			Serial.println(topicSeg);
			break;
		case STATUS_CONNECTION_ERROR:
			readyToPublish = 0;
			//resetESP8266();
			Serial.println(F("Error message received."));
			break;
		case MSG_TYPE_STATUS:
			//Serial.print("Status received :"); Serial.println(topicSeg[0]);
			
			if (topicSeg[0] == STATUS_SUBSCRIBE_FAILED) {
				readyToPublish = 0;
			}
			else if (topicSeg[0] == STATUS_SUBSCRIBE_SUCCESS) {
				readyToPublish = 1;
			}
			else if (topicSeg[0] == STATUS_CONNECTIONS_OK) {
				//Serial.println("Connections OK...");
				readyToPublish = 1;
				ESPRespondMillis = millis();
			}
			else if (topicSeg[0] == STATUS_ESTABLISHING_CONNECTIONS) {
				readyToPublish = 0;
			}
			else if (topicSeg[0] == STATUS_PUBLISH_SUCCESS) {
				//
			}
			else if (topicSeg[0] == STATUS_NEW_CONNECTION) {
				publishSystemState();
			}
			else {
				Serial.print(F("Unprocessed topic seg: "));//
				Serial.println(topicSeg[0]);
			}
			break;
		default:
			Serial.print(F("----- Invalid request received ----- "));
			Serial.print(F("Type: "));
			Serial.println(typeSeg);
			Serial.print(F("Topic: "));
			Serial.println(topicSeg);
			break;
		}
	}
}
void resetESP8266() {
	return;
	Serial.println(F("Reseting WiFi device"));
	//digitalWrite(esp_8266_RESET_PIN, LOW);            //Reset the ESP8266 module by making the pin low.
	//delay(1000);
	//digitalWrite(esp_8266_RESET_PIN, HIGH);
	//systemStatusSent = 0;
}

void takeAction(String topic, String val) {
	//Serial.print(F("Message from: "));
	//Serial.print(topic);
	//Serial.print(F(" with value: "));
	//Serial.println(val);
	
	if (topic == "home/security/set") {
		if (val == "DISARM") {
			armSystem(SYS_STATUS_DISARMED);
		}
		else if (val == "ARM_HOME") {
			armSystem(SYS_STATUS_ARMED_STAY);
		}
		else if (val == "ARM_AWAY") {
			armSystem(SYS_STATUS_ARMED_AWAY);
		}
	}
	else if (topic == "weather/id") {
		HMI_update_pic("home_page.pic_icon", get_pic_id(val));
	}
	else if (topic == "weather/description") {
		HMI_update_txt("home_page.txt_forecast", val);
	}
	else if (topic == "weather/temp") {
		HMI_update_txt("weather_page.t0", val);
	}
	else if (topic == "weather/pressure") {
		HMI_update_txt("weather_page.t1", val);
	}
	else if (topic == "weather/humidity") {
		HMI_update_txt("weather_page.t2", val);
	}
	else if (topic == "weather/wind_speed") {
		HMI_update_txt("weather_page.t3", val);
	}
	else if (topic == "weather/wind_direction") {
		HMI_update_txt("weather_page.t4", val);
	}
	else if (topic == "weather/clouds") {
		HMI_update_txt("weather_page.t5", val);
	}
	else if (topic == "weather/sunrise") {
		HMI_update_txt("weather_page.t6", val);
	}
	else if (topic == "weather/sunset") {
		HMI_update_txt("weather_page.t7", val);
	}
	else if (topic == "datetime/now") {
		uint8_t index = val.indexOf("-");
		if (index <= 0) return;

		time = val.substring(0, index);
		String date = val.substring(index + 1);

		HMI_update_txt("home_page.txt_time", time);
		HMI_update_txt("home_page.txt_date", date);

		if (val.indexOf("AM") >= 0) {
			HMI_update_txt("home_page.txt_greeting", "Good morning");
		}
		else {
			HMI_update_txt("home_page.txt_greeting", "Good evening");
		}
	}
}



void publishToMqtt(char* topic, char* val, bool retained) {
	if (readyToPublish) {
		ESP8266_PORT.write(SEG_START);
		ESP8266_PORT.write(MSG_TYPE_PUBLISH);
		ESP8266_PORT.write(SEG_SEPARATOR);
		ESP8266_PORT.write(topic);
		ESP8266_PORT.write(SEG_SEPARATOR);
		ESP8266_PORT.write(val);
		ESP8266_PORT.write(SEG_SEPARATOR);
		ESP8266_PORT.write(retained);
		ESP8266_PORT.write(SEG_SEPARATOR);
		ESP8266_PORT.write(SEG_END);
		ESP8266_PORT.write(SEG_SEPARATOR);

		//Serial.print("Published: "); Serial.print(topic); Serial.print(" Val: "); Serial.println(val);
	}
	//if (!readyToPublish) { Serial.println("Not ready to publish"); }
}

bool isNumeric(char *str) {
	for (byte i = 0; str[i]; i++)
	{
		if (!isDigit(str[i]))
			return false;
	}
	return true;
}

bool readIncomingMessages() {
	/*while (ESP8266_PORT.available() > 0) {
		Serial.write(ESP8266_PORT.read());
	}
	while(Serial.available() > 0) {
		ESP8266_PORT.write(Serial.read());
	}
	return 0;*/

	topicSeg[0] = '\0';
	endSeg[0] = '\0';
	typeSeg[0] = '\0';
	valSeg[0] = '\0';
	additionalSeg[0] = '\0';

	if (ESP8266_PORT.find("<")) {                          //Check if the start segment is there
		uint8_t len = 0;
		//Serial.println("---------Message received-------------");
		delay(1);
		len = ESP8266_PORT.readBytesUntil(SEG_SEPARATOR, typeSeg, 2);

		//Serial.print("Type seg: "); Serial.print("("); Serial.print(len); Serial.print(") -"); Serial.println(typeSeg);

		if (!isNumeric(typeSeg)) return false;
		if (typeSeg[0] == MSG_TYPE_INFO) { return false; }

		ESP8266_PORT.read();

		len = ESP8266_PORT.readBytesUntil(SEG_SEPARATOR, topicSeg, TOPIC_SEG_SIZE -1);            //Store the topic segment
		delay(1);
		if (len<TOPIC_SEG_SIZE) {
			topicSeg[len] = '\0';
		}

		//Serial.print("Topic seg: "); Serial.print("("); Serial.print(len); Serial.print(") -"); Serial.println(topicSeg);

		len = ESP8266_PORT.readBytesUntil(SEG_SEPARATOR, valSeg, VAL_SEG_SIZE -1);              //Store value segment
		delay(1);
		if (len<VAL_SEG_SIZE) {
			valSeg[len] = '\0';
		}

		//Serial.print("Val seg: "); Serial.print("("); Serial.print(len); Serial.print(") -"); Serial.println(valSeg);

		len = ESP8266_PORT.readBytesUntil(SEG_SEPARATOR, additionalSeg, ADDITIONAL_SEG_SIZE -1);       //Store any info (ip address)
		delay(1);
		if (len<ADDITIONAL_SEG_SIZE) {
			additionalSeg[len] = '\0';
		}

		//Serial.print("Add. seg: "); Serial.print("("); Serial.print(len); Serial.print(") -"); Serial.println(additionalSeg);

		len=ESP8266_PORT.readBytesUntil(SEG_SEPARATOR, endSeg, 2);              //Store the end segment
		endSeg[len] = '\0';
		delay(1);

		//Serial.print("End seg: "); Serial.print("("); Serial.print(len); Serial.print(") -"); Serial.println(endSeg);
	}
	if (endSeg[0] == SEG_END) {                                //Check if the end segment matches
		return true;                                         //If end segment matches it's valid data
	}
	return false;
}
