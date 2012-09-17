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
#include "ui_public.h"

bool UIQ3_ConsoleCommand(int realTime)
{
	return VM_Call(uivm, Q3UI_CONSOLE_COMMAND, realTime);
}

void UIQ3_DrawConnectScreen(bool overlay)
{
	VM_Call(uivm, Q3UI_DRAW_CONNECT_SCREEN, overlay);
}

bool UIQ3_HasUniqueCDKey()
{
	return VM_Call(uivm, Q3UI_HASUNIQUECDKEY);
}

//	The ui module is making a system call
qintptr CLQ3_UISystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case Q3UI_ERROR:
		common->Error("%s", (char*)VMA(1));
		return 0;

	case Q3UI_PRINT:
		common->Printf("%s", (char*)VMA(1));
		return 0;

	case Q3UI_MILLISECONDS:
		return Sys_Milliseconds();

	case Q3UI_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;

	case Q3UI_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;

	case Q3UI_CVAR_SET:
		Cvar_Set((char*)VMA(1), (char*)VMA(2));
		return 0;

	case Q3UI_CVAR_VARIABLEVALUE:
		return FloatAsInt(Cvar_VariableValue((char*)VMA(1)));

	case Q3UI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_CVAR_SETVALUE:
		Cvar_SetValue((char*)VMA(1), VMF(2));
		return 0;

	case Q3UI_CVAR_RESET:
		Cvar_Reset((char*)VMA(1));
		return 0;

	case Q3UI_CVAR_CREATE:
		Cvar_Get((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_CVAR_INFOSTRINGBUFFER:
		Cvar_InfoStringBuffer(args[1], MAX_INFO_STRING_Q3, (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_ARGC:
		return Cmd_Argc();

	case Q3UI_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_CMD_EXECUTETEXT:
		Cbuf_ExecuteText(args[1], (char*)VMA(2));
		return 0;

	case Q3UI_FS_FOPENFILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);

	case Q3UI_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;

	case Q3UI_FS_WRITE:
		FS_Write(VMA(1), args[2], args[3]);
		return 0;

	case Q3UI_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;

	case Q3UI_FS_GETFILELIST:
		return FS_GetFileList((char*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);

	case Q3UI_FS_SEEK:
		return FS_Seek(args[1], args[2], args[3]);

	case Q3UI_R_REGISTERMODEL:
		return R_RegisterModel((char*)VMA(1));

	case Q3UI_R_REGISTERSKIN:
		return R_RegisterSkin((char*)VMA(1));

	case Q3UI_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip((char*)VMA(1));

	case Q3UI_R_CLEARSCENE:
		R_ClearScene();
		return 0;

//--------

	case Q3UI_R_ADDPOLYTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), 1);
		return 0;

	case Q3UI_R_ADDLIGHTTOSCENE:
		R_AddLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5));
		return 0;

//--------

	case Q3UI_R_SETCOLOR:
		R_SetColor((float*)VMA(1));
		return 0;

	case Q3UI_R_DRAWSTRETCHPIC:
		R_StretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;

	case Q3UI_R_MODELBOUNDS:
		R_ModelBounds(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;

//--------

	case Q3UI_CM_LERPTAG:
		R_LerpTag((orientation_t*)VMA(1), args[2], args[3], args[4], VMF(5), (char*)VMA(6));
		return 0;

	case Q3UI_S_REGISTERSOUND:
		return S_RegisterSound((char*)VMA(1));

	case Q3UI_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], 127);
		return 0;

//--------

	case Q3UI_KEY_SETBINDING:
		Key_SetBinding(args[1], (char*)VMA(2));
		return 0;

	case Q3UI_KEY_ISDOWN:
		return Key_IsDown(args[1]);

	case Q3UI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();

	case Q3UI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode(args[1]);
		return 0;

	case Q3UI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

//--------

	case Q3UI_MEMORY_REMAINING:
		return 0x4000000;

//--------

	case Q3UI_SET_PBCLSTATUS:
		return 0;

	case Q3UI_R_REGISTERFONT:
		R_RegisterFont((char*)VMA(1), args[2], (fontInfo_t*)VMA(3));
		return 0;

	case Q3UI_MEMSET:
		Com_Memset(VMA(1), args[2], args[3]);
		return 0;

	case Q3UI_MEMCPY:
		Com_Memcpy(VMA(1), VMA(2), args[3]);
		return 0;

	case Q3UI_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return (qintptr)(char*)VMA(1);

	case Q3UI_SIN:
		return FloatAsInt(sin(VMF(1)));

	case Q3UI_COS:
		return FloatAsInt(cos(VMF(1)));

	case Q3UI_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));

	case Q3UI_SQRT:
		return FloatAsInt(sqrt(VMF(1)));

	case Q3UI_FLOOR:
		return FloatAsInt(floor(VMF(1)));

	case Q3UI_CEIL:
		return FloatAsInt(ceil(VMF(1)));

	case Q3UI_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case Q3UI_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case Q3UI_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case Q3UI_PC_READ_TOKEN:
		return PC_ReadTokenHandleQ3(args[1], (q3pc_token_t*)VMA(2));
	case Q3UI_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));

	case Q3UI_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;
	case Q3UI_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack((char*)VMA(1), (char*)VMA(2), 0);
		return 0;

//--------

	case Q3UI_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematic((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);

//--------

	case Q3UI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

//--------

	case Q3UI_R_REMAP_SHADER:
		R_RemapShader((char*)VMA(1), (char*)VMA(2), (char*)VMA(3));
		return 0;

//--------

	default:
		common->Error("Bad UI system trap: %i", static_cast<int>(args[0]));
	}
	return 0;
}
