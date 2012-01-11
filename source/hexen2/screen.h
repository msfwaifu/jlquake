// screen.h

void SCR_Init (void);

void SCR_UpdateScreen (void);


void SCR_SizeUp (void);
void SCR_SizeDown (void);
void SCR_BringDownConsole (void);
void SCR_CenterPrint (char *str);

void SCR_BeginLoadingPlaque (void);
void SCR_EndLoadingPlaque (void);
void SCR_DrawLoading (void);

int SCR_ModalMessage (const char *text);

extern	float		scr_con_current;
extern	float		scr_conlines;		// lines of console to display

extern	int			sb_lines;

extern	Cvar*		scr_viewsize;

extern int			total_loading_size, current_loading_size, loading_stage;

void SCR_UpdateWholeScreen (void);
