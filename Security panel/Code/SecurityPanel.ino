/*
* Arduino Security System Panel.
*
* Created by Indika Ratnayake
* www.niratnayake.com
*
*/

#include <DHT.h>							//Humidity and temparature sensor library
#include <Adafruit_Fingerprint.h>			//Fingerprint reader library https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library
#include <EEPROM.h>							//Save settings to the memory


#define MAX_PASSCODE_LEN 6					//Maximum length of the passcode.
#define MAX_NAME_LEN 8						//Maximum laenght of the name assinged to a fingerprint
#define MAX_NO_OF_FINFERPRINTS 6			//Number of fingerprints saved in memory

#define PASSCODE_ADDR 0						//Passcode will be saved at this address on the EEPROM (Max of 6 digits)
#define SYSSTATUS_ADDR 7					//System status will be saved at this address
#define FIN_1_ADDR 8						//Memory addresses of the stored fingerprints.
#define FIN_2_ADDR 16
#define FIN_3_ADDR 24
#define FIN_4_ADDR 32
#define FIN_5_ADDR 40
#define FIN_6_ADDR 48

const int name_addrs[MAX_NO_OF_FINFERPRINTS] = { FIN_1_ADDR, FIN_2_ADDR, FIN_3_ADDR, FIN_4_ADDR, FIN_5_ADDR, FIN_6_ADDR };
uint8_t saved_finger_ids[MAX_NO_OF_FINFERPRINTS];

#define DHTPIN 3							// Temp and humidity sensor
#define DHTTYPE DHT22						//Sensor type
#define PIRSENSOR 4							//PIR sensor pin
#define SCREEN_POWER_PIN 5					//Touchscreen power pin
#define esp_8266_RESET_PIN 6				//ESP8266 reset pin
#define BUZZERPIN A0						//Buzzer Pin
#define PHOTOCELL_PIN A6					//Photodell pin

#define PASSCODE_TIMEOUT 15000				//Tymeout for requesting passcode/ fingerprint
#define DISPLAY_SLEEP_TIMEOUT 25000			//Display turn off after this time (If no motion is detected)

#define ESP8266_PORT	Serial1				//ESP8266 port
#define FINGER_PRINT_PORT Serial2			//Fingerprint sensor is connected to serial1
#define HMI_PORT Serial3					//TFT touch screen port


#define SEG_START 0x3C				//<         //Starting segment of the serial message (ESP8266)
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

#define HMI_END 0XFF							//End byte of the serial messages sent to the display.


#define ESP_RESET_TIME 60000					//Reset ESP after 30seconds if there is no response.

#define SYS_STATUS_DISARMED		0				//System status
#define SYS_STATUS_ARMED_STAY	1
#define SYS_STATUS_ARMED_AWAY	2
#define SYS_STATUS_TRIGGERED	3

#define HOME_PAGE		0						//Pages on the creen
#define SETTINGS_PAGE	1
#define PASSCODE_PAGE_0	2
#define PASSCODE_PAGE_1	3
#define PASSCODE_PAGE_2	4
#define WEATHER_PAGE	5
#define NEW_FIN_PAGE	6
#define DEL_FIN_PAGE	7

uint8_t current_page = 0;

DHT dht(DHTPIN, DHTTYPE);						// Initialize temp and humidity sensor

Adafruit_Fingerprint myFinger = Adafruit_Fingerprint(&FINGER_PRINT_PORT);

unsigned long temparatureMillis = 0;			//Store the last temp reading time
unsigned long millisNow = 0;					//Store touch time to ignore repeated touch signals
unsigned long motionEndMillis = 0;				//Store motion end time
unsigned long ESPRespondMillis = 0;

//int fingerPrintID = -1;

uint8_t currentSysStatus = 0;					//System armed or disarmed

String time = "12:00 AM";

bool screenState = 1;							//Screen on or off
bool motionState = 0;							//Motion detected or not
bool activate_finger = 0;						//Activates the fingerprint scanner

bool readyToPublish = 0;
//bool systemStatusSent = 0;					//Send the system status when the us=nit restarts.
//long int temp = 5000;

#define TOPIC_SEG_SIZE 100
#define VAL_SEG_SIZE 50
#define ADDITIONAL_SEG_SIZE 50

char topicSeg[TOPIC_SEG_SIZE], valSeg[VAL_SEG_SIZE], additionalSeg[ADDITIONAL_SEG_SIZE];
char endSeg[3], typeSeg[3];

