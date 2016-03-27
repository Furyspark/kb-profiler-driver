/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    KBFTEST.C

Abstract:


Environment:

    usermode console application

--*/

#define _CRT_SECURE_NO_WARNINGS


#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <initguid.h>
#include <conio.h>
#include <ntddkbd.h>
#include <stdio.h>
#include <string.h>

#include <windows.h>
#include "cJSON.c"

// #include "..\sys\scancode_array.c"

#pragma warning(disable:4201)

#include <setupapi.h>
#include <winioctl.h>

#pragma warning(default:4201)

#include "..\sys\public.h"
#include "..\sys\profile_functions.c"

/**
 * START HELPER FUNCTIONS
 */

#pragma warning(push)
#pragma warning(disable: 4267)
INT
StringToInt
(
	IN const char* str
)
{
	INT a, len;
	INT result = 0;
	len = strlen(str);
	for (a = 0; a < len; a++) {
		result = result * 10 + (str[a] - '0');
	}
	return result;
}
#pragma warning(pop)

VOID
KbProfiler_ClearProfile(
	IN PROFILE *profile
)
{
	(*profile).empty = TRUE;

	for (INT a = 0; a < 32; a++) {
		for (INT b = 0; b < 256; b++) {
			(*profile).keymaps[a].bindings[b].code = SCANCODE_NONE;
			(*profile).keymaps[a].bindings[b].alt = FALSE;
			(*profile).keymaps[a].bindings[b].shift = FALSE;
			(*profile).keymaps[a].bindings[b].ctrl = FALSE;
			(*profile).keymaps[a].bindings[b].codeE0 = FALSE;
			(*profile).keymaps[a].bindings[b].originE0 = FALSE;
			(*profile).keymaps[a].bindings[b].keymap = 0;
			(*profile).keymaps[a].bindings[b].index = b;
			(*profile).keymaps[a].bindings[b].toggle = FALSE;
			(*profile).keymaps[a].bindings[b].rapidfire = 0;
		}
	}
}

VOID
KbProfiler_ParseProfileJSON
(
	IN cJSON *json,
	IN PROFILE *profile
)
{
	printf("JSON Parse\n");
	cJSON * keymaps = cJSON_GetObjectItem(json, "keymaps");
	INT keymapCount = cJSON_GetArraySize(keymaps);

	cJSON * bindings = cJSON_GetObjectItem(json, "bindings");
	for (INT a = 0; a < keymapCount; a++) {
		cJSON * keymapBindings = cJSON_GetArrayItem(bindings, a);
		INT bindingsLen = cJSON_GetArraySize(keymapBindings);
		// Parse individual bindings
		for (INT b = 0; b < bindingsLen; b++) {
			cJSON * bind = cJSON_GetArrayItem(keymapBindings, b);
			cJSON * bindCode = cJSON_GetObjectItem(bind, "key");
			char *keyStr = bindCode->valuestring;
			cJSON * originCode = cJSON_GetObjectItem(bind, "origin");
			char *originStr = originCode->valuestring;

			INT bindIndex = KbProfiler_GetBindIndex(KbProfiler_TranslateKeyString(keyStr));
			INT originIndex = KbProfiler_GetBindIndex(KbProfiler_TranslateKeyString(originStr));

			// Parse E0
			if (KbProfiler_IsE0Key(keyStr)) {
				(*profile).keymaps[a].bindings[originIndex].codeE0 = TRUE;
			}
			if (KbProfiler_IsE0Key(originStr)) {
				(*profile).keymaps[a].bindings[originIndex].originE0 = TRUE;
			}

			// Determine whether keymap action
			char keymapTest[7];
			strncpy(keymapTest, keyStr, 6);
			keymapTest[6] = '\0';
			// Parse keymap usage
			if (strcmp(keymapTest, "keymap") == 0 || strcmp(keymapTest, "Keymap") == 0) {
				char keymapTemp[4];
				strncpy(keymapTemp, &keyStr[6], sizeof(keyStr)-6);
				keymapTemp[3] = '\0';
				INT keymapIndex = StringToInt(keymapTemp)-1;
				(*profile).keymaps[a].bindings[originIndex].keymap = keymapIndex;
			}
			else {
				// Parse key code
				(*profile).keymaps[a].bindings[originIndex].code = ScanCodes[bindIndex];
				// Parse shift usage
				cJSON * shift = cJSON_GetObjectItem(bind, "shift");
				if (shift->type == cJSON_String) {
					if (strcmp(shift->valuestring, "1") == 0) {
						(*profile).keymaps[a].bindings[originIndex].shift = TRUE;
					}
				}
				else if (shift->type == cJSON_True) {
					(*profile).keymaps[a].bindings[originIndex].shift = TRUE;
				}
				// Parse ctrl usage
				cJSON * ctrl = cJSON_GetObjectItem(bind, "ctrl");
				if (ctrl->type == cJSON_String) {
					if (strcmp(ctrl->valuestring, "1") == 0) {
						(*profile).keymaps[a].bindings[originIndex].ctrl = TRUE;
					}
				}
				else if (ctrl->type == cJSON_True) {
					(*profile).keymaps[a].bindings[originIndex].ctrl = TRUE;
				}
				// Parse alt usage
				cJSON * alt= cJSON_GetObjectItem(bind, "alt");
				if (alt->type == cJSON_String) {
					if (strcmp(alt->valuestring, "1") == 0) {
						(*profile).keymaps[a].bindings[originIndex].alt = TRUE;
					}
				}
				else if (alt->type == cJSON_True) {
					(*profile).keymaps[a].bindings[originIndex].alt = TRUE;
				}
				// Parse rapid fire
				cJSON * rapidfire = cJSON_GetObjectItem(bind, "rapidfire");
				if (rapidfire->type == cJSON_String && strcmp(rapidfire->valuestring, "0") != 0) {
					INT rapidfireValue = StringToInt(rapidfire->valuestring);
					(*profile).keymaps[a].bindings[originIndex].rapidfire = rapidfireValue;
				}
				else if (rapidfire->type == cJSON_Number && rapidfire->valueint != 0) {
					(*profile).keymaps[a].bindings[originIndex].rapidfire = rapidfire->valueint;
				}
				// Parse toggle
				cJSON * toggle = cJSON_GetObjectItem(bind, "toggle");
				if (toggle->type == cJSON_String && strcmp(toggle->valuestring, "0") != 0) {
					(*profile).keymaps[a].bindings[originIndex].toggle = TRUE;
				}
				else if (toggle->type == cJSON_Number && toggle->valueint != 0) {
					(*profile).keymaps[a].bindings[originIndex].toggle = TRUE;
				}
				else if (toggle->type == cJSON_True) {
					(*profile).keymaps[a].bindings[originIndex].toggle = TRUE;
				}
			}
		}
	}
}

/**
 * END HELPER FUNCTIONS
 */

//-----------------------------------------------------------------------------
// 4127 -- Conditional Expression is Constant warning
//-----------------------------------------------------------------------------
#define WHILE(constant) \
__pragma(warning(disable: 4127)) while(constant); __pragma(warning(default: 4127))

DEFINE_GUID(GUID_DEVINTERFACE_KBFILTER,
0x3fb7299d, 0x6847, 0x4490, 0xb0, 0xc9, 0x99, 0xe0, 0x98, 0x6a, 0xb8, 0x86);
// {3FB7299D-6847-4490-B0C9-99E0986AB886}


