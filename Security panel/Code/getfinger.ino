// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
	//return -1;
	int p = myFinger.getImage();
	if (p != FINGERPRINT_OK)  return -1;

	p = myFinger.image2Tz(1);
	if (p != FINGERPRINT_OK)  return -1;

	p = myFinger.fingerFastSearch();
	if (p != FINGERPRINT_OK)  return -1;

	// found a match!
	Serial.print("Found ID #"); Serial.print(myFinger.fingerID);
	Serial.print(" with confidence of "); Serial.println(myFinger.confidence);
	return myFinger.fingerID;
	//return -1;
}