void setup()
{
	currentSysStatus = getSysStatus();				//Gets the system status from memory
	//Set pin modes
	pinMode(DHTPIN, INPUT);
	pinMode(PIRSENSOR, INPUT);
	pinMode(PHOTOCELL_PIN, INPUT);
	pinMode(BUZZERPIN, OUTPUT);
	pinMode(SCREEN_POWER_PIN, OUTPUT);
	//pinMode(esp_8266_RESET_PIN, OUTPUT);			//Set the ESP8266 reset pin as output
	digitalWrite(esp_8266_RESET_PIN, HIGH);			//Needs to be high for normal operation
	
	//turnDisplay(1);
	beep(2, 40);

	// Initial setup
	Serial.begin(9600);

	ESP8266_PORT.begin(57600);
	while (!ESP8266_PORT) {}

	HMI_PORT.begin(57600);
	while (!HMI_PORT) {}

	dht.begin();                           //Sratr the temp and humidity sensor

	readTemparature();                     //Read temperature for 1st time and update the diplay.

	myFinger.begin(57600);                // set the data rate for the sensor serial port          

	if (myFinger.verifyPassword()) {
		Serial.println(F("Found fingerprint sensor!"));
	}
	else {
		Serial.println(F("Did not find fingerprint sensor :("));
	}

	resetESP8266();											 //Reset the ESP8266 so the messages will be received again when the arduino is ready.
	
	HMI_update_txt("home_page.pass_code", read_mem(PASSCODE_ADDR, MAX_PASSCODE_LEN));
	
	publishSystemState();
}

void loop()
{
	readHMI_Data(0); 
	readEspPort();

	if (screenState) {
		setBrightnessLevel();
	}

	if (activate_finger) {
		scan_finger();
	}

	ditectMotion();
	//Check temparature every 5000ms (5 seconds) and only on homescreen
	if (millis() - temparatureMillis >= 5000) {
		temparatureMillis = millis();
		readTemparature();
	}

	/*if (millis() - ESPRespondMillis > ESP_RESET_TIME) {
		resetESP8266();
		Serial.println("No response from EPS. Resetting device.");
		ESPRespondMillis = millis();
	}*/
}

//***** Function to read the light level using the potocell *******
int readLightLevel() {
	int level = 1023 - analogRead(PHOTOCELL_PIN);
	level = map(level, 0, 1023, 0, 100);
	return level;
}

//***** Function to automatically set the brightness level of the display.
void setBrightnessLevel() {
	int level = readLightLevel();
	if (level >= 0 && level < 20) {
		HMI_set_brightness(15);
	}
	else if (level > 21 && level < 40) {
		HMI_set_brightness(30);
	}
	else if (level > 41 && level < 60) {
		HMI_set_brightness(45);
	}
	else if (level > 61 && level < 80) {
		HMI_set_brightness(70);
	}
	else if (level > 81 && level <= 100) {
		HMI_set_brightness(90);
	}
}
//************ Function to read temparature and humidity
void readTemparature() {
	float h = dht.readHumidity();              // Read humidity
	float t = 0;

	t = dht.readTemperature();               // Read temperature in C

	// Check if any reads failed and exit early (to try again).
	if (isnan(h) || isnan(t)) {
		//Serial.println(F("Failed to read from DHT sensor!"));
		return;
	}

	char tm[8], hu[8];
	dtostrf(t, 6, 2, tm);
	dtostrf(h, 6, 2, hu);

	publishToMqtt("home/livingroom/temperature", tm, 0);
	publishToMqtt("home/livingroom/humidity", hu, 0);

	HMI_update_txt("home_page.txt_temp", String(tm) + " C");
	HMI_update_txt("home_page.txt_humid", String(hu) + " %");
}


//************** Function to detect motion ********************
void ditectMotion() {
	if (digitalRead(PIRSENSOR) == HIGH) {            // check if the input is HIGH
		//Serial.println("Motion detected!");
		motionState = 1;
		if (screenState == 0) {
			turn_HMI(1);
		}
	}
	else {
		if (screenState == 1) {
			if (motionState) {
				motionEndMillis = millis();
			}
			motionState = 0;
			if (millis() - motionEndMillis > DISPLAY_SLEEP_TIMEOUT) {			
				if (current_page != HOME_PAGE) {			//If it's not the homescreen draw the home screen
					HMI_draw_screen("home_page");
					delay(5);
				}
				turn_HMI(0);
			}
		}
	}
}


//******* Function to beep the buzzer everytime a button is pressed *******
void beep(int number, int duration) {
	for (int i = 0; i<number; i++) {
		digitalWrite(BUZZERPIN, HIGH);
		delay(duration);
		digitalWrite(BUZZERPIN, LOW);
		if (i < number - 1) {                                    //Skip delay on the last iteration
			delay(duration);
		}
	}
}

//******* Function to read the passcode *************
String read_mem(uint8_t addr, uint8_t len) {
	//Serial.print("Reading from: "); Serial.println(addr);
	String	c = "";
	char tm;
	for (int i = addr; i < addr + len; i++) {
		tm = EEPROM.read(i);
		if (tm == '\0') {
			break;
		}
		c += (char)tm;
		delay(10);
	}
	/*if (c == "") {
		Serial.println("No passcode found. Saving default.");
		saveCode("1985");
		readCode();
	}*/
	//Serial.print("Memory read: "); Serial.println(c);
	return c;
}

//******** Function to check the passcode *****************
bool checkCode(String code) {
	if (code == read_mem(PASSCODE_ADDR, MAX_PASSCODE_LEN)) {
		return true;
	}
	else
		return false;
}
//******** Function to save new passcode *****************
bool write_mem(uint8_t addr, uint8_t max_len, String val) {
	char char_array[MAX_NAME_LEN+1];

	val.toCharArray(char_array, MAX_NAME_LEN+1);

	//Serial.print("Writing: "); Serial.println(val);
	//Serial.print("Addr: "); Serial.println(addr);

	int curr_pos = 0;

	for (uint8_t i = addr; i < addr + max_len; i++) {
		if (curr_pos < val.length()) {
			EEPROM.write(i, char_array[i-addr]);
			Serial.print("Array item: "); Serial.println(char_array[i-addr]);
		}
		else {
			EEPROM.write(i-addr, 0);
		}
		curr_pos += 1;
		//Serial.print("Char: "); Serial.println(char_array[i]);
		delay(20);
	}
	return true;
}

void clear_mem(uint8_t addr, uint8_t len) {
	for (int i = addr; i < addr+len; i++) {
		EEPROM.write(i, 0);
		delay(10);
	}
	Serial.print("Clearing mem: "); Serial.println(addr);
}

int get_available_id() {
	for (int i = 0; i < MAX_NO_OF_FINFERPRINTS; i++) {
		uint8_t addr = name_addrs[i];
		String mv = read_mem(addr, 1);
		int tmp = i + 1;

		if (mv != String(tmp)) {
			return tmp;
		}
	}
	return -1;
}

//********* Function to arm/ disarm the system ***********
void armSystem(uint8_t val) {
	if (currentSysStatus == val) return;

	currentSysStatus = val;
	EEPROM.write(SYSSTATUS_ADDR, val);
	delay(10);
	publishSystemState();
	beep(1, 80);
}

//********* Function to publish system state to the mqtt server ***********
void publishSystemState() {
	if (currentSysStatus == SYS_STATUS_DISARMED) {
		publishToMqtt("home/security", "disarmed", 1);
	}
	else if (currentSysStatus == SYS_STATUS_ARMED_AWAY) {
		publishToMqtt("home/security", "armed_away", 1);
	}
	else if (currentSysStatus == SYS_STATUS_ARMED_STAY) {
		publishToMqtt("home/security", "armed_home", 1);
	}
	else if (currentSysStatus == SYS_STATUS_TRIGGERED) {
		publishToMqtt("home/security", "triggered", 1);
	}
	HMI_update_int("home_page.sys_status", currentSysStatus);
	HMI_draw_screen("home_page");

}

uint8_t getSysStatus() {
	uint8_t st = EEPROM.read(SYSSTATUS_ADDR);
	Serial.print("System status: "); Serial.println(st);
	return st;
}

//******** Function to get passcode or fingerprint **********
void scan_finger() {
	Serial.println("Scanning finger");
	unsigned long fingerScanner_start_millis = millis();				//Store fingerprint read time
	unsigned long finger_gap_between_millis = millis()-30;
	while (true) {
		if (millis() - finger_gap_between_millis > 30) {
			finger_gap_between_millis = millis();
			if (getFingerprintIDez() > -1) {							//If a fingerprint is found																	//FINGER_PRINT_PORT.end();
				beep(1, 80);
				activate_finger = 0;
				HMI_fingerprint_found();
				return;
			}
		}
		if (millis() - fingerScanner_start_millis > PASSCODE_TIMEOUT) {	//If timeout
																		//FINGER_PRINT_PORT.end();
			beep(1, 80);
			HMI_draw_screen("home_page");
			activate_finger = 0;
			Serial.println("Fingerprint timed out");
			return;
		}
		if(readHMI_Data(0)) return;
		//readEspPort();
	}

}
