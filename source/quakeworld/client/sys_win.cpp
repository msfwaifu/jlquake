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
// sys_win.h

#include "quakedef.h"
#include "../../client/windows_shared.h"

#define PAUSE_SLEEP     50				// sleep time on pause or minimization
#define NOT_FOCUS_SLEEP 20				// sleep time when not focus

#define MAX_NUM_ARGVS   50

HANDLE qwclsemaphore;

static HANDLE tevent;

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

/*
================
Sys_Init
================
*/
void Sys_Init(void)
{
	LARGE_INTEGER PerformanceFreq;
	unsigned int lowpart, highpart;

#ifndef SERVERONLY
	// allocate a named semaphore on the client so the
	// front end can tell if it is alive

	// mutex will fail if semephore allready exists
	qwclsemaphore = CreateMutex(
		NULL,			/* Security attributes */
		0,				/* owner       */
		"qwcl");/* Semaphore name      */
	if (!qwclsemaphore)
	{
		common->FatalError("QWCL is already running on this system");
	}
	CloseHandle(qwclsemaphore);

	qwclsemaphore = CreateSemaphore(
		NULL,			/* Security attributes */
		0,				/* Initial count       */
		1,				/* Maximum count       */
		"qwcl");/* Semaphore name      */
#endif

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod(1);
}


void Sys_Error(const char* error, ...)
{
	va_list argptr;
	char text[1024], text2[1024];
	DWORD dummy;

	Host_Shutdown();

	va_start(argptr, error);
	Q_vsnprintf(text, 1024, error, argptr);
	va_end(argptr);

	Sys_Print(text);
	Sys_Print("\n");

	Sys_SetErrorText(text);
	Sys_ShowConsole(1, true);

#ifndef SERVERONLY
	CloseHandle(qwclsemaphore);
#endif

	// wait for the user to quit
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Sys_DestroyConsole();
	exit(1);
}

void Sys_Quit(void)
{
	Host_Shutdown();
#ifndef SERVERONLY
	if (tevent)
	{
		CloseHandle(tevent);
	}

	if (qwclsemaphore)
	{
		CloseHandle(qwclsemaphore);
	}
#endif

	Sys_DestroyConsole();
	exit(0);
}

void Sys_Sleep(void)
{
}


void Sys_SendKeyEvents(void)
{
	Sys_MessageLoop();
}



/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
==================
WinMain
==================
*/
void SleepUntilInput(int time)
{

	MsgWaitForMultipleObjects(1, &tevent, FALSE, time, QS_ALLINPUT);
}



/*
==================
WinMain
==================
*/
char* argv[MAX_NUM_ARGVS];
static char* empty_string = "";


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	quakeparms_t parms;
	double time, oldtime, newtime;
	static char cwd[1024];
	int t;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
	{
		return 0;
	}

	global_hInstance = hInstance;

	if (!GetCurrentDirectory(sizeof(cwd), cwd))
	{
		common->FatalError("Couldn't determine current directory");
	}

	if (cwd[String::Length(cwd) - 1] == '/')
	{
		cwd[String::Length(cwd) - 1] = 0;
	}

	parms.basedir = cwd;

	parms.argc = 1;
	argv[0] = empty_string;

	while (*lpCmdLine && (parms.argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[parms.argc] = lpCmdLine;
			parms.argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}

		}
	}

	parms.argv = argv;

	COM_InitArgv2(parms.argc, parms.argv);

	Sys_CreateConsole("QuakeWorld Console");

	tevent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!tevent)
	{
		common->FatalError("Couldn't create event");
	}

	Sys_Init();

	common->Printf("Host_Init\n");
	Host_Init(&parms);

	oldtime = Sys_DoubleTime();

	/* main window message loop */
	while (1)
	{
		// yield the CPU for a little while when paused, minimized, or not the focus
		if ((cl.qh_paused && !ActiveApp) || Minimized)
		{
			SleepUntilInput(PAUSE_SLEEP);
		}
		else if (!ActiveApp)
		{
			SleepUntilInput(NOT_FOCUS_SLEEP);
		}

		newtime = Sys_DoubleTime();
		time = newtime - oldtime;
		Host_Frame(time);
		oldtime = newtime;
	}

	/* return success of application */
	return TRUE;
}
