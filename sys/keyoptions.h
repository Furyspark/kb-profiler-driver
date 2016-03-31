typedef struct KEYOPTIONS {
	BOOLEAN keyHeld[256];
	BOOLEAN keyToggled[256];
	BOOLEAN keyRapidfireState[256];
	INT keyRapidfireCountdown[256];
	HANDLE rapidfireThread[256];
	INT keyInMap[256];
	INT curMap;
} KEYOPTIONS;
