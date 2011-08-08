// console.c

#include "quakedef.h"

int 		con_linewidth;

float		con_cursorspeed = 4;

#define		CON_TEXTSIZE	16384

qboolean 	con_forcedup;		// because no entities to refresh

int			con_totallines;		// total lines in console scrollback
int			con_backscroll;		// lines up from bottom to display
int			con_current;		// where next message will be printed
int			con_x;				// offset in current line for next print
static short		*con_text=0;

QCvar*		con_notifytime;

#define	NUM_CON_TIMES 4
float		con_times[NUM_CON_TIMES];	// realtime time the line was generated
								// for transparent notify lines

int			con_vislines;

qboolean	con_debuglog;

#define		MAXCMDLINE	256
extern	char	key_lines[32][MAXCMDLINE];
extern	int		edit_line;
extern	int		key_linepos;
		

qboolean	con_initialized;

extern void M_Menu_Main_f (void);

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void)
{
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		in_keyCatchers &= ~KEYCATCH_CONSOLE;
		if (cls.state == ca_connected)
		{
			key_lines[edit_line][1] = 0;	// clear any typing
			key_linepos = 1;
		}
		else
		{
			M_Menu_Main_f ();
		}
	}
	else
		in_keyCatchers |= KEYCATCH_CONSOLE;
	
	SCR_EndLoadingPlaque ();
	Com_Memset(con_times, 0, sizeof(con_times));
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
{
	short i;

	for(i=0;i<CON_TEXTSIZE;i++)
		con_text[i] = ' ';
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int		i;
	
	for (i=0 ; i<NUM_CON_TIMES ; i++)
		con_times[i] = 0;
}

						
/*
================
Con_MessageMode_f
================
*/
extern qboolean team_message;

