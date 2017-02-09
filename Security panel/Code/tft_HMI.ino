bool readHMI_Data(bool enrolling) {
	//while (HMI_PORT.available() > 0) {
	//	Serial.write(HMI_PORT.read());
	//}

	if (HMI_PORT.available() <= 0) return 0;

	if (HMI_PORT.find("<")) {
		String part1 = "", part2 = "";
		part1 = HMI_PORT.readStringUntil('-', 15);
		part2 = HMI_PORT.readStringUntil('>', 15);

		Serial.print("Part1: "); Serial.println(part1);
		Serial.print("Part2: "); Serial.println(part2);
		
		if (part1 == "sys") {
			if (part2 == "ha") {			//Home away
				armSystem(SYS_STATUS_ARMED_AWAY);
			}
			else if (part2 == "da") {		//Disarm
				armSystem(SYS_STATUS_DISARMED);
			}
			else if (part2 == "hs") {		//Home stay
				armSystem(SYS_STATUS_ARMED_STAY);
			}
		}
		else if (part1 == "btn") {
			motionState = 1;

			if (enrolling || activate_finger) {
				if (part2 == "cancel" || part2 == "back" || part2 == "home") {
					if (activate_finger) {
						activate_finger = 0;
					}
					return 1;
				}
			}

			//beep(1, 40);
		}
		else if (part1 == "pw") {										//Save new passcode
			write_mem(PASSCODE_ADDR, MAX_PASSCODE_LEN, part2);
		}
		else if (part1 == "page") {
			if (part2 == "home") current_page = HOME_PAGE;
			else if (part2 == "settings") current_page = SETTINGS_PAGE;
			else if (part2 == "pc_0") {									//On disarm page
				current_page = PASSCODE_PAGE_0;
				activate_finger = 1;
			}
			else if (part2 == "pc_1") {									//On disarm page
				current_page = PASSCODE_PAGE_1;
				activate_finger = 1;
			}
			else if (part2 == "pc_2") current_page = PASSCODE_PAGE_2;	//New code
			else if (part2 == "weather") current_page = WEATHER_PAGE;
			else if (part2 == "new_fin") current_page = NEW_FIN_PAGE;
			else if (part2 == "del_fin") {
				current_page = DEL_FIN_PAGE;
				HMI_list_fingerprints();
			}
		}
		else if (part1 == "name") {										//Begin fingerprint enrollment
			enrollFingerBegin(part2);
		}
		else if (part1 == "code" && part2 == "ok") {
			activate_finger = 0;
			return 1;
		}
		else if (part1 == "del") {
			deleteFingerprint(part2.toInt());
		}
		else if (part1 == "time") {
			motionState = 1;
		}
	}
	return 0;
}

void HMI_update_txt(String component, String val) {
	if (!HMI_PORT) {
		Serial.println("HMI Port not ready");
		return;
	}
	String cmd = component + ".txt=\"" + val + "\"";
	HMI_PORT.print(cmd);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
	delay(10);
}

void HMI_update_int(String component, int val) {
	if (!HMI_PORT) {
		Serial.println("HMI Port not ready");
		return;
	}
	String cmd = component + ".val=" + String(val);
	HMI_PORT.print(cmd);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
}

void HMI_update_pic(String component, uint8_t pic_id) {
	if (!HMI_PORT) {
		Serial.println("HMI Port not ready");
		return;
	}
	String cmd = component + ".pic=" + pic_id;
	HMI_PORT.print(cmd);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
}

void HMI_draw_screen(String screen_name) {
	if (!HMI_PORT) {
		Serial.println("HMI Port not ready");
		return;
	}
	String cmd = "page " + screen_name;
	HMI_PORT.print(cmd);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
}

//********Function to turn the display on or off********
void turn_HMI(bool val) {
	if (val) {
		Serial.println("Turning display on");
		HMI_set_brightness(60);
		screenState = 1;
	}
	else {
		HMI_set_brightness(0);
		Serial.println("Turning display off");
		screenState = 0;
	}
}

void HMI_set_brightness(int val) {
	String cmd = "dim=" + String(val);
	//Serial.println(cmd);
	HMI_PORT.print(cmd);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
}

void HMI_fingerprint_found() {
	if (current_page == PASSCODE_PAGE_0) {			//Disarm the system
		armSystem(SYS_STATUS_DISARMED);
	}
	else if (current_page == PASSCODE_PAGE_1) {		//Go to settings page
		HMI_draw_screen("settings_page");
	}
}

void HMI_set_component_visibility(String component, bool vis) {
	/*String cmd = "vis " + component + "," + String(vis);
	Serial.println(cmd);
	HMI_PORT.print(cmd);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);*/
}

void HMI_list_fingerprints() {
	Serial.println("Listing saved prints");

	uint8_t found_count = 0;
	memset(saved_finger_ids, 0, MAX_NO_OF_FINFERPRINTS);	//Set all array elements to 0
	for (int i = 0; i < MAX_NO_OF_FINFERPRINTS; i++) {
		uint8_t addr = name_addrs[i];
		//Serial.print("Address: "); Serial.println(addr);

		String name = read_mem(addr, MAX_NAME_LEN);
		//Serial.print("Name len: "); Serial.println(name.length());

		int id = 0;
		if(name.length()>2){
			id = name.substring(0, 1).toInt();
		}
		
		Serial.print("Read ID: "); Serial.println(id);

		if ( id > 0) {
			String component = "del_fin_page.t" + String(found_count);
			HMI_update_txt(component, name);
			saved_finger_ids[found_count] = id;
			found_count += 1;
		}
	}

	Serial.print("Fingerprint count: "); Serial.println(found_count);

	if (found_count == 0) {
		HMI_set_enable("btn_del", 0);			//If no fingerprints, disable delete button
		HMI_update_txt("del_fin_page.txt_message", "No saved prints found.");
	}
	else {
		HMI_set_enable("btn_del", 1);
	}
}

void HMI_set_enable(String component, bool enable) {
	return;
	String cmd = "tsw " + component + "," + String(enable);
	Serial.println(cmd);
	HMI_PORT.print(cmd);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
	HMI_PORT.write(HMI_END);
}