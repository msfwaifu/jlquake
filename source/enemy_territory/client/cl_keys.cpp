/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "client.h"

/*

key up events are sent even if in console mode

*/

qboolean UI_checkKeyExec(int key);			// NERVE - SMF
qboolean CL_CGameCheckKeyExec(int key);

keyname_t keynames_d[] =	//deutsch
{
	{"TAB", K_TAB},
	{"EINGABETASTE", K_ENTER},
	{"ESC", K_ESCAPE},
	{"LEERTASTE", K_SPACE},
	{"R�CKTASTE", K_BACKSPACE},
	{"PFEILT.AUF", K_UPARROW},
	{"PFEILT.UNTEN", K_DOWNARROW},
	{"PFEILT.LINKS", K_LEFTARROW},
	{"PFEILT.RECHTS", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"STRG", K_CTRL},
	{"UMSCHALLT", K_SHIFT},

	{"FESTSTELLT", K_CAPSLOCK},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"EINFG", K_INS},
	{"ENTF", K_DEL},
	{"BILD-AB", K_PGDN},
	{"BILD-AUF", K_PGUP},
	{"POS1", K_HOME},
	{"ENDE", K_END},

	{"MAUS1", K_MOUSE1},
	{"MAUS2", K_MOUSE2},
	{"MAUS3", K_MOUSE3},
	{"MAUS4", K_MOUSE4},
	{"MAUS5", K_MOUSE5},

	{"MRADOBEN", K_MWHEELUP },
	{"MRADUNTEN",    K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},

	{"ZB_POS1",          K_KP_HOME },
	{"ZB_PFEILT.AUF",    K_KP_UPARROW },
	{"ZB_BILD-AUF",      K_KP_PGUP },
	{"ZB_PFEILT.LINKS",  K_KP_LEFTARROW },
	{"ZB_5",         K_KP_5 },
	{"ZB_PFEILT.RECHTS",K_KP_RIGHTARROW },
	{"ZB_ENDE",          K_KP_END },
	{"ZB_PFEILT.UNTEN",  K_KP_DOWNARROW },
	{"ZB_BILD-AB",       K_KP_PGDN },
	{"ZB_ENTER",     K_KP_ENTER },
	{"ZB_EINFG",     K_KP_INS },
	{"ZB_ENTF",          K_KP_DEL },
	{"ZB_SLASH",     K_KP_SLASH },
	{"ZB_MINUS",     K_KP_MINUS },
	{"ZB_PLUS",          K_KP_PLUS },
	{"ZB_NUM",           K_KP_NUMLOCK },
	{"ZB_*",         K_KP_STAR },
	{"ZB_EQUALS",        K_KP_EQUALS },

	{"PAUSE", K_PAUSE},

	{"COMMAND", K_COMMAND},	//mac
	{NULL,0}
};	//end german

keyname_t keynames_f[] =	//french
{
	{"TAB", K_TAB},
	{"ENTREE",   K_ENTER},
	{"ECHAP",    K_ESCAPE},
	{"ESPACE",   K_SPACE},
	{"RETOUR",   K_BACKSPACE},
	{"HAUT", K_UPARROW},
	{"BAS",      K_DOWNARROW},
	{"GAUCHE",   K_LEFTARROW},
	{"DROITE",   K_RIGHTARROW},

	{"ALT",      K_ALT},
	{"CTRL", K_CTRL},
	{"MAJ",      K_SHIFT},

	{"VERRMAJ", K_CAPSLOCK},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INSER", K_INS},
	{"SUPPR", K_DEL},
	{"PGBAS", K_PGDN},
	{"PGHAUT", K_PGUP},
	{"ORIGINE", K_HOME},
	{"FIN", K_END},

	{"SOURIS1", K_MOUSE1},
	{"SOURIS2", K_MOUSE2},
	{"SOURIS3", K_MOUSE3},
	{"SOURIS4", K_MOUSE4},
	{"SOURIS5", K_MOUSE5},

	{"MOLETTEHT.",   K_MWHEELUP },
	{"MOLETTEBAS",   K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},

	{"PN_ORIGINE",       K_KP_HOME },
	{"PN_HAUT",          K_KP_UPARROW },
	{"PN_PGBAS",     K_KP_PGUP },
	{"PN_GAUCHE",        K_KP_LEFTARROW },
	{"PN_5",         K_KP_5 },
	{"PN_DROITE",        K_KP_RIGHTARROW },
	{"PN_FIN",           K_KP_END },
	{"PN_BAS",           K_KP_DOWNARROW },
	{"PN_PGBAS",     K_KP_PGDN },
	{"PN_ENTR",          K_KP_ENTER },
	{"PN_INSER",     K_KP_INS },
	{"PN_SUPPR",     K_KP_DEL },
	{"PN_SLASH",     K_KP_SLASH },
	{"PN_MOINS",     K_KP_MINUS },
	{"PN_PLUS",          K_KP_PLUS },
	{"PN_VERRNUM",       K_KP_NUMLOCK },
	{"PN_*",         K_KP_STAR },
	{"PN_EQUALS",        K_KP_EQUALS },

	{"PAUSE", K_PAUSE},

	{"COMMAND", K_COMMAND},	//mac

	{NULL,0}
};	//end french