void Con_MessageMode_f (void)
{
	in_keyCatchers |= KEYCATCH_MESSAGE;
	team_message = false;
}

						
/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
{
	in_keyCatchers |= KEYCATCH_MESSAGE;
	team_message = true;
}

						
/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	short	tbuf[CON_TEXTSIZE];

	width = (viddef.width >> 3) - 2;

	if (width == con_linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = 38;
		con_linewidth = width;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		Con_Clear_f();
	}
	else
	{
		oldwidth = con_linewidth;
		con_linewidth = width;
		oldtotallines = con_totallines;
		con_totallines = CON_TEXTSIZE / con_linewidth;
		numlines = oldtotallines;

		if (con_totallines < numlines)
			numlines = con_totallines;

		numchars = oldwidth;
	
		if (con_linewidth < numchars)
			numchars = con_linewidth;

		Com_Memcpy(tbuf, con_text, CON_TEXTSIZE<<1);
		Con_Clear_f();

		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con_text[(con_totallines - 1 - i) * con_linewidth + j] =
						tbuf[((con_current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con_backscroll = 0;
	con_current = con_totallines - 1;
}


/*
================
Con_Init
================
*/
void Con_Init (void)
{
	const char	*t2 = "qconsole.log";

	con_debuglog = COM_CheckParm("-condebug");

	if (con_debuglog)
	{
		FS_FCloseFile(FS_FOpenFileWrite(t2));
	}

	con_text = (short*)Hunk_AllocName (CON_TEXTSIZE<<1, "context");
	Con_Clear_f();
	con_linewidth = -1;
	Con_CheckResize ();
	
	Con_Printf ("Console initialized.\n");

//
// register our commands
//
	con_notifytime = Cvar_Get("con_notifytime", "3", 0);		//seconds

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	con_initialized = true;

	PR_LoadStrings();
#ifdef MISSIONPACK
	PR_LoadInfoStrings();
#endif
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (void)
{
	int i,j;

	con_x = 0;
	con_current++;

	j = (con_current%con_totallines)*con_linewidth;
	for(i=0;i<con_linewidth;i++)
		con_text[i+j] = ' ';
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the notify window will pop up.
================
*/
void Con_Print (const char *txt)
{
	int		y;
	int		c, l;
	static int	cr;
	int		mask;
	
	con_backscroll = 0;

	if (txt[0] == 1)
	{
		mask = 256;		// go to colored text
		S_StartLocalSound("misc/comm.wav");
	// play talk wav
		txt++;
	}
	else if (txt[0] == 2)
	{
		mask = 256;		// go to colored text
		txt++;
	}
	else
		mask = 0;


	while ( (c = *txt) )
	{
	// count word length
		for (l=0 ; l< con_linewidth ; l++)
			if ( txt[l] <= ' ')
				break;

	// word wrap
		if (l != con_linewidth && (con_x + l > con_linewidth) )
			con_x = 0;

		txt++;

		if (cr)
		{
			con_current--;
			cr = false;
		}

		
		if (!con_x)
		{
			Con_Linefeed ();
		// mark time for transparent overlay
			if (con_current >= 0)
				con_times[con_current % NUM_CON_TIMES] = realtime;
		}

		switch (c)
		{
		case '\n':
			con_x = 0;
			break;

		case '\r':
			con_x = 0;
			cr = 1;
			break;

		default:	// display character and advance
			y = con_current % con_totallines;
			con_text[y*con_linewidth+con_x] = c | mask;
			con_x++;
			if (con_x >= con_linewidth)
				con_x = 0;
			break;
		}
		
	}
}


/*
================
Con_DebugLog
================
*/
void Con_DebugLog(const char *file, const char *fmt, ...)
{
	va_list argptr; 
	static char data[1024];
	fileHandle_t fd;

	va_start(argptr, fmt);
	Q_vsnprintf(data, sizeof(data), fmt, argptr);
	va_end(argptr);
	FS_FOpenFileByMode(file, &fd, FS_APPEND);
	FS_Write(data, String::Length(data), fd);
	FS_FCloseFile(fd);
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Con_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;
	
	va_start (argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end (argptr);
	
// also echo to debugging console
	Sys_Print(msg);	// also echo to debugging console

// log all messages to file
	if (con_debuglog)
		Con_DebugLog("qconsole.log", "%s", msg);

	if (!con_initialized)
		return;
		
	if (cls.state == ca_dedicated)
		return;		// no graphics mode

// write it to the scrollable buffer
	Con_Print (msg);
	
// update the screen if the console is displayed
	if (cls.signon != SIGNONS && !scr_disabled_for_loading )
	{
	// protect against infinite loop if something in SCR_UpdateScreen calls
	// Con_Printd
		if (!inupdate)
		{
			inupdate = true;
			SCR_UpdateScreen ();
			inupdate = false;
		}
	}
}

/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
		
	if (!developer || !developer->value)
		return;			// don't confuse non-developers with techie stuff...

	va_start (argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end (argptr);
	
	Con_Printf ("%s", msg);
}


/*
==================
Con_SafePrintf

Okay to call even when the screen can't be updated
==================
*/
void Con_SafePrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	int			temp;
		
	va_start (argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end (argptr);

	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = true;
	Con_Printf ("%s", msg);
	scr_disabled_for_loading = temp;
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput (void)
{
	int		y;
	int		i;
	char	*text;

	if (!(in_keyCatchers & KEYCATCH_CONSOLE) && !con_forcedup)
		return;		// don't draw anything

	text = key_lines[edit_line];
	
// add the cursor frame
	text[key_linepos] = 10+((int)(realtime*con_cursorspeed)&1);
	
// fill out remainder with spaces
	for (i=key_linepos+1 ; i< con_linewidth ; i++)
		text[i] = ' ';
		
//	prestep if horizontally scrolling
	if (key_linepos >= con_linewidth)
		text += 1 + key_linepos - con_linewidth;
		
// draw it
	y = con_vislines-16;

	for (i=0 ; i<con_linewidth ; i++)
		Draw_Character ( (i+1)<<3, con_vislines - 16, text[i]);

// remove cursor
	key_lines[edit_line][key_linepos] = 0;
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	short	*text;
	int		i;
	float	time;
	extern char chat_buffer[];

	v = 0;
	for (i= con_current-NUM_CON_TIMES+1 ; i<=con_current ; i++)
	{
		if (i < 0)
			continue;
		time = con_times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = realtime - time;
		if (time > con_notifytime->value)
			continue;
		text = con_text + (i % con_totallines)*con_linewidth;

		for (x = 0 ; x < con_linewidth ; x++)
			Draw_Character ( (x+1)<<3, v, text[x]);

		v += 8;
	}


	if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		x = 0;
		
		Draw_String (8, v, "say:");
		while(chat_buffer[x])
		{
			Draw_Character ( (x+5)<<3, v, chat_buffer[x]);
			x++;
		}
		Draw_Character ( (x+5)<<3, v, 10+((int)(realtime*con_cursorspeed)&1));
		v += 8;
	}
}

/*
================
Con_DrawConsole

Draws the console with the solid background
The typing input line at the bottom should only be drawn if typing is allowed
================
*/
void Con_DrawConsole (int lines, qboolean drawinput)
{
	int				i, x, y;
	int				rows;
	short			*text;
	int				j;
	
	if (lines <= 0)
		return;

// draw the background
	Draw_ConsoleBackground (lines);

// draw the text
	con_vislines = lines;

	rows = (lines-16)>>3;		// rows of text to draw
	y = lines - 16 - (rows<<3);	// may start slightly negative

	for (i= con_current - rows + 1 ; i<=con_current ; i++, y+=8 )
	{
		j = i - con_backscroll;
		if (j<0)
			j = 0;
		text = con_text + (j % con_totallines)*con_linewidth;

		for (x=0 ; x<con_linewidth ; x++)
			Draw_Character ( (x+1)<<3, y, text[x]);
	}

// draw the input prompt, user text, and cursor if desired
	if (drawinput)
		Con_DrawInput ();
}


/*
==================
Con_NotifyBox
==================
*/
void Con_NotifyBox (const char *text)
{
	double		t1, t2;

// during startup for sound / cd warnings
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	Con_Printf (text);

	Con_Printf ("Press a key.\n");
	Con_Printf("\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n");

	key_count = -2;		// wait for a key down and up
	in_keyCatchers |= KEYCATCH_CONSOLE;

	do
	{
		t1 = Sys_DoubleTime();
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();
		IN_ProcessEvents();
		t2 = Sys_DoubleTime();
		realtime += t2-t1;		// make the cursor blink
	} while (key_count < 0);

	Con_Printf ("\n");
	in_keyCatchers &= ~KEYCATCH_CONSOLE;
	realtime = 0;				// put the cursor back to invisible
}