int
_cdecl
main(
	_In_ int argc,
	_In_ char *argv[]
)
{
	BOOLEAN clearProfile = FALSE;
	BOOLEAN jsonLoaded = FALSE;
	cJSON * jsonRoot;

	// Initialize scan code array
	KbProfiler_InitScanCodes();

	// Initialize profile
	PROFILE profile;
	KbProfiler_ClearProfile(&profile);

	/**
	 * Parse arguments
	 */
	for (INT a = 1; a < argc; a++) {
		char param[2048];
		char cmdBase[2048];
		char cmdValue[2048];
		strcpy(param, argv[a]);

		char *p = strchr(param, '=');
		// Single argument
		if (!p) {
			strcpy(cmdBase, param);
		}
		// Argument with value
		else {
			*p++;
			strcpy(cmdValue, p);
			strncpy(cmdBase, param, sizeof(p) - 1);
			cmdBase[sizeof(p) - 1] = '\0';
			// Profile arguments
			if (strcmp(cmdBase, "profile") == 0) {
				// Clear profile
				if (strcmp(cmdValue, "clear") == 0) {
					printf("Clearing profile.\n");
					clearProfile = TRUE;
				}
				// Load profile
				else {
					FILE *f;
					if (f = fopen(cmdValue, "rb")) {
						fseek(f, 0, SEEK_END);
						long fsize = ftell(f);
						fseek(f, 0, SEEK_SET);

						char *json = malloc(fsize + 1);
						fread(json, fsize, 1, f);
						fclose(f);

						jsonRoot = cJSON_Parse(json);
						jsonLoaded = TRUE;
					}
					else {
						printf("Profile JSON file does not exist there.");
					}
				}
			}
		}
	}

	// Parse profile
	profile.empty = FALSE;
	if (clearProfile) {
		profile.empty = TRUE;
	}

	if (jsonLoaded == TRUE) {
		KbProfiler_ParseProfileJSON(jsonRoot, &profile);
		cJSON_Delete(jsonRoot);
	}

	/**
	 * END USER STUFF
	 */
	HDEVINFO                            hardwareDeviceInfo;
	SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;
	ULONG                               predictedLength = 0;
	ULONG                               requiredLength = 0, bytes = 0, profileBytes = 0;
	HANDLE                              file;
	ULONG                               i = 0;
	KEYBOARD_ATTRIBUTES                 kbdattrib;

	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	//
	// Open a handle to the device interface information set of all
	// present toaster class interfaces.
	//

	hardwareDeviceInfo = SetupDiGetClassDevs(
		(LPGUID)&GUID_DEVINTERFACE_KBFILTER,
		NULL, // Define no enumerator (global)
		NULL, // Define no
		(DIGCF_PRESENT | // Only Devices present
			DIGCF_DEVICEINTERFACE)); // Function class devices.
	if (INVALID_HANDLE_VALUE == hardwareDeviceInfo)
	{
		printf("SetupDiGetClassDevs failed: %x\n", GetLastError());
		return 0;
	}

	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	printf("\nList of KBFILTER Device Interfaces\n");
	printf("---------------------------------\n");

	i = 0;

	//
	// Enumerate devices of toaster class
	//

	do {
		if (SetupDiEnumDeviceInterfaces(hardwareDeviceInfo,
			0, // No care about specific PDOs
			(LPGUID)&GUID_DEVINTERFACE_KBFILTER,
			i, //
			&deviceInterfaceData)) {

			if (deviceInterfaceDetailData) {
				free(deviceInterfaceDetailData);
				deviceInterfaceDetailData = NULL;
			}

			//
			// Allocate a function class device data structure to
			// receive the information about this particular device.
			//

			//
			// First find out required length of the buffer
			//

			if (!SetupDiGetDeviceInterfaceDetail(
				hardwareDeviceInfo,
				&deviceInterfaceData,
				NULL, // probing so no output buffer yet
				0, // probing so output buffer length of zero
				&requiredLength,
				NULL)) { // not interested in the specific dev-node
				if (ERROR_INSUFFICIENT_BUFFER != GetLastError()) {
					printf("SetupDiGetDeviceInterfaceDetail failed %d\n", GetLastError());
					SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
					return FALSE;
				}

			}

			predictedLength = requiredLength;

			deviceInterfaceDetailData = malloc(predictedLength);

			if (deviceInterfaceDetailData) {
				deviceInterfaceDetailData->cbSize =
					sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			}
			else {
				printf("Couldn't allocate %d bytes for device interface details.\n", predictedLength);
				SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
				return FALSE;
			}


			if (!SetupDiGetDeviceInterfaceDetail(
				hardwareDeviceInfo,
				&deviceInterfaceData,
				deviceInterfaceDetailData,
				predictedLength,
				&requiredLength,
				NULL)) {
				printf("Error in SetupDiGetDeviceInterfaceDetail\n");
				SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
				free(deviceInterfaceDetailData);
				return FALSE;
			}
			printf("%d) %s\n", ++i,
				deviceInterfaceDetailData->DevicePath);
		}
		else if (ERROR_NO_MORE_ITEMS != GetLastError()) {
			free(deviceInterfaceDetailData);
			deviceInterfaceDetailData = NULL;
			continue;
		}
		else
			break;

	} WHILE(TRUE);


	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

	if (!deviceInterfaceDetailData)
	{
		printf("No device interfaces present\n");
		return 0;
	}

	//
	// Open the last toaster device interface
	//

	printf("\nOpening the last interface:\n %s\n",
		deviceInterfaceDetailData->DevicePath);

	file = CreateFile(deviceInterfaceDetailData->DevicePath,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL, // no SECURITY_ATTRIBUTES structure
		OPEN_EXISTING, // No special create flags
		0, // No special attributes
		NULL);

	if (INVALID_HANDLE_VALUE == file) {
		printf("Error in CreateFile: %x", GetLastError());
		free(deviceInterfaceDetailData);
		return 0;
	}

	//
	// Send an IOCTL to retrive the keyboard attributes
	// These are cached in the kbfiltr
	//

	if (!DeviceIoControl(file,
		IOCTL_KBFILTR_GET_KEYBOARD_ATTRIBUTES,
		NULL, 0,
		&kbdattrib, sizeof(kbdattrib),
		&bytes, NULL)) {
		printf("Retrieve Keyboard Attributes request failed:0x%x\n", GetLastError());
		free(deviceInterfaceDetailData);
		CloseHandle(file);
		return 0;
	}

	printf("\nKeyboard Attributes:\n"
		" KeyboardMode:          0x%x\n"
		" NumberOfFunctionKeys:  0x%x\n"
		" NumberOfIndicators:    0x%x\n"
		" NumberOfKeysTotal:     0x%x\n"
		" InputDataQueueLength:  0x%x\n",
		kbdattrib.KeyboardMode,
		kbdattrib.NumberOfFunctionKeys,
		kbdattrib.NumberOfIndicators,
		kbdattrib.NumberOfKeysTotal,
		kbdattrib.InputDataQueueLength);
	printf("Bytes pushed: %d\n", bytes);

	/**
	 * USER STUFF
	 */
	

	//KbProfiler_LoadProfile(&profile, );


	/*profile.keymaps[0].bindings[KbProfiler_GetBindIndex(SCANCODE_D)].code = SCANCODE_Z;
	profile.keymaps[0].bindings[KbProfiler_GetBindIndex(SCANCODE_D)].shift = TRUE;
	profile.keymaps[0].bindings[KbProfiler_GetBindIndex(SCANCODE_S)].code = SCANCODE_X;
	profile.keymaps[1].bindings[KbProfiler_GetBindIndex(SCANCODE_S)].code = SCANCODE_G;
	profile.keymaps[0].bindings[KbProfiler_GetBindIndex(SCANCODE_A)].keymap = 1;
	profile.empty = FALSE;*/

	if (!DeviceIoControl(file,
		IOCTL_KBFILTR_GET_PROFILE,
		&profile, sizeof(PROFILE),
		NULL, 0,
		&profileBytes, NULL)) {

		printf("Pushing profile failed.\n");
	}
	else {
		printf("Successfully pushed profile to driver.\n");
		printf("Bytes pushed: %d\n", profileBytes);
	}


	free(deviceInterfaceDetailData);
	CloseHandle(file);

	return 0;
}