keyname_t keynames_s[] =	//Spanish
{
	{"TABULADOR", K_TAB},
	{"INTRO", K_ENTER},
	{"ESC", K_ESCAPE},
	{"BARRA_ESPACIAD", K_SPACE},
	{"RETROCESO", K_BACKSPACE},
	{"CURSOR_ARRIBA", K_UPARROW},
	{"CURSOR_ABAJO", K_DOWNARROW},
	{"CURSOR_IZQDA", K_LEFTARROW},
	{"CURSOR_DERECHA", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"MAY�S", K_SHIFT},

	{"BLOQ_MAY�S", K_CAPSLOCK},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INSERT", K_INS},
	{"SUPR", K_DEL},
	{"AV_P�G", K_PGDN},
	{"RE_P�G", K_PGUP},
	{"INICIO", K_HOME},
	{"FIN", K_END},

	{"RAT�N1", K_MOUSE1},
	{"RAT�N2", K_MOUSE2},
	{"RAT�N3", K_MOUSE3},
	{"RAT�N4", K_MOUSE4},
	{"RAT�N5", K_MOUSE5},

	{"RUEDA_HACIA_ARRIBA",   K_MWHEELUP },
	{"RUEDA_HACIA_ABAJO",    K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},

	{"INICIO(NUM)",          K_KP_HOME },
	{"ARRIBA(NUM)",      K_KP_UPARROW },
	{"RE_P�G(NUM)",          K_KP_PGUP },
	{"IZQUIERDA(NUM)",   K_KP_LEFTARROW },
	{"5(NUM)",           K_KP_5 },
	{"DERECHA(NUM)", K_KP_RIGHTARROW },
	{"FIN(NUM)",         K_KP_END },
	{"ABAJO(NUM)",   K_KP_DOWNARROW },
	{"AV_P�G(NUM)",          K_KP_PGDN },
	{"INTRO(NUM)",       K_KP_ENTER },
	{"INS(NUM)",         K_KP_INS },
	{"SUPR(NUM)",            K_KP_DEL },
	{"/(NUM)",       K_KP_SLASH },
	{"-(NUM)",       K_KP_MINUS },
	{"+(NUM)",           K_KP_PLUS },
	{"BLOQ_NUM",     K_KP_NUMLOCK },
	{"*(NUM)",           K_KP_STAR },
	{"INTRO(NUM)",       K_KP_EQUALS },

	{"PAUSA", K_PAUSE},

	{"PUNTO_Y_COMA", ';'},		// because a raw semicolon seperates commands

	{"COMANDO", K_COMMAND},	//mac

	{NULL,0}
};

