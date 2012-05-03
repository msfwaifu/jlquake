
extern char* keybindings[256];
extern int key_count;				// incremented every key event
extern int key_lastpress;

void Key_Event(int key, qboolean down, unsigned time);
void Key_Init(void);
void Key_WriteBindings(fileHandle_t f);
void Key_SetBinding(int keynum, const char* binding);
void Key_ClearStates(void);


void IN_ProcessEvents();
