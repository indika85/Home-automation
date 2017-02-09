uint8_t deleteFingerprint(uint8_t id) {
	Serial.print("received ID: "); Serial.println(id);
	if (id < 0) {
		HMI_update_txt("del_fin_page.txt_message", "Invalid ID.");
		delay(2000);
		HMI_draw_screen("settings_page");
		return 0;
	}

	bool found = 0;
	for (int i = 0; i < MAX_NO_OF_FINFERPRINTS; i++) {
		if (saved_finger_ids[i] == id) {
			found = 1;
			break;
		}
	}
	if (!found) {
		HMI_update_txt("del_fin_page.txt_message", "Please enetr a valid ID.");
		return 0;
	}
	uint8_t p = -1;

	p = myFinger.deleteModel(id);

	if (p == FINGERPRINT_OK) {
		HMI_update_txt("del_fin_page.txt_message", "Fingerprint deleted. Please wait.");
		HMI_update_txt("del_fin_page.txt_id", "");
		Serial.println("Deleted!");
		clear_mem(name_addrs[id - 1], MAX_NAME_LEN);
		delay(2000);
		HMI_draw_screen("del_fin_page");
		return p;
	}
	else if (p == FINGERPRINT_PACKETRECIEVEERR) {
		Serial.println("Communication error");
		HMI_update_txt("del_fin_page.txt_message", "ERROR! Try again.");
		return p;
	}
	else if (p == FINGERPRINT_BADLOCATION) {
		Serial.println("Could not delete in that location");
		HMI_update_txt("del_fin_page.txt_message", "No fingerprint deleted");
		return p;
	}
	else if (p == FINGERPRINT_FLASHERR) {
		HMI_update_txt("del_fin_page.txt_message", "ERROR! Try again");
		Serial.println("Error writing to flash");
		return p;
	}
	else {
		Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
		return p;
	}
}
