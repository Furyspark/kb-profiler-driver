#include "keyoptions.h"
#include "scancode_array.c"
#include "profile.h"


INT
KbProfiler_GetBindIndex
(
	IN USHORT scanCode
)
{
	for (INT a = 0; a < 256; a++) {
		if (scanCode == ScanCodes[a]) {
			return a;
		}
	}
	return 0;
}


BOOLEAN
KbProfiler_TargetKeymapIsBindEmpty
(
  IN PROFILE *profile,
  IN INT bindIndex,
  IN INT keymapIndex
)
{
  BINDING targetBind = (*profile).keymaps[keymapIndex].bindings[bindIndex];
  if(targetBind.code == SCANCODE_NONE) {
    return TRUE;
  }
  return FALSE;
}

VOID
KbProfiler_SwitchToKeymap
(
  IN PROFILE *profile,
  IN KEYOPTIONS *keyOpts,
  IN INT keymapIndex
)
{
  for(INT a = 0;a < 256;a++) {
    if(!(*keyOpts).keyHeld[a]) {
      if(!KbProfiler_TargetKeymapIsBindEmpty(profile, a, keymapIndex) || keymapIndex == 0) {
        (*keyOpts).keyInMap[a] = keymapIndex;
      }
    }
  }
  (*keyOpts).curMap = keymapIndex;
}

USHORT
KbProfiler_TranslateKeyString
(
	IN const char* keystr
)
{
	for (INT a = 0; a < 256; a++) {
		if (strcmp(ScanKeys[a], keystr) == 0) {
			return ScanCodes[a];
		}
	}
	return SCANCODE_NONE;
}

BOOLEAN
KbProfiler_IsE0Key
(
	IN const char* keystr
)
{
	char *values[16];
	INT a;
	for (a = 0; a < 16; a++) {
		values[a] = "";
	}
	values[0] = "numpadenter";
	values[1] = "numpadsub";
	values[2] = "home";
	values[3] = "pgup";
	values[4] = "pgdn";
	values[5] = "end";
	values[6] = "insert";
	values[7] = "delete";
	values[8] = "left";
	values[9] = "right";
	values[10] = "up";
	values[11] = "down";

	for (a = 0; a < 16; a++) {
		if (strcmp(keystr, values[a]) == 0) {
			return TRUE;
		}
	}

	return FALSE;
}
