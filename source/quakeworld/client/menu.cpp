/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "quakedef.h"

extern Cvar* r_gamma;

void M_Keys_Draw(void);
void M_Video_Draw(void);
void M_Help_Draw(void);
void M_Quit_Draw(void);
void M_SerialConfig_Draw(void);
void M_ModemConfig_Draw(void);

void M_Keys_Key(int key);
void M_Video_Key(int key);
void M_Help_Key(int key);
void M_Quit_Key(int key);
void M_SerialConfig_Key(int key);
void M_ModemConfig_Key(int key);

qboolean m_recursiveDraw;

//=============================================================================

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f(void)
{
	mqh_entersound = true;

	if (in_keyCatchers & KEYCATCH_UI)
	{
		if (m_state != m_main)
		{
			MQH_Menu_Main_f();
			return;
		}
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		return;
	}
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		Con_ToggleConsole_f();
	}
	else
	{
		MQH_Menu_Main_f();
	}
}


//=============================================================================
/* KEYS MENU */

const char* bindnames[][2] =
{
	{"+attack",         "attack"},
	{"impulse 10",      "change weapon"},
	{"+jump",           "jump / swim up"},
	{"+forward",        "walk forward"},
	{"+back",           "backpedal"},
	{"+left",           "turn left"},
	{"+right",          "turn right"},
	{"+speed",          "run"},
	{"+moveleft",       "step left"},
	{"+moveright",      "step right"},
	{"+strafe",         "sidestep"},
	{"+lookup",         "look up"},
	{"+lookdown",       "look down"},
	{"centerview",      "center view"},
	{"+mlook",          "mouse look"},
	{"+moveup",         "swim up"},
	{"+movedown",       "swim down"}
};

#define NUMCOMMANDS (sizeof(bindnames) / sizeof(bindnames[0]))

int keys_cursor;
int bind_grab;

void M_FindKeysForCommand(const char* command, int* twokeys)
{
	int count;
	int j;
	int l;
	char* b;

	twokeys[0] = twokeys[1] = -1;
	l = String::Length(command);
	count = 0;

	for (j = 0; j < 256; j++)
	{
		b = keys[j].binding;
		if (!b)
		{
			continue;
		}
		if (!String::NCmp(b, command, l))
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
			{
				break;
			}
		}
	}
}

void M_UnbindCommand(const char* command)
{
	int j;
	int l;
	char* b;

	l = String::Length(command);

	for (j = 0; j < 256; j++)
	{
		b = keys[j].binding;
		if (!b)
		{
			continue;
		}
		if (!String::NCmp(b, command, l))
		{
			Key_SetBinding(j, "");
		}
	}
}


void M_Keys_Draw(void)
{
	int i, l;
	int keys[2];
	const char* name;
	int x, y;
	image_t* p;

	p = R_CachePic("gfx/ttl_cstm.lmp");
	MQH_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	if (bind_grab)
	{
		MQH_Print(12, 32, "Press a key or button for this action");
	}
	else
	{
		MQH_Print(18, 32, "Enter to change, backspace to clear");
	}

// search for known bindings
	for (i = 0; i < (int)NUMCOMMANDS; i++)
	{
		y = 48 + 8 * i;

		MQH_Print(16, y, bindnames[i][1]);

		l = String::Length(bindnames[i][0]);

		M_FindKeysForCommand(bindnames[i][0], keys);

		if (keys[0] == -1)
		{
			MQH_Print(140, y, "???");
		}
		else
		{
			name = Key_KeynumToString(keys[0]);
			MQH_Print(140, y, name);
			x = String::Length(name) * 8;
			if (keys[1] != -1)
			{
				MQH_Print(140 + x + 8, y, "or");
				MQH_Print(140 + x + 32, y, Key_KeynumToString(keys[1]));
			}
		}
	}

	if (bind_grab)
	{
		MQH_DrawCharacter(130, 48 + keys_cursor * 8, '=');
	}
	else
	{
		MQH_DrawCharacter(130, 48 + keys_cursor * 8, 12 + ((int)(realtime * 4) & 1));
	}
}


void M_Keys_Key(int k)
{
	char cmd[80];
	int keys[2];

	if (bind_grab)
	{	// defining a key
		S_StartLocalSound("misc/menu1.wav");
		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf(cmd, "bind %s \"%s\"\n", Key_KeynumToString(k), bindnames[keys_cursor][0]);
			Cbuf_InsertText(cmd);
		}

		bind_grab = false;
		return;
	}

	switch (k)
	{
	case K_ESCAPE:
		MQH_Menu_Options_f();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
		{
			keys_cursor = NUMCOMMANDS - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("misc/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= (int)NUMCOMMANDS)
		{
			keys_cursor = 0;
		}
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand(bindnames[keys_cursor][0], keys);
		S_StartLocalSound("misc/menu2.wav");
		if (keys[1] != -1)
		{
			M_UnbindCommand(bindnames[keys_cursor][0]);
		}
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_StartLocalSound("misc/menu2.wav");
		M_UnbindCommand(bindnames[keys_cursor][0]);
		break;
	}
}

//=============================================================================
/* VIDEO MENU */

void M_Video_Draw(void)
{
	image_t* p = R_CachePic("gfx/vidmodes.lmp");
	MQH_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	MQH_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 2,
		"Video modes must be set from the");
	MQH_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 3,
		"console with set r_mode <number>");
	MQH_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 4,
		"and set r_colorbits <bits-per-pixel>");
	MQH_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 6,
		"Select windowed mode with set r_fullscreen 0");
}


