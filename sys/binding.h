

typedef struct BINDING {
	USHORT code;
	BOOLEAN alt;
	BOOLEAN shift;
	BOOLEAN ctrl;
	BOOLEAN codeE0;
	BOOLEAN originE0;
	INT keymap;
	INT index;
	BOOLEAN toggle;
	INT rapidfire;
} BINDING;