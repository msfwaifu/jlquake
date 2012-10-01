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

#define CDKEY_LEN 16
#define CDCHKSUM_LEN 2

static char clt3_cdkey[34] = "                                ";

bool CLT3_CDKeyValidate(const char* key, const char* checksum)
{
	int len = String::Length(key);
	if (len != CDKEY_LEN)
	{
		return false;
	}

	if (checksum && String::Length(checksum) != CDCHKSUM_LEN)
	{
		return false;
	}

	byte sum = 0;
	// for loop gets rid of conditional assignment warning
	for (int i = 0; i < len; i++)
	{
		char ch = *key++;
		if (ch >= 'a' && ch <= 'z')
		{
			ch -= 32;
		}
		switch (ch)
		{
		case '2':
		case '3':
		case '7':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'G':
		case 'H':
		case 'J':
		case 'L':
		case 'P':
		case 'R':
		case 'S':
		case 'T':
		case 'W':
			if (GGameType & (GAME_WolfMP | GAME_ET))
			{
				sum = (sum << 1) ^ ch;
			}
			else
			{
				sum += ch;
			}
			continue;
		default:
			return false;
		}
	}

	char chs[3];
	sprintf(chs, "%02x", sum);

	if (checksum && !String::ICmp(chs, checksum))
	{
		return true;
	}

	if (!checksum)
	{
		return true;
	}

	return false;
}

static void MakeCDKeyFileName(const char* gameName, char* fbuffer)
{
	sprintf(fbuffer, "%s/%s", gameName, GGameType & GAME_Quake3 ? "q3key" : "rtcwkey");
}

void CLT3_ReadCDKey(const char* gameName)
{
	char fbuffer[MAX_OSPATH];
	MakeCDKeyFileName(gameName, fbuffer);

	fileHandle_t f;
	FS_SV_FOpenFileRead(fbuffer, &f);
	if (!f)
	{
		String::NCpyZ(clt3_cdkey, "                ", 17);
		return;
	}

	char buffer[33];
	Com_Memset(buffer, 0, sizeof(buffer));

	FS_Read(buffer, 16, f);
	FS_FCloseFile(f);

	if (CLT3_CDKeyValidate(buffer, NULL))
	{
		String::NCpyZ(clt3_cdkey, buffer, 17);
	}
	else
	{
		String::NCpyZ(clt3_cdkey, "                ", 17);
	}
}

void CLT3_AppendCDKey(const char* gameName)
{
	char fbuffer[MAX_OSPATH];
	MakeCDKeyFileName(gameName, fbuffer);

	fileHandle_t f;
	FS_SV_FOpenFileRead(fbuffer, &f);
	if (!f)
	{
		String::NCpyZ(&clt3_cdkey[16], "                ", 17);
		return;
	}

	char buffer[33];
	Com_Memset(buffer, 0, sizeof(buffer));

	FS_Read(buffer, 16, f);
	FS_FCloseFile(f);

	if (CLT3_CDKeyValidate(buffer, NULL))
	{
		String::Cat(&clt3_cdkey[16], sizeof(clt3_cdkey) - 16, buffer);
	}
	else
	{
		String::NCpyZ(&clt3_cdkey[16], "                ", 17);
	}
}

static void CLT3_WriteCDKeyForGame(const char* gameName, const char* ikey)
{
	char fbuffer[MAX_OSPATH];
	MakeCDKeyFileName(gameName, fbuffer);

	char key[17];
	String::NCpyZ(key, ikey, 17);

	if (!CLT3_CDKeyValidate(key, NULL))
	{
		return;
	}

	fileHandle_t f = FS_SV_FOpenFileWrite(fbuffer);
	if (!f)
	{
		common->Printf("Couldn't write %s.\n", gameName);
		return;
	}

	FS_Write(key, 16, f);

	FS_Printf(f, "\n// generated by %s, do not modify\r\n", GGameType & GAME_ET ? "ET" : GGameType & GAME_Quake3 ? "quake" : "RTCW");
	FS_Printf(f, "// Do not give this file to ANYONE.\r\n");
	FS_Printf(f, "// id Software and Activision will NOT ask you to send this file to them.\r\n");
	FS_FCloseFile(f);
}

void CLT3_WriteCDKey()
{
	Cvar* fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (UIT3_UsesUniqueCDKey() && fs && fs->string[0] != 0)
	{
		CLT3_WriteCDKeyForGame(fs->string, &clt3_cdkey[16]);
	}
	else
	{
		CLT3_WriteCDKeyForGame(fs_PrimaryBaseGame, clt3_cdkey);
	}
}

void CLT3_CDKeyForAuthorize(char* nums)
{
	// only grab the alphanumeric values from the cdkey, to avoid any dashes or spaces
	int j = 0;
	int l = String::Length(clt3_cdkey);
	if (l > 32)
	{
		l = 32;
	}
	for (int i = 0; i < l; i++)
	{
		if ((clt3_cdkey[i] >= '0' && clt3_cdkey[i] <= '9') ||
			(clt3_cdkey[i] >= 'a' && clt3_cdkey[i] <= 'z') ||
			(clt3_cdkey[i] >= 'A' && clt3_cdkey[i] <= 'Z')
			)
		{
			nums[j] = clt3_cdkey[i];
			j++;
		}
	}
	nums[j] = 0;
}

void CLT3UI_GetCDKey(char* buf, int buflen)
{
	Cvar* fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (UIT3_UsesUniqueCDKey() && fs && fs->string[0] != 0)
	{
		Com_Memcpy(buf, &clt3_cdkey[16], 16);
		buf[16] = 0;
	}
	else
	{
		Com_Memcpy(buf, clt3_cdkey, 16);
		buf[16] = 0;
	}
}

void CLT3UI_SetCDKey(char* buf)
{
	Cvar* fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (UIT3_UsesUniqueCDKey() && fs && fs->string[0] != 0)
	{
		Com_Memcpy(&clt3_cdkey[16], buf, 16);
		clt3_cdkey[32] = 0;
		// set the flag so the fle will be written at the next opportunity
		cvar_modifiedFlags |= CVAR_ARCHIVE;
	}
	else
	{
		Com_Memcpy(clt3_cdkey, buf, 16);
		// set the flag so the fle will be written at the next opportunity
		cvar_modifiedFlags |= CVAR_ARCHIVE;
	}
}