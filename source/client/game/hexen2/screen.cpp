//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../client.h"
#include "local.h"
#include "../quake_hexen2/menu.h"
#include "../../../common/hexen2strings.h"

#define PLAQUE_WIDTH 26

int clh2_total_loading_size, clh2_current_loading_size, clh2_loading_stage;

char scrh2_infomessage[MAX_INFO_H2];

const char* clh2_plaquemessage = NULL;	// Pointer to current plaque message

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

#define MAXLINES_H2 27

static int scrh2_lines;
static int scrh2_StartC[MAXLINES_H2], scrh2_EndC[MAXLINES_H2];

static void SCRH2_FindTextBreaks(const char* message, int Width)
{
	scrh2_lines = 0;
	int pos = 0;
	int start = 0;
	int lastspace = -1;

	while (1)
	{
		if (pos - start >= Width || message[pos] == '@' ||
			message[pos] == 0)
		{
			int oldlast = lastspace;
			if (message[pos] == '@' || lastspace == -1 || message[pos] == 0)
			{
				lastspace = pos;
			}

			scrh2_StartC[scrh2_lines] = start;
			scrh2_EndC[scrh2_lines] = lastspace;
			scrh2_lines++;
			if (scrh2_lines == MAXLINES_H2)
			{
				return;
			}

			if (message[pos] == '@')
			{
				start = pos + 1;
			}
			else if (oldlast == -1)
			{
				start = lastspace;
			}
			else
			{
				start = lastspace + 1;
			}

			lastspace = -1;
		}

		if (message[pos] == 32)
		{
			lastspace = pos;
		}
		else if (message[pos] == 0)
		{
			break;
		}

		pos++;
	}
}

static void MH2_Print2(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy + ((viddef.height - 200) >> 1), str, 256);
}

static void SCRH2_Bottom_Plaque_Draw(const char* message)
{
	if (!*message)
	{
		return;
	}

	SCRH2_FindTextBreaks(message, PLAQUE_WIDTH);

	int by = (((viddef.height) / 8) - scrh2_lines - 2) * 8;

	MQH_DrawTextBox(32, by - 16, 30, scrh2_lines + 2);

	for (int i = 0; i < scrh2_lines; i++, by += 8)
	{
		char temp[80];
		String::NCpy(temp, &message[scrh2_StartC[i]],scrh2_EndC[i] - scrh2_StartC[i]);
		temp[scrh2_EndC[i] - scrh2_StartC[i]] = 0;
		int bx = ((40 - String::Length(temp)) * 8) / 2;
		MQH_Print(bx, by, temp);
	}
}

void SCRH2_DrawCenterString(const char* message)
{
	if (h2intro_playing)
	{
		SCRH2_Bottom_Plaque_Draw(message);
		return;
	}

	SCRH2_FindTextBreaks(message, 38);

	int by = ((25 - scrh2_lines) * 8) / 2;
	for (int i = 0; i < scrh2_lines; i++, by += 8)
	{
		char temp[80];
		String::NCpy(temp,&message[scrh2_StartC[i]],scrh2_EndC[i] - scrh2_StartC[i]);
		temp[scrh2_EndC[i] - scrh2_StartC[i]] = 0;
		int bx = ((40 - String::Length(temp)) * 8) / 2;
		MH2_Print2(bx, by, temp);
	}
}

void SCRH2_DrawPause()
{
	image_t* pic;
	float delta;
	static qboolean newdraw = false;
	int finaly;
	static float LogoPercent,LogoTargetPercent;

	if (!scr_showpause->value)		// turn off for screenshots
	{
		return;
	}

	if (!cl.qh_paused)
	{
		newdraw = false;
		return;
	}

	if (!newdraw)
	{
		newdraw = true;
		LogoTargetPercent = 1;
		LogoPercent = 0;
	}

	pic = R_CachePic("gfx/menu/paused.lmp");

	if (LogoPercent < LogoTargetPercent)
	{
		delta = ((LogoTargetPercent - LogoPercent) / .5) * cls.frametime * 0.001;
		if (delta < 0.004)
		{
			delta = 0.004;
		}
		LogoPercent += delta;
		if (LogoPercent > LogoTargetPercent)
		{
			LogoPercent = LogoTargetPercent;
		}
	}

	finaly = ((float)R_GetImageHeight(pic) * LogoPercent) - R_GetImageHeight(pic);
	UI_DrawPic((viddef.width - R_GetImageWidth(pic)) / 2, finaly, pic);
}