void M_Video_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		S_StartLocalSound("misc/menu1.wav");
		MQH_Menu_Options_f();
		break;

	default:
		break;
	}
}

//=============================================================================
/* HELP MENU */

void M_Help_Draw(void)
{
	MQH_DrawPic(0, 0, R_CachePic(va("gfx/help%i.lmp", mqh_help_page)));
}


void M_Help_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		MQH_Menu_Main_f();
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
		mqh_entersound = true;
		if (++mqh_help_page >= NUM_HELP_PAGES_Q1)
		{
			mqh_help_page = 0;
		}
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		mqh_entersound = true;
		if (--mqh_help_page < 0)
		{
			mqh_help_page = NUM_HELP_PAGES_Q1 - 1;
		}
		break;
	}

}

//=============================================================================
/* QUIT MENU */

void M_Quit_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			mqh_entersound = true;
		}
		else
		{
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
		in_keyCatchers |= KEYCATCH_CONSOLE;
		CL_Disconnect();
		Sys_Quit();
		break;

	default:
		break;
	}

}

void M_Quit_Draw(void)
{
#define VSTR(x) # x
#define VSTR2(x) VSTR(x)
	const char* cmsg[] = {
//    0123456789012345678901234567890123456789
		"0            QuakeWorld",
		"1    version " VSTR2(VERSION) " by id Software",
		"0Programming",
		"1 John Carmack    Michael Abrash",
		"1 John Cash       Christian Antkow",
		"0Additional Programming",
		"1 Dave 'Zoid' Kirsch",
		"1 Jack 'morbid' Mathews",
		"0Id Software is not responsible for",
		"0providing technical support for",
		"0QUAKEWORLD(tm). (c)1996 Id Software,",
		"0Inc.  All Rights Reserved.",
		"0QUAKEWORLD(tm) is a trademark of Id",
		"0Software, Inc.",
		"1NOTICE: THE COPYRIGHT AND TRADEMARK",
		"1NOTICES APPEARING  IN YOUR COPY OF",
		"1QUAKE(r) ARE NOT MODIFIED BY THE USE",
		"1OF QUAKEWORLD(tm) AND REMAIN IN FULL",
		"1FORCE.",
		"0NIN(r) is a registered trademark",
		"0licensed to Nothing Interactive, Inc.",
		"0All rights reserved. Press y to exit",
		NULL
	};
	const char** p;
	int y;

	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw();
		m_state = m_quit;
	}
#if 1
	MQH_DrawTextBox(0, 0, 38, 23);
	y = 12;
	for (p = cmsg; *p; p++, y += 8)
	{
		if (**p == '0')
		{
			MQH_PrintWhite(16, y, *p + 1);
		}
		else
		{
			MQH_Print(16, y, *p + 1);
		}
	}
#else
	MQH_DrawTextBox(56, 76, 24, 4);
	MQH_Print(64, 84,  mq1_quitMessage[msgNumber * 4 + 0]);
	MQH_Print(64, 92,  mq1_quitMessage[msgNumber * 4 + 1]);
	MQH_Print(64, 100, mq1_quitMessage[msgNumber * 4 + 2]);
	MQH_Print(64, 108, mq1_quitMessage[msgNumber * 4 + 3]);
#endif
}



//=============================================================================
/* Menu Subsystem */


void M_Init(void)
{
	Cmd_AddCommand("togglemenu", M_ToggleMenu_f);

	MQH_Init();
}


void M_Draw(void)
{
	if (m_state == m_none || !(in_keyCatchers & KEYCATCH_UI))
	{
		return;
	}

	if (!m_recursiveDraw)
	{
		if (con.displayFrac)
		{
			Con_DrawFullBackground();
			S_ExtraUpdate();
		}
		else
		{
			Draw_FadeScreen();
		}
	}
	else
	{
		m_recursiveDraw = false;
	}

	MQH_Draw();
	switch (m_state)
	{
	case m_keys:
		M_Keys_Draw();
		break;

	case m_video:
		M_Video_Draw();
		break;

	case m_help:
		M_Help_Draw();
		break;

	case m_quit:
		M_Quit_Draw();
		break;
	}

	if (mqh_entersound)
	{
		S_StartLocalSound("misc/menu2.wav");
		mqh_entersound = false;
	}

	S_ExtraUpdate();
}


void M_Keydown(int key)
{
	switch (m_state)
	{

	case m_keys:
		M_Keys_Key(key);
		return;

	case m_video:
		M_Video_Key(key);
		return;

	case m_help:
		M_Help_Key(key);
		return;

	case m_quit:
		M_Quit_Key(key);
		return;
	}
	MQH_Keydown(key);
}