keyname_t keynames_i[] =	//Italian
{
	{"TAB", K_TAB},
	{"INVIO", K_ENTER},
	{"ESC", K_ESCAPE},
	{"SPAZIO", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},
	{"FRECCIASU", K_UPARROW},
	{"FRECCIAGI�", K_DOWNARROW},
	{"FRECCIASX", K_LEFTARROW},
	{"FRECCIADX", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"MAIUSC", K_SHIFT},

	{"BLOCMAIUSC", K_CAPSLOCK},

	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},

	{"INS", K_INS},
	{"CANC", K_DEL},
	{"PAGGI�", K_PGDN},
	{"PAGGSU", K_PGUP},
	{"HOME", K_HOME},
	{"FINE", K_END},

	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},

	{"ROTELLASU",    K_MWHEELUP },
	{"ROTELLAGI�",   K_MWHEELDOWN },

	{"JOY1", K_JOY1},
	{"JOY2", K_JOY2},
	{"JOY3", K_JOY3},
	{"JOY4", K_JOY4},
	{"JOY5", K_JOY5},
	{"JOY6", K_JOY6},
	{"JOY7", K_JOY7},
	{"JOY8", K_JOY8},
	{"JOY9", K_JOY9},
	{"JOY10", K_JOY10},
	{"JOY11", K_JOY11},
	{"JOY12", K_JOY12},
	{"JOY13", K_JOY13},
	{"JOY14", K_JOY14},
	{"JOY15", K_JOY15},
	{"JOY16", K_JOY16},
	{"JOY17", K_JOY17},
	{"JOY18", K_JOY18},
	{"JOY19", K_JOY19},
	{"JOY20", K_JOY20},
	{"JOY21", K_JOY21},
	{"JOY22", K_JOY22},
	{"JOY23", K_JOY23},
	{"JOY24", K_JOY24},
	{"JOY25", K_JOY25},
	{"JOY26", K_JOY26},
	{"JOY27", K_JOY27},
	{"JOY28", K_JOY28},
	{"JOY29", K_JOY29},
	{"JOY30", K_JOY30},
	{"JOY31", K_JOY31},
	{"JOY32", K_JOY32},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},

	{"TN_HOME",          K_KP_HOME },
	{"TN_FRECCIASU",     K_KP_UPARROW },
	{"TN_PAGGSU",            K_KP_PGUP },
	{"TN_FRECCIASX", K_KP_LEFTARROW },
	{"TN_5",         K_KP_5 },
	{"TN_FRECCIA_DX",    K_KP_RIGHTARROW },
	{"TN_FINE",          K_KP_END },
	{"TN_FRECCIAGI�",    K_KP_DOWNARROW },
	{"TN_PAGGI�",            K_KP_PGDN },
	{"TN_INVIO",     K_KP_ENTER },
	{"TN_INS",           K_KP_INS },
	{"TN_CANC",          K_KP_DEL },
	{"TN_/",     K_KP_SLASH },
	{"TN_-",     K_KP_MINUS },
	{"TN_+",         K_KP_PLUS },
	{"TN_BLOCNUM",       K_KP_NUMLOCK },
	{"TN_*",         K_KP_STAR },
	{"TN_=",     K_KP_EQUALS },

	{"PAUSA", K_PAUSE},

	{"�", ';'},		// because a raw semicolon seperates commands

	{"COMMAND", K_COMMAND},	//mac

	{NULL,0}
};

//============================================================================


/*
================
Message_Key

In game talk message
================
*/
void Message_Key(int key)
{

	char buffer[MAX_STRING_CHARS];


	if (key == K_ENTER || key == K_KP_ENTER)
	{
		if (chatField.buffer[0] && cls.state == CA_ACTIVE)
		{
			if (chat_team)
			{
				String::Sprintf(buffer, sizeof(buffer), "say_team \"%s\"\n", chatField.buffer);
			}
			else if (chat_buddy)
			{
				String::Sprintf(buffer, sizeof(buffer), "say_buddy \"%s\"\n", chatField.buffer);
			}
			else
			{
				String::Sprintf(buffer, sizeof(buffer), "say \"%s\"\n", chatField.buffer);
			}

			CL_AddReliableCommand(buffer);
		}
		in_keyCatchers &= ~KEYCATCH_MESSAGE;
		Field_Clear(&chatField);
		return;
	}

	Con_MessageKeyEvent(key);
}

//============================================================================


qboolean Key_GetOverstrikeMode(void)
{
	return key_overstrikeMode;
}


void Key_SetOverstrikeMode(qboolean state)
{
	key_overstrikeMode = state;
}