void SCRH2_DrawLoading()
{
	int size, count, offset;
	image_t* pic;

	if (!scr_draw_loading && clh2_loading_stage == 0)
	{
		return;
	}

	pic = R_CachePic("gfx/menu/loading.lmp");
	offset = (viddef.width - R_GetImageWidth(pic)) / 2;
	UI_DrawPic(offset, 0, pic);

	if (clh2_loading_stage == 0)
	{
		return;
	}

	if (clh2_total_loading_size)
	{
		size = clh2_current_loading_size * 106 / clh2_total_loading_size;
	}
	else
	{
		size = 0;
	}

	if (clh2_loading_stage == 1)
	{
		count = size;
	}
	else
	{
		count = 106;
	}

	UI_FillPal(offset + 42, 87, count, 1, 136);
	UI_FillPal(offset + 42, 87 + 1, count, 4, 138);
	UI_FillPal(offset + 42, 87 + 5, count, 1, 136);

	if (clh2_loading_stage == 2)
	{
		count = size;
	}
	else
	{
		count = 0;
	}

	UI_FillPal(offset + 42, 97, count, 1, 168);
	UI_FillPal(offset + 42, 97 + 1, count, 4, 170);
	UI_FillPal(offset + 42, 97 + 5, count, 1, 168);
}

void SCRH2_UpdateInfoMessage()
{
	String::Cpy(scrh2_infomessage, "Objectives:");

	if (!prh2_global_info_strings)
	{
		return;
	}

	for (unsigned int i = 0; i < 32; i++)
	{
		unsigned int check = (1 << i);

		if (cl.h2_info_mask & check)
		{
			char* newmessage = &prh2_global_info_strings[prh2_info_string_index[i]];
			String::Cat(scrh2_infomessage, sizeof(scrh2_infomessage), "@@");
			String::Cat(scrh2_infomessage, sizeof(scrh2_infomessage), newmessage);
		}
	}

	for (unsigned int i = 0; i < 32; i++)
	{
		unsigned int check = (1 << i);

		if (cl.h2_info_mask2 & check)
		{
			char* newmessage = &prh2_global_info_strings[prh2_info_string_index[i + 32]];
			String::Cat(scrh2_infomessage, sizeof(scrh2_infomessage), "@@");
			String::Cat(scrh2_infomessage, sizeof(scrh2_infomessage), newmessage);
		}
	}
}

void SCRH2_Plaque_Draw(const char* message, bool AlwaysDraw)
{
	int i;
	char temp[80];
	int bx,by;

	if (con.displayFrac == 1 && !AlwaysDraw)
	{
		return;		// console is full screen

	}
	if (!*message)
	{
		return;
	}

	SCRH2_FindTextBreaks(message, PLAQUE_WIDTH);

	by = ((25 - scrh2_lines) * 8) / 2;
	MQH_DrawTextBox2(32, by - 16, 30, scrh2_lines + 2);

	for (i = 0; i < scrh2_lines; i++,by += 8)
	{
		String::NCpy(temp,&message[scrh2_StartC[i]],scrh2_EndC[i] - scrh2_StartC[i]);
		temp[scrh2_EndC[i] - scrh2_StartC[i]] = 0;
		bx = ((40 - String::Length(temp)) * 8) / 2;
		MH2_Print2(bx, by, temp);
	}
}

void SCRH2_Info_Plaque_Draw(const char* message)
{
	if (con.displayFrac == 1)
	{
		return;		// console is full screen

	}
	if (!*message)
	{
		return;
	}

	SCRH2_FindTextBreaks(message, PLAQUE_WIDTH + 4);

	if (scrh2_lines == MAXLINES_H2)
	{
		common->DPrintf("SCRH2_Info_Plaque_Draw: line overflow error\n");
		scrh2_lines = MAXLINES_H2 - 1;
	}

	int by = ((25 - scrh2_lines) * 8) / 2;
	MQH_DrawTextBox2(15, by - 16, PLAQUE_WIDTH + 4 + 4, scrh2_lines + 2);

	for (int i = 0; i < scrh2_lines; i++,by += 8)
	{
		char temp[80];
		String::NCpy(temp, &message[scrh2_StartC[i]], scrh2_EndC[i] - scrh2_StartC[i]);
		temp[scrh2_EndC[i] - scrh2_StartC[i]] = 0;
		int bx = ((40 - String::Length(temp)) * 8) / 2;
		MH2_Print2(bx, by, temp);
	}
}

static void I_Print(int cx, int cy, char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy + ((viddef.height - 200) >> 1), str, 256);
}

