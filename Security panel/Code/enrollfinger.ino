int enrollFingerBegin(String name) {
	if (name.length() > MAX_NAME_LEN) {
		Serial.println("Naximum name length exceeded.");
		return -1;
	}
	unsigned long tmp = 0;
	
	motionState = 1;

	HMI_update_txt("new_fin_page.txt_message", "Setting up for enrolling finger: " + name);

	//return 0;
	int id = get_available_id();
	delay(1000);
	Serial.print("Available id: "); Serial.println(id);
	if (id < 0) {
		HMI_update_txt("new_fin_page.txt_message", "Memory full. Returning to settings page.");
		delay(2000);
		HMI_draw_screen("settings_page");
		return 0;
	}
	int p = -1;
	HMI_update_txt("new_fin_page.txt_message", "Place your finger on the scanner to continue.");
	HMI_set_component_visibility("btn_cancel", 1);
	tmp = millis();
	while (p != FINGERPRINT_OK) {
		if (readHMI_Data(1)) {
			return 0;
		}

		p = myFinger.getImage();
		switch (p) {
		case FINGERPRINT_OK:
			Serial.println("Image taken");
			break;
		case FINGERPRINT_NOFINGER:
			Serial.println(".");
			break;
		case FINGERPRINT_PACKETRECIEVEERR:
			Serial.println("Communication error");
			HMI_update_txt("new_fin_page.txt_message", "Communication Error");
			break;
		case FINGERPRINT_IMAGEFAIL:
			Serial.println("Imaging error");
			HMI_update_txt("new_fin_page.txt_message", "Image Error");
			break;
		default:
			HMI_update_txt("new_fin_page.txt_message", "Unknown Error");
			Serial.println("Unknown error");
			break;
		}
		if (millis() - tmp > PASSCODE_TIMEOUT) {
			Serial.println("Fingerprint enroll timeout");
			HMI_update_txt("new_fin_page.txt_message", "Timed out. Please try again.");
			HMI_set_component_visibility("new_fin_page.btn_cancel", 0);
			delay(2000);
			return 0;
		}
	}

	motionState = 1;
	// OK success!

	p = myFinger.image2Tz(1);
	switch (p) {
	case FINGERPRINT_OK:
		Serial.println("Image converted");
		break;
	case FINGERPRINT_IMAGEMESS:
		Serial.println("Image too messy");
		HMI_update_txt("new_fin_page.txt_message", "Image not clear. Try again");
		return p;
	case FINGERPRINT_PACKETRECIEVEERR:
		Serial.println("Communication error");
		HMI_update_txt("new_fin_page.txt_message", "Communication Error");
		return p;
	case FINGERPRINT_FEATUREFAIL:
		Serial.println("Could not find fingerprint features");
		HMI_update_txt("new_fin_page.txt_message", "ERROR! Fingerprint features");
		return p;
	case FINGERPRINT_INVALIDIMAGE:
		Serial.println("Could not find fingerprint features");
		HMI_update_txt("new_fin_page.txt_message", "ERROR! Invalid image");
		return p;
	default:
		Serial.println("Unknown error");
		HMI_update_txt("new_fin_page.txt_message", "Unknown Error");
		return p;
	}

	HMI_update_txt("new_fin_page.txt_message", "Remove finger from the scanner.");
	delay(3000);
	p = 0;

	tmp = millis();
	while (p != FINGERPRINT_NOFINGER) {
		p = myFinger.getImage();

		if (readHMI_Data(1)) {
			return 0;
		}

		if (millis() - tmp > PASSCODE_TIMEOUT) {
			Serial.println("Fingerprint enroll timeout-1");
			HMI_update_txt("new_fin_page.txt_message", "Timed out. Please try again.");
			HMI_set_component_visibility("new_fin_page.btn_cancel", 0);
			delay(2000);
			return 0;
		}
	}
	Serial.print("ID "); Serial.println(id);
	p = -1;
	HMI_update_txt("new_fin_page.txt_message", "Place same myFinger again");

	motionState = 1;

	tmp = millis();
	while (p != FINGERPRINT_OK) {
		if (readHMI_Data(1)) {
			return 0;
		}

		p = myFinger.getImage();
		switch (p) {
		case FINGERPRINT_OK:
			Serial.println("Image taken");
			break;
		case FINGERPRINT_NOFINGER:
			//Serial.print(".");
			break;
		case FINGERPRINT_PACKETRECIEVEERR:
			Serial.println("Communication error");
			HMI_update_txt("new_fin_page.txt_message", "Communication Error");
			break;
		case FINGERPRINT_IMAGEFAIL:
			Serial.println("Imaging error");
			HMI_update_txt("new_fin_page.txt_message", "Image Error");
			break;
		default:
			Serial.println("Unknown error");
			HMI_update_txt("new_fin_page.txt_message", "Unknown Error");
			break;
		}

		if (millis() - tmp > PASSCODE_TIMEOUT) {
			Serial.println("Fingerprint enroll timeout-2");
			HMI_update_txt("new_fin_page.txt_message", "Timed out. Please try again.");
			HMI_set_component_visibility("new_fin_page.btn_cancel", 1);
			delay(2000);
			return 0;
		}
	}

	motionState = 1;
	// OK success!

	p = myFinger.image2Tz(2);
	switch (p) {
	case FINGERPRINT_OK:
		Serial.println("Image converted");
		break;
	case FINGERPRINT_IMAGEMESS:
		Serial.println("Image too messy");
		HMI_update_txt("new_fin_page.txt_message", "Error! Image not clear");
		return p;
	case FINGERPRINT_PACKETRECIEVEERR:
		Serial.println("Communication error");
		HMI_update_txt("new_fin_page.txt_message", "Communication Error");
		return p;
	case FINGERPRINT_FEATUREFAIL:
		Serial.println("Could not find fingerprint features");
		HMI_update_txt("new_fin_page.txt_message", "Error finding features");
		return p;
	case FINGERPRINT_INVALIDIMAGE:
		Serial.println("Could not find fingerprint features");
		HMI_update_txt("new_fin_page.txt_message", "Error finding features");
		return p;
	default:
		Serial.println("Unknown error");
		HMI_update_txt("new_fin_page.txt_message", "Unknown Error");
		return p;
	}

	// OK converted!
	Serial.print("Creating model for #");  Serial.println(id);

	p = myFinger.createModel();
	if (p == FINGERPRINT_OK) {
		Serial.println("Prints matched!");
	}
	else if (p == FINGERPRINT_PACKETRECIEVEERR) {
		Serial.println("Communication error");
		HMI_update_txt("new_fin_page.txt_message", "Communication Error");
		return p;
	}
	else if (p == FINGERPRINT_ENROLLMISMATCH) {
		Serial.println("Fingerprints did not match");
		HMI_update_txt("new_fin_page.txt_message", "Fingerprint Mismatch Error!");
		return p;
	}
	else {
		Serial.println("Unknown error");
		HMI_update_txt("new_fin_page.txt_message", "Unknown Error");
		return p;
	}

	Serial.print("ID "); Serial.println(id);
	p = myFinger.storeModel(id);
	if (p == FINGERPRINT_OK) {
		String val = String(id) + "-" + name;
		Serial.print("Saving name: ");Serial.println(val);

		write_mem(name_addrs[id-1], MAX_NAME_LEN, val);				//Save the ID and name to memory
		
		Serial.println("Fingerprint saved.");
		HMI_update_txt("new_fin_page.txt_message", "Fingerprint saved.");
		HMI_update_txt("new_fin_page.txt_name", "");
		HMI_set_component_visibility("btn_cancel", 0);
		delay(2000);
		motionState = 1;
	}
	else if (p == FINGERPRINT_PACKETRECIEVEERR) {
		Serial.println("Communication error");
		HMI_update_txt("new_fin_page.txt_message", "Communication Error");
		return p;
	}
	else if (p == FINGERPRINT_BADLOCATION) {
		Serial.println("Could not store in that location");
		HMI_update_txt("new_fin_page.txt_message", "Memry Location Error");
		return p;
	}
	else if (p == FINGERPRINT_FLASHERR) {
		Serial.println("Error writing to flash");
		HMI_update_txt("new_fin_page.txt_message", "Error writing to flash");
		return p;
	}
	else {
		Serial.println("Unknown error");
		HMI_update_txt("new_fin_page.txt_message", "Unknown Error");
		return p;
	}
	return p;
}
