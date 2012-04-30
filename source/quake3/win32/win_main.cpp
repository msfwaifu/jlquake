/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// win_main.c

#include "../client/client.h"
#include "../qcommon/qcommon.h"
#include "../../client/windows_shared.h"
#include <direct.h>

static char sys_cmdline[MAX_STRING_CHARS];

/*
==================
Sys_BeginProfiling
==================
*/
void Sys_BeginProfiling(void)
{
	// this is just used on the mac build
}

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void Sys_Error(const char* error, ...)
{
	va_list argptr;
	char text[4096];
	MSG msg;

	va_start(argptr, error);
	Q_vsnprintf(text, 4096, error, argptr);
	va_end(argptr);

	Sys_Print(text);
	Sys_Print("\n");

	Sys_SetErrorText(text);
	Sys_ShowConsole(1, true);

	timeEndPeriod(1);

#ifndef DEDICATED
	IN_Shutdown();
#endif

	// wait for the user to quit
	while (1)
	{
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			Com_Quit_f();
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Sys_DestroyConsole();

	exit(1);
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit(void)
{
	timeEndPeriod(1);
#ifndef DEDICATED
	IN_Shutdown();
#endif
	Sys_DestroyConsole();

	exit(0);
}

/*
========================================================================

LOAD/UNLOAD DLL

========================================================================
*/

/*
=================
Sys_VM_UnloadDll

=================
*/
void Sys_VM_UnloadDll(void* dllHandle)
{
	Sys_UnloadDll(dllHandle);
}

/*
=================
Sys_VM_LoadDll

Used to load a development dll instead of a virtual machine

TTimo: added some verbosity in debug
=================
*/
// fqpath param added 7/20/02 by T.Ray - Sys_VM_LoadDll is only called in vm.c at this time
// fqpath will be empty if dll not loaded, otherwise will hold fully qualified path of dll module loaded
// fqpath buffersize must be at least MAX_QPATH+1 bytes long
void* Sys_VM_LoadDll(const char* name, char* fqpath, qintptr(**entryPoint) (int, ...),
	int (* systemcalls)(qintptr, ...))
{
	static int lastWarning = 0;
	HINSTANCE libHandle;
	void (* dllEntry)(int (* syscallptr)(qintptr, ...));
	const char* basepath;
	const char* cdpath;
	const char* gamedir;
	char* fn;
#ifdef NDEBUG
	int timestamp;
	int ret;
#endif
	char filename[MAX_QPATH];

	*fqpath = 0;		// added 7/20/02 by T.Ray

	String::NCpyZ(filename, Sys_GetDllName(name), sizeof(filename));

#ifdef NDEBUG
	timestamp = Sys_Milliseconds();
	if (((timestamp - lastWarning) > (5 * 60000)) && !Cvar_VariableIntegerValue("dedicated") &&
		!Cvar_VariableIntegerValue("com_blindlyLoadDLLs"))
	{
		if (FS_FileExists(filename))
		{
			lastWarning = timestamp;
			ret = MessageBoxEx(NULL, "You are about to load a .DLL executable that\n"
									 "has not been verified for use with Quake III Arena.\n"
									 "This type of file can compromise the security of\n"
									 "your computer.\n\n"
									 "Select 'OK' if you choose to load it anyway.",
				"Security Warning", MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_TOPMOST | MB_SETFOREGROUND,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
			if (ret != IDOK)
			{
				return NULL;
			}
		}
	}
#endif

#ifndef NDEBUG
	libHandle = Sys_LoadDll(filename);
	if (libHandle)
	{
		Com_Printf("LoadLibrary '%s' ok\n", filename);
	}
	else
	{
		Com_Printf("LoadLibrary '%s' failed\n", filename);
	}
	if (!libHandle)
	{
#endif
	basepath = Cvar_VariableString("fs_basepath");
	cdpath = Cvar_VariableString("fs_cdpath");
	gamedir = Cvar_VariableString("fs_game");

	fn = FS_BuildOSPath(basepath, gamedir, filename);
	libHandle = Sys_LoadDll(fn);
#ifndef NDEBUG
	if (libHandle)
	{
		Com_Printf("LoadLibrary '%s' ok\n", fn);
	}
	else
	{
		Com_Printf("LoadLibrary '%s' failed\n", fn);
	}
#endif

	if (!libHandle)
	{
		if (cdpath[0])
		{
			fn = FS_BuildOSPath(cdpath, gamedir, filename);
			libHandle = Sys_LoadDll(fn);
#ifndef NDEBUG
			if (libHandle)
			{
				Com_Printf("LoadLibrary '%s' ok\n", fn);
			}
			else
			{
				Com_Printf("LoadLibrary '%s' failed\n", fn);
			}
#endif
		}

		if (!libHandle)
		{
			return NULL;
		}
	}
#ifndef NDEBUG
}
#endif

	dllEntry = (void (*)(int (*)(qintptr, ...)))Sys_GetDllFunction(libHandle, "dllEntry");
	*entryPoint = (qintptr (*)(int,...))Sys_GetDllFunction(libHandle, "vmMain");
	if (!*entryPoint || !dllEntry)
	{
		Sys_UnloadDll(libHandle);
		return NULL;
	}
	dllEntry(systemcalls);

	if (libHandle)
	{
		String::NCpyZ(fqpath, filename, MAX_QPATH);							// added 7/20/02 by T.Ray
	}
	return libHandle;
}


/*
========================================================================

EVENT LOOP

========================================================================
*/

byte sys_packetReceived[MAX_MSGLEN_Q3];

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent(void)
{
	MSG msg;
	char* s;
	QMsg netmsg;
	netadr_t adr;

	// return if we have data
	if (eventHead > eventTail)
	{
		eventTail++;
		return eventQue[(eventTail - 1) & MASK_QUED_EVENTS];
	}

	// pump the message loop
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			Com_Quit_f();
		}

		// save the msg time, because wndprocs don't have access to the timestamp
		sysMsgTime = msg.time;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// check for console commands
	s = Sys_ConsoleInput();
	if (s)
	{
		char* b;
		int len;

		len = String::Length(s) + 1;
		b = (char*)Mem_Alloc(len);
		String::NCpyZ(b, s, len - 1);
		Sys_QueEvent(0, SE_CONSOLE, 0, 0, len, b);
	}

	// check for network packets
	netmsg.Init(sys_packetReceived, sizeof(sys_packetReceived));
	if (Sys_GetPacket(&adr, &netmsg))
	{
		netadr_t* buf;
		int len;

		// copy out to a seperate buffer for qeueing
		// the readcount stepahead is for SOCKS support
		len = sizeof(netadr_t) + netmsg.cursize - netmsg.readcount;
		buf = (netadr_t*)Mem_Alloc(len);
		*buf = adr;
		Com_Memcpy(buf + 1, &netmsg._data[netmsg.readcount], netmsg.cursize - netmsg.readcount);
		Sys_QueEvent(0, SE_PACKET, 0, 0, len, buf);
	}

	return Sys_SharedGetEvent();
}

//================================================================

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
#ifndef DEDICATED
void Sys_In_Restart_f(void)
{
	IN_Shutdown();
	IN_Init();
}
#endif


/*
=================
Sys_Net_Restart_f

Restart the network subsystem
=================
*/
void Sys_Net_Restart_f(void)
{
	NET_Restart();
}


/*
================
Sys_Init

Called after the common systems (cvars, files, etc)
are initialized
================
*/
void Sys_Init(void)
{
	int cpuid;

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod(1);

#ifndef DEDICATED
	Cmd_AddCommand("in_restart", Sys_In_Restart_f);
#endif
	Cmd_AddCommand("net_restart", Sys_Net_Restart_f);

	OSVERSIONINFO osversion;
	osversion.dwOSVersionInfoSize = sizeof(osversion);

	if (!GetVersionEx(&osversion))
	{
		Sys_Error("Couldn't get OS info");
	}

	if (osversion.dwMajorVersion < 5)
	{
		Sys_Error("Quake3 requires Windows version 5 or greater");
	}

	Cvar_Set("arch", "winnt");

	//
	// figure out our CPU
	//
	Cvar_Get("sys_cpustring", "detect", 0);
	if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "detect"))
	{
		Com_Printf("...detecting CPU, found ");

		cpuid = Sys_GetProcessorId();

		switch (cpuid)
		{
		case CPUID_GENERIC:
			Cvar_Set("sys_cpustring", "generic");
			break;
		case CPUID_INTEL_UNSUPPORTED:
			Cvar_Set("sys_cpustring", "x86 (pre-Pentium)");
			break;
		case CPUID_INTEL_PENTIUM:
			Cvar_Set("sys_cpustring", "x86 (P5/PPro, non-MMX)");
			break;
		case CPUID_INTEL_MMX:
			Cvar_Set("sys_cpustring", "x86 (P5/Pentium2, MMX)");
			break;
		case CPUID_INTEL_KATMAI:
			Cvar_Set("sys_cpustring", "Intel Pentium III");
			break;
		case CPUID_AMD_3DNOW:
			Cvar_Set("sys_cpustring", "AMD w/ 3DNow!");
			break;
		case CPUID_AXP:
			Cvar_Set("sys_cpustring", "Alpha AXP");
			break;
		default:
			Com_Error(ERR_FATAL, "Unknown cpu type %d\n", cpuid);
			break;
		}
	}
	else
	{
		Com_Printf("...forcing CPU type to ");
		if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "generic"))
		{
			cpuid = CPUID_GENERIC;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "x87"))
		{
			cpuid = CPUID_INTEL_PENTIUM;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "mmx"))
		{
			cpuid = CPUID_INTEL_MMX;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "3dnow"))
		{
			cpuid = CPUID_AMD_3DNOW;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "PentiumIII"))
		{
			cpuid = CPUID_INTEL_KATMAI;
		}
		else if (!String::ICmp(Cvar_VariableString("sys_cpustring"), "axp"))
		{
			cpuid = CPUID_AXP;
		}
		else
		{
			Com_Printf("WARNING: unknown sys_cpustring '%s'\n", Cvar_VariableString("sys_cpustring"));
			cpuid = CPUID_GENERIC;
		}
	}
	Cvar_SetValue("sys_cpuid", cpuid);
	Com_Printf("%s\n", Cvar_VariableString("sys_cpustring"));

	Cvar_Set("username", Sys_GetCurrentUser());

#ifndef DEDICATED
	IN_Init();		// FIXME: not in dedicated?
#endif
}


//=======================================================================

int totalMsec, countMsec;

/*
==================
WinMain

==================
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char cwd[MAX_OSPATH];
	int startTime, endTime;

	// should never get a previous instance in Win32
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;
	String::NCpyZ(sys_cmdline, lpCmdLine, sizeof(sys_cmdline));

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole("Quake 3 Console");

	// no abort/retry/fail errors
	SetErrorMode(SEM_FAILCRITICALERRORS);

	// get the initial time base
	Sys_Milliseconds();

	Com_Init(sys_cmdline);
	NET_Init();

	_getcwd(cwd, sizeof(cwd));
	Com_Printf("Working directory: %s\n", cwd);

	// hide the early console since we've reached the point where we
	// have a working graphics subsystems
	if (!com_dedicated->integer && !com_viewlog->integer)
	{
		Sys_ShowConsole(0, qfalse);
	}

	// main game loop
	while (1)
	{
		// if not running as a game client, sleep a bit
#ifndef DEDICATED
		if (Minimized || (com_dedicated && com_dedicated->integer))
#endif
		{
			Sleep(5);
		}

		// set low precision every frame, because some system calls
		// reset it arbitrarily
//		_controlfp( _PC_24, _MCW_PC );
//    _controlfp( -1, _MCW_EM  ); // no exceptions, even if some crappy
		// syscall turns them back on!

		startTime = Sys_Milliseconds();

#ifndef DEDICATED
		// make sure mouse and joystick are only called once a frame
		IN_Frame();
#endif

		// run the game
		Com_Frame();

		endTime = Sys_Milliseconds();
		totalMsec += endTime - startTime;
		countMsec++;
	}

	// never gets here
}