void SBH2_IntermissionOverlay()
{
	image_t* pic;
	int elapsed, size, bx, by, i;
	const char* message;
	char temp[80];

	if (cl.qh_gametype == QHGAME_DEATHMATCH)
	{
		SbarH2_DeathmatchOverlay();
		return;
	}

	switch (cl.qh_intermission)
	{
	case 1:
		pic = R_CachePic("gfx/meso.lmp");
		break;
	case 2:
		pic = R_CachePic("gfx/egypt.lmp");
		break;
	case 3:
		pic = R_CachePic("gfx/roman.lmp");
		break;
	case 4:
		pic = R_CachePic("gfx/castle.lmp");
		break;
	case 5:
		pic = R_CachePic("gfx/castle.lmp");
		break;
	case 6:
		pic = R_CachePic("gfx/end-1.lmp");
		break;
	case 7:
		pic = R_CachePic("gfx/end-2.lmp");
		break;
	case 8:
		pic = R_CachePic("gfx/end-3.lmp");
		break;
	case 9:
		pic = R_CachePic("gfx/castle.lmp");
		break;
	case 10:
		pic = R_CachePic("gfx/mpend.lmp");
		break;
	case 11:
		pic = R_CachePic("gfx/mpmid.lmp");
		break;
	case 12:
		pic = R_CachePic("gfx/end-3.lmp");
		break;

	default:
		common->FatalError("SBH2_IntermissionOverlay: Bad episode");
		break;
	}

	UI_DrawPic(((viddef.width - 320) >> 1),((viddef.height - 200) >> 1), pic);

	if (cl.qh_intermission >= 6 && cl.qh_intermission <= 8)
	{
		elapsed = (cl.qh_serverTimeFloat - cl.qh_completed_time) * 20;
		elapsed -= 50;
		if (elapsed < 0)
		{
			elapsed = 0;
		}
	}
	else if (cl.qh_intermission == 12)
	{
		elapsed = (h2_introTime);
		if (h2_introTime < 500)
		{
			h2_introTime += 0.25;
		}
	}
	else
	{
		elapsed = (cl.qh_serverTimeFloat - cl.qh_completed_time) * 20;
	}

	if (cl.qh_intermission <= 4 && cl.qh_intermission + 394 <= prh2_string_count)
	{
		message = &prh2_global_strings[prh2_string_index[cl.qh_intermission + 394]];
	}
	else if (cl.qh_intermission == 5)
	{
		message = &prh2_global_strings[prh2_string_index[ABILITIES_STR_INDEX + NUM_CLASSES_H2 * 2 + 1]];
	}
	else if (cl.qh_intermission >= 6 && cl.qh_intermission <= 8 && cl.qh_intermission + 386 <= prh2_string_count)
	{
		message = &prh2_global_strings[prh2_string_index[cl.qh_intermission + 386]];
	}
	else if (cl.qh_intermission == 9)
	{
		message = &prh2_global_strings[prh2_string_index[391]];
	}
	else
	{
		message = "";
	}

	if (cl.qh_intermission == 10)
	{
		message = &prh2_global_strings[prh2_string_index[538]];
	}
	else if (cl.qh_intermission == 11)
	{
		message = &prh2_global_strings[prh2_string_index[545]];
	}
	else if (cl.qh_intermission == 12)
	{
		message = &prh2_global_strings[prh2_string_index[561]];
	}

	SCRH2_FindTextBreaks(message, 38);

	if (cl.qh_intermission == 8)
	{
		by = 16;
	}
	else
	{
		by = ((25 - scrh2_lines) * 8) / 2;
	}

	for (i = 0; i < scrh2_lines; i++,by += 8)
	{
		size = scrh2_EndC[i] - scrh2_StartC[i];
		String::NCpy(temp,&message[scrh2_StartC[i]],size);

		if (size > elapsed)
		{
			size = elapsed;
		}
		temp[size] = 0;

		bx = ((40 - String::Length(temp)) * 8) / 2;
		if (cl.qh_intermission < 6 || cl.qh_intermission > 9)
		{
			I_Print(bx, by, temp);
		}
		else
		{
			MQH_PrintWhite(bx, by, temp);
		}

		elapsed -= size;
		if (elapsed <= 0)
		{
			break;
		}
	}

	if (i == scrh2_lines && elapsed >= 300 && cl.qh_intermission >= 6 && cl.qh_intermission <= 7)
	{
		cl.qh_intermission++;
		cl.qh_completed_time = cl.qh_serverTimeFloat;
	}
}