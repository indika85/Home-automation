//https://openweathermap.org/weather-conditions

uint8_t get_pic_id(String val) {
	int weather_id = val.toInt();
	//Serial.print("WeatherID: "); Serial.println(val);
	if (weather_id <= 0) {
		return 21;
	}

	if (weather_id == 800) {								//800 = clear sky
		if(isDayTime())	return 20;							//Sun
		return 29;											//Moon
	}
	if (weather_id >= 801 && weather_id <= 804) {			//few clouds
		if (isDayTime()) return 19;							//Sun + clouds
		return 21;											//Moon + clouds
	}
	if (weather_id >= 300 && weather_id <= 321) {			//Drizzle
		if (isDayTime()) return 23;							//Sun + Drizzle
		return 31;											//Moon + Drizzle
	}
	if (weather_id == 600) {								//Light snow
		if (isDayTime()) return 25;							//Sun + Snow
		return 26;											//Moon + Snow
	}
	if (weather_id >= 200 && weather_id <= 232) return 30;	//Thunderstorm
	if (weather_id >= 701 && weather_id <= 771) return 32;	//Atmosphere
	if (weather_id >= 601 && weather_id <= 622) return 22;	//Snow
	if (weather_id >= 500 && weather_id <= 531) return 24;	//Rain
	if (weather_id == 900 || weather_id == 781) return 35;	//Tornado
	if (weather_id == 901 || weather_id == 902) return 36;	//tropical storm
	if (weather_id == 903) return 33;						//Extreme cold
	if (weather_id == 904) return 34;						//Extreme hot
	if (weather_id == 905) return 37;						//Windy
	if (weather_id == 906) return 38;						//Hail
	
	return 39;												//N/A Icon
}

bool isDayTime() {
	int hour = time.substring(0, 2).toInt();
	//Serial.print("Hour: "); Serial.println(hour);
	if (hour < 0) {
		//Serial.print("Invalid hour :"); Serial.println(time.substring(0, 1));
		return true;
	}

	if (time.indexOf("AM") >= 0) {
		if (hour == 12) return false;
		if (hour >= 1 && hour <= 3) return false;
		if (hour >= 4 && hour <= 11) return true;
	}
	else {
		if (hour == 12) return true;
		if (hour >= 1 && hour <= 5) return true;
		if (hour >= 6 && hour <= 11) return false;
	}
	return true;
}
