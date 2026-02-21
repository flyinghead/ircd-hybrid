static int parseAlphaHex(const char *buf, int digits) {
	int i;
	int r = 0;
	for (i = 0 ; i < digits ; ++i) {
		int c = toupper(buf[i]);
		c -= 'A';
		if (c < 0) {
			return -1;
		}
		if (c > 15) {
			return -1;
		}
		r = (r << 4) + c;
	}
	return r;
}

static void printAlphaHex(char *buf, unsigned value, int digits) {
	int i;
	for (i = 0 ; i < digits ; ++i) {
		unsigned digValue = (value >> (unsigned)((digits - i - 1)*4)) & 0xf;
		buf[i] = 'A' + digValue;
	}

	assert(parseAlphaHex(buf, digits) == value);
}

static void computeNickPasswordCheckSum(const char nick[12], unsigned usrIp, unsigned randomVal, char *result) {

	int rounds, i, j;

	unsigned r = usrIp;
	for (rounds = 0 ; rounds < 10 ; ++rounds) {
		for (i = 0 ; i < 12 ; ++i) {
			unsigned c = nick[i];
			r ^= c << 24;
			r ^= randomVal;
			for (j = 0 ; j < 8 ; ++j) {
				if (r & 0x80000000) {
					r = (r << 1U) ^ 0xa67f3443;
				} else {
					r = (r << 1U) ^ 0x378624e5;
				}
			}
			r += c;
		}
	}
	r &= 0xffff;
	printAlphaHex(result, r, 4);
	result[4] = '\0';
}

//static void generateNickPassword(const char nick[12], unsigned usrIp, char *result) {
//	unsigned randomVal = rand() & 0xff;
//	printAlphaHex(result, randomVal, 2);
//	computeNickPasswordCheckSum(nick, usrIp, randomVal, result+2);
//}

static char verifyNickPasswordMsg[512];

static int verifyNickPasswordEvo1(const char nick[12], unsigned usrIp, const char *password) {

	int	randomVal;
	char	check[20];
	int	i, l;

	// Password must be the correct length

	if (password == NULL) {
		strcpy(verifyNickPasswordMsg, "NULL password");
		return 0;
	}
	l = strlen(password);
	if (l != 6) {
		sprintf(verifyNickPasswordMsg, "l=%d", l);
		return 0;
	}

	// Fetch out the random value from the first nibble

	randomVal = parseAlphaHex(password, 2);
	sprintf(verifyNickPasswordMsg, "randomVal=%d", randomVal);
	if ((randomVal < 0) || (randomVal > 0xff)) {
		return 0;
	}

	// Figure out what the password should be

	computeNickPasswordCheckSum(nick, usrIp, (unsigned)randomVal, check);

	// Verify the password

	for (i = 2 ; i < l ; ++i) {
		if (toupper(password[i]) != toupper(check[i-2])) {
			sprintf(verifyNickPasswordMsg, "randomVal=%d, check=%s, password=%d", randomVal, check, password);
			return 0;
		}
	}

	// Password is OK

	return 1;
}

static int verifyNickPasswordEvo2(const char nick[12], unsigned usrIp, const char *password, unsigned challenge) {

	int	randomVal;
	char	check[20];
	int	i, l;

	// Password must be the correct length

	if (password == NULL) {
		strcpy(verifyNickPasswordMsg, "NULL password");
		return 0;
	}
	l = strlen(password);
	if (l != 4) {
		sprintf(verifyNickPasswordMsg, "l=%d", l);
		return 0;
	}

	// Figure out what the password should be

	computeNickPasswordCheckSum(nick, usrIp, challenge, check);

	// Verify the password

	for (i = 2 ; i < l ; ++i) {
		if (toupper(password[i]) != toupper(check[i])) {
                    sprintf(verifyNickPasswordMsg, "challenge=%x, check=%s, password=%d", challenge, check, password);
			return 0;
		}
	}

	// Password is OK

	return 1;
}