/*
===================
Key_IsDown
===================
*/
qboolean Key_IsDown(int keynum)
{
	if (keynum == -1)
	{
		return qfalse;
	}

	return keys[keynum].down;
}


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers

to be configured even if they don't have defined names.
===================
*/
int Key_StringToKeynum(char* str)
{
	keyname_t* kn;

	if (!str || !str[0])
	{
		return -1;
	}
	if (!str[1])
	{
		// Always lowercase
		String::ToLower(str);
		return str[0];
	}

	// check for hex code
	if (str[0] == '0' && str[1] == 'x' && String::Length(str) == 4)
	{
		int n1, n2;

		n1 = str[2];
		if (n1 >= '0' && n1 <= '9')
		{
			n1 -= '0';
		}
		else if (n1 >= 'a' && n1 <= 'f')
		{
			n1 = n1 - 'a' + 10;
		}
		else
		{
			n1 = 0;
		}

		n2 = str[3];
		if (n2 >= '0' && n2 <= '9')
		{
			n2 -= '0';
		}
		else if (n2 >= 'a' && n2 <= 'f')
		{
			n2 = n2 - 'a' + 10;
		}
		else
		{
			n2 = 0;
		}

		return n1 * 16 + n2;
	}

	// scan for a text match
	for (kn = keynames; kn->name; kn++)
	{
		if (!String::ICmp(str,kn->name))
		{
			return kn->keynum;
		}
	}

	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
const char* Key_KeynumToString(int keynum, qboolean bTranslate)
{
	keyname_t* kn;
	static char tinystr[5];
	int i, j;

	if (keynum == -1)
	{
		return "<KEY NOT FOUND>";
	}

	if (keynum < 0 || keynum > 255)
	{
		return "<OUT OF RANGE>";
	}

	// check for printable ascii (don't use quote)
	if (keynum > 32 && keynum < 127 && keynum != '"')
	{
		tinystr[0] = keynum;
		tinystr[1] = 0;
		if (keynum == ';' && !bTranslate)
		{
			//fall through and use keyname table
		}
		else
		{
			return tinystr;
		}
	}


	kn = keynames;		//init to english
#ifndef __MACOS__	//DAJ USA
	if (bTranslate)
	{
		if (cl_language->integer - 1 == LANGUAGE_FRENCH)
		{
			kn = keynames_f;	//use french
		}
		else if (cl_language->integer - 1 == LANGUAGE_GERMAN)
		{
			kn = keynames_d;	//use german
		}
		else if (cl_language->integer - 1 == LANGUAGE_ITALIAN)
		{
			kn = keynames_i;	//use italian
		}
		else if (cl_language->integer - 1 == LANGUAGE_SPANISH)
		{
			kn = keynames_s;	//use spanish
		}
	}
#endif

	// check for a key string
	for (; kn->name; kn++)
	{
		if (keynum == kn->keynum)
		{
			return kn->name;
		}
	}

	// make a hex string
	i = keynum >> 4;
	j = keynum & 15;

	tinystr[0] = '0';
	tinystr[1] = 'x';
	tinystr[2] = i > 9 ? i - 10 + 'a' : i + '0';
	tinystr[3] = j > 9 ? j - 10 + 'a' : j + '0';
	tinystr[4] = 0;

	return tinystr;
}

#define BIND_HASH_SIZE 1024

static long generateHashValue(const char* fname)
{
	int i;
	long hash;

	if (!fname)
	{
		return 0;
	}

	hash = 0;
	i = 0;
	while (fname[i] != '\0')
	{
		hash += (long)(fname[i]) * (i + 119);
		i++;
	}
	hash &= (BIND_HASH_SIZE - 1);
	return hash;
}

/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding(int keynum, const char* binding)
{

	char* lcbinding;	// fretn - make a copy of our binding lowercase
						// so name toggle scripts work again: bind x name BzZIfretn?
						// resulted into bzzifretn?

	if (keynum == -1)
	{
		return;
	}

	// free old bindings
	if (keys[keynum].binding)
	{
		Z_Free(keys[keynum].binding);
	}

	// allocate memory for new binding
	keys[keynum].binding = CopyString(binding);
	lcbinding = CopyString(binding);
	String::ToLower(lcbinding);		// saves doing it on all the generateHashValues in Key_GetBindingByString

	keys[keynum].hash = generateHashValue(lcbinding);

	// consider this like modifying an archived cvar, so the
	// file write will be triggered at the next oportunity
	cvar_modifiedFlags |= CVAR_ARCHIVE;
}


/*
===================
Key_GetBinding
===================
*/
const char* Key_GetBinding(int keynum)
{
	if (keynum == -1)
	{
		return "";
	}

	return keys[keynum].binding;
}

// binding MUST be lower case
void Key_GetBindingByString(const char* binding, int* key1, int* key2)
{
	int i;
	int hash = generateHashValue(binding);

	*key1 = -1;
	*key2 = -1;

	for (i = 0; i < MAX_KEYS; i++)
	{
		if (keys[i].hash == hash && !String::ICmp(binding, keys[i].binding))
		{
			if (*key1 == -1)
			{
				*key1 = i;
			}
			else if (*key2 == -1)
			{
				*key2 = i;
				return;
			}
		}
	}
}

/*
===================
Key_GetKey
===================
*/

int Key_GetKey(const char* binding)
{
	int i;

	if (binding)
	{
		for (i = 0; i < 256; i++)
		{
			if (keys[i].binding && String::ICmp(binding, keys[i].binding) == 0)
			{
				return i;
			}
		}
	}
	return -1;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f(void)
{
	int b;

	if (Cmd_Argc() != 2)
	{
		Com_Printf("unbind <key> : remove commands from a key\n");
		return;
	}

	b = Key_StringToKeynum(Cmd_Argv(1));
	if (b == -1)
	{
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding(b, "");
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f(void)
{
	int i;

	for (i = 0; i < 256; i++)
		if (keys[i].binding)
		{
			Key_SetBinding(i, "");
		}
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f(void)
{
	int i, c, b;
	char cmd[1024];

	c = Cmd_Argc();

	if (c < 2)
	{
		Com_Printf("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum(Cmd_Argv(1));
	if (b == -1)
	{
		Com_Printf("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keys[b].binding)
		{
			Com_Printf("\"%s\" = \"%s\"\n", Cmd_Argv(1), keys[b].binding);
		}
		else
		{
			Com_Printf("\"%s\" is not bound\n", Cmd_Argv(1));
		}
		return;
	}

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i = 2; i < c; i++)
	{
		strcat(cmd, Cmd_Argv(i));
		if (i != (c - 1))
		{
			strcat(cmd, " ");
		}
	}

	Key_SetBinding(b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings(fileHandle_t f)
{
	int i;

	FS_Printf(f, "unbindall\n");

	for (i = 0; i < 256; i++)
	{
		if (keys[i].binding && keys[i].binding[0])
		{
			FS_Printf(f, "bind %s \"%s\"\n", Key_KeynumToString(i, qfalse), keys[i].binding);

		}

	}
}


/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f(void)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		if (keys[i].binding && keys[i].binding[0])
		{
			Com_Printf("%s \"%s\"\n", Key_KeynumToString(i, qfalse), keys[i].binding);
		}
	}
}


/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands(void)
{
	// register our functions
	Cmd_AddCommand("bind",Key_Bind_f);
	Cmd_AddCommand("unbind",Key_Unbind_f);
	Cmd_AddCommand("unbindall",Key_Unbindall_f);
	Cmd_AddCommand("bindlist",Key_Bindlist_f);
}

/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
//static consoleCount = 0;
// fretn
qboolean consoleButtonWasPressed = qfalse;

void CL_KeyEvent(int key, qboolean down, unsigned time)
{
	char* kb;
	char cmd[1024];
	qboolean bypassMenu = qfalse;		// NERVE - SMF
	qboolean onlybinds = qfalse;

	if (!key)
	{
		return;
	}

	switch (key)
	{
	case K_KP_PGUP:
	case K_KP_EQUALS:
	case K_KP_5:
	case K_KP_LEFTARROW:
	case K_KP_UPARROW:
	case K_KP_RIGHTARROW:
	case K_KP_DOWNARROW:
	case K_KP_END:
	case K_KP_PGDN:
	case K_KP_INS:
	case K_KP_DEL:
	case K_KP_HOME:
		if (Sys_IsNumLockDown())
		{
			onlybinds = qtrue;
		}
		break;
	}

	// update auto-repeat status and WOLFBUTTON_ANY status
	keys[key].down = down;

	if (down)
	{
		keys[key].repeats++;
		if (keys[key].repeats == 1)
		{
			anykeydown++;
		}
	}
	else
	{
		keys[key].repeats = 0;
		anykeydown--;
		if (anykeydown < 0)
		{
			anykeydown = 0;
		}
	}

#ifdef __linux__
	if (key == K_ENTER)
	{
		if (down)
		{
			if (keys[K_ALT].down)
			{
				Key_ClearStates();
				if (Cvar_VariableValue("r_fullscreen") == 0)
				{
					Com_Printf("Switching to fullscreen rendering\n");
					Cvar_Set("r_fullscreen", "1");
				}
				else
				{
					Com_Printf("Switching to windowed rendering\n");
					Cvar_Set("r_fullscreen", "0");
				}
				Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
				return;
			}
		}
	}
#endif

	// console key is hardcoded, so the user can never unbind it
	if (key == '`' || key == '~')
	{
		if (!down)
		{
			return;
		}
		Con_ToggleConsole_f();

		// the console key should never be used as a char
		consoleButtonWasPressed = qtrue;
		return;
	}
	else
	{
		consoleButtonWasPressed = qfalse;
	}

//----(SA)	added
	if (cl.wa_cameraMode)
	{
		if (!(in_keyCatchers & (KEYCATCH_UI | KEYCATCH_CONSOLE)))			// let menu/console handle keys if necessary

		{	// in cutscenes we need to handle keys specially (pausing not allowed in camera mode)
			if ((key == K_ESCAPE ||
				 key == K_SPACE ||
				 key == K_ENTER) && down)
			{
				CL_AddReliableCommand("cameraInterrupt");
				return;
			}

			// eat all keys
			if (down)
			{
				return;
			}
		}

		if ((in_keyCatchers & KEYCATCH_CONSOLE) && key == K_ESCAPE)
		{
			// don't allow menu starting when console is down and camera running
			return;
		}
	}
	//----(SA)	end

	// most keys during demo playback will bring up the menu, but non-ascii

	// keys can still be used for bound actions
	if (down && (key < 128 || key == K_MOUSE1) &&
		(clc.demoplaying || cls.state == CA_CINEMATIC) && !in_keyCatchers)
	{

		Cvar_Set("nextdemo","");
		key = K_ESCAPE;
	}

	// escape is always handled special
	if (key == K_ESCAPE && down)
	{
		if (in_keyCatchers & KEYCATCH_MESSAGE)
		{
			// clear message mode
			Message_Key(key);
			return;
		}

		// escape always gets out of CGAME stuff
		if (in_keyCatchers & KEYCATCH_CGAME)
		{
			in_keyCatchers &= ~KEYCATCH_CGAME;
			VM_Call(cgvm, CG_EVENT_HANDLING, CGAME_EVENT_NONE);
			return;
		}

		if (!(in_keyCatchers & KEYCATCH_UI))
		{
			if (cls.state == CA_ACTIVE && !clc.demoplaying)
			{
				// Arnout: on request
				if (in_keyCatchers & KEYCATCH_CONSOLE)		// get rid of the console
				{
					Con_ToggleConsole_f();
				}
				else
				{
					VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_INGAME);
				}
			}
			else
			{
				CL_Disconnect_f();
				S_StopAllSounds();
				VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
			}
			return;
		}

		VM_Call(uivm, UI_KEY_EVENT, key, down);
		return;
	}

	//
	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing
	// an action started before a mode switch.
	//
	if (!down)
	{
		kb = keys[key].binding;
		if (kb && kb[0] == '+')
		{
			// button commands add keynum and time as parms so that multiple
			// sources can be discriminated and subframe corrected
			String::Sprintf(cmd, sizeof(cmd), "-%s %i %i\n", kb + 1, key, time);
			Cbuf_AddText(cmd);
		}

		if (in_keyCatchers & KEYCATCH_UI && uivm)
		{
			if (!onlybinds || VM_Call(uivm, UI_WANTSBINDKEYS))
			{
				VM_Call(uivm, UI_KEY_EVENT, key, down);
			}
		}
		else if (in_keyCatchers & KEYCATCH_CGAME && cgvm)
		{
			if (!onlybinds || VM_Call(cgvm, CG_WANTSBINDKEYS))
			{
				VM_Call(cgvm, CG_KEY_EVENT, key, down);
			}
		}

		return;
	}

	// NERVE - SMF - if we just want to pass it along to game
	if (cl_bypassMouseInput && cl_bypassMouseInput->integer)
	{
		if ((key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 || key == K_MOUSE4 || key == K_MOUSE5))
		{
			if (cl_bypassMouseInput->integer == 1)
			{
				bypassMenu = qtrue;
			}
		}
		else if ((in_keyCatchers & KEYCATCH_UI && !UI_checkKeyExec(key)) || (in_keyCatchers & KEYCATCH_CGAME && !CL_CGameCheckKeyExec(key)))
		{
			bypassMenu = qtrue;
		}
	}

	// distribute the key down event to the apropriate handler
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		if (!onlybinds)
		{
			Con_KeyEvent(key);
		}
	}
	else if (in_keyCatchers & KEYCATCH_UI && !bypassMenu)
	{
		if (!onlybinds || VM_Call(uivm, UI_WANTSBINDKEYS))
		{
			VM_Call(uivm, UI_KEY_EVENT, key, down);
		}
	}
	else if (in_keyCatchers & KEYCATCH_CGAME && !bypassMenu)
	{
		if (cgvm)
		{
			if (!onlybinds || VM_Call(cgvm, CG_WANTSBINDKEYS))
			{
				VM_Call(cgvm, CG_KEY_EVENT, key, down);
			}
		}
	}
	else if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		if (!onlybinds)
		{
			Message_Key(key);
		}
	}
	else if (cls.state == CA_DISCONNECTED)
	{

		if (!onlybinds)
		{
			Con_KeyEvent(key);
		}

	}
	else
	{
		// send the bound action
		kb = keys[key].binding;
		if (!kb)
		{
			if (key >= 200)
			{
				Com_Printf("%s is unbound, use controls menu to set.\n",
					Key_KeynumToString(key, qfalse));
			}
		}
		else if (kb[0] == '+')
		{
			// button commands add keynum and time as parms so that multiple
			// sources can be discriminated and subframe corrected
			String::Sprintf(cmd, sizeof(cmd), "%s %i %i\n", kb, key, time);
			Cbuf_AddText(cmd);
		}
		else
		{
			// down-only command
			Cbuf_AddText(kb);
			Cbuf_AddText("\n");
		}
	}
}


/*
===================
CL_CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void CL_CharEvent(int key)
{
	// the console key should never be used as a char
	// ydnar: added uk equivalent of shift+`
	// the RIGHT way to do this would be to have certain keys disable the equivalent SE_CHAR event

	// fretn - this should be fixed in Com_EventLoop
	// but I can't be arsed to leave this as is

	if (key == (unsigned char)'`' || key == (unsigned char)'~' || key == (unsigned char)'�')
	{
		return;
	}

	// distribute the key down event to the apropriate handler
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		Field_CharEvent(&g_consoleField, key);
	}
	else if (in_keyCatchers & KEYCATCH_UI)
	{
		VM_Call(uivm, UI_KEY_EVENT, key | K_CHAR_FLAG, qtrue);
	}
	else if (in_keyCatchers & KEYCATCH_CGAME)
	{
		VM_Call(cgvm, CG_KEY_EVENT, key | K_CHAR_FLAG, qtrue);
	}
	else if (in_keyCatchers & KEYCATCH_MESSAGE)
	{
		Field_CharEvent(&chatField, key);
	}
	else if (cls.state == CA_DISCONNECTED)
	{
		Field_CharEvent(&g_consoleField, key);
	}
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates(void)
{
	int i;

	anykeydown = qfalse;

	for (i = 0; i < MAX_KEYS; i++)
	{
		if (keys[i].down)
		{
			CL_KeyEvent(i, qfalse, 0);

		}
		keys[i].down = 0;
		keys[i].repeats = 0;
	}
}
