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

// common.c -- misc functions used in client and server

#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../client/client.h"
#include "../../server/server.h"
#include "../../server/tech3/local.h"
#include <setjmp.h>

// htons
#ifdef __linux__
#include <netinet/in.h>
// getpid
#include <unistd.h>
#elif __MACOS__
// getpid
#include <unistd.h>
#else
#include <winsock.h>
#endif

#define DEF_COMZONEMEGS "24"// RF, increased this from 16, to account for botlib/AAS

bool UIT3_UsesUniqueCDKey();

jmp_buf abortframe;		// an ERR_DROP occured, exit the entire frame


FILE* debuglogfile;
static fileHandle_t logfile;

Cvar* com_ignorecrash = NULL;		// bani - let experienced users ignore crashes, explicit NULL to make win32 teh happy
Cvar* com_pid;			// bani - process id

Cvar* com_fixedtime;
Cvar* com_dropsim;			// 0.0 to 1.0, simulated packet drops
Cvar* com_maxfps;
Cvar* com_timedemo;
Cvar* com_logfile;			// 1 = buffer log, 2 = flush after each print
Cvar* com_showtrace;
Cvar* com_version;
//Cvar	*com_blood;
Cvar* com_buildScript;		// for automated data building scripts
Cvar* com_introPlayed;
Cvar* com_logosPlaying;
Cvar* com_cameraMode;
#if defined(_WIN32) && defined(_DEBUG)
Cvar* com_noErrorInterrupt;
#endif
Cvar* com_recommendedSet;

Cvar* com_watchdog;
Cvar* com_watchdog_cmd;

// Rafael Notebook
Cvar* cl_notebook;

Cvar* com_hunkused;			// Ridah

// com_speeds times
int time_frontend;			// renderer frontend time
int time_backend;			// renderer backend time

int com_frameMsec;
int com_frameNumber;


qboolean com_fullyInitialized;

char com_errorMessage[MAXPRINTMSG];

void Com_WriteConfig_f(void);
void CIN_CloseAllVideos();

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3)));
};

static idCommonLocal commonLocal;
idCommon* common = &commonLocal;

void idCommonLocal::Printf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_Printf("%s", string);
}

void idCommonLocal::DPrintf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_DPrintf("%s", string);
}

void idCommonLocal::Error(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw DropException(string);
}

void idCommonLocal::FatalError(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw Exception(string);
}

void idCommonLocal::EndGame(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw EndGameException(string);
}

//============================================================================

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
int QDECL Com_VPrintf(const char* fmt, va_list argptr)
{
	char msg[MAXPRINTMSG];
	static qboolean opening_qconsole = false;

	// FIXME TTimo
	// switched vsprintf -> vsnprintf
	// rcon could cause buffer overflow
	//
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);

	if (rd_buffer)
	{
		if ((String::Length(msg) + String::Length(rd_buffer)) > (rd_buffersize - 1))
		{
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		String::Cat(rd_buffer, rd_buffersize, msg);
		// show_bug.cgi?id=51
		// only flush the rcon buffer when it's necessary, avoid fragmenting
		//rd_flush(rd_buffer);
		//*rd_buffer = 0;
		return String::Length(msg);
	}

	// echo to console if we're not a dedicated server
	if (com_dedicated && !com_dedicated->integer)
	{
		Con_ConsolePrint(msg);
	}

	// echo to dedicated console and early console
	Sys_Print(msg);

	// logfile
	if (com_logfile && com_logfile->integer)
	{
		// TTimo: only open the qconsole.log if the filesystem is in an initialized state
		//   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
		if (!logfile && FS_Initialized() && !opening_qconsole)
		{
			struct tm* newtime;
			time_t aclock;

			opening_qconsole = true;

			time(&aclock);
			newtime = localtime(&aclock);

			logfile = FS_FOpenFileWrite("etconsole.log");
			common->Printf("logfile opened on %s\n", asctime(newtime));
			if (com_logfile->integer > 1)
			{
				// force it to not buffer so we get valid
				// data even if we are crashing
				FS_ForceFlush(logfile);
			}

			opening_qconsole = false;
		}
		if (logfile && FS_Initialized())
		{
			FS_Write(msg, String::Length(msg), logfile);
		}
	}
	return String::Length(msg);
}
int QDECL Com_VPrintf(const char* fmt, va_list argptr) id_attribute((format(printf,1,0)));

void QDECL Com_Printf(const char* fmt, ...)
{
	va_list argptr;

	va_start(argptr, fmt);
	Com_VPrintf(fmt, argptr);
	va_end(argptr);
}
void QDECL Com_Printf(const char* fmt, ...) id_attribute((format(printf,1,2)));

/*
================
Com_DPrintf

A common->Printf that only shows up if the "developer" cvar is set
================
*/
void QDECL Com_DPrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	if (!com_developer || com_developer->integer != 1)
	{
		return;			// don't confuse non-developers with techie stuff...
	}

	va_start(argptr,fmt);
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end(argptr);

	common->Printf("%s", msg);
}
void QDECL Com_DPrintf(const char* fmt, ...) id_attribute((format(printf,1,2)));

/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void QDECL Com_Error(int code, const char* fmt, ...)
{
	va_list argptr;
	static int lastErrorTime;
	static int errorCount;
	int currentTime;

#if 0	//#if defined(_WIN32) && defined(_DEBUG)
	if (code != ERR_DISCONNECT)
	{
		if (!com_noErrorInterrupt->integer)
		{
			__asm {
				int 0x03
			}
		}
	}
#endif

	// when we are running automated scripts, make sure we
	// know if anything failed
	if (com_buildScript && com_buildScript->integer)
	{
		code = ERR_FATAL;
	}

	// make sure we can get at our local stuff
	FS_PureServerSetLoadedPaks("", "");

	// if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
	currentTime = Sys_Milliseconds();
	if (currentTime - lastErrorTime < 100)
	{
		if (++errorCount > 3)
		{
			code = ERR_FATAL;
		}
	}
	else
	{
		errorCount = 0;
	}
	lastErrorTime = currentTime;

	if (com_errorEntered)
	{
		char buf[4096];

		va_start(argptr,fmt);
		Q_vsnprintf(buf, sizeof(buf), fmt, argptr);
		va_end(argptr);

		Sys_Error("recursive error '%s' after: %s", buf, com_errorMessage);
	}
	com_errorEntered = true;

	va_start(argptr,fmt);
	Q_vsnprintf(com_errorMessage, sizeof(com_errorMessage), fmt, argptr);
	va_end(argptr);

	if (code != ERR_DISCONNECT)
	{
		Cvar_Set("com_errorMessage", com_errorMessage);
	}

	if (code == ERR_SERVERDISCONNECT)
	{
		CL_Disconnect(true);
		CL_FlushMemory();
		com_errorEntered = false;
		longjmp(abortframe, -1);
	}
	else if (code == ERR_DROP || code == ERR_DISCONNECT)
	{
		common->Printf("********************\nERROR: %s\n********************\n", com_errorMessage);
		SVT3_Shutdown(va("Server crashed: %s\n",  com_errorMessage));
		CL_Disconnect(true);
		CL_FlushMemory();
		com_errorEntered = false;
		longjmp(abortframe, -1);
	}
#ifndef DEDICATED
	else if (code == ERR_AUTOUPDATE)
	{
		CL_Disconnect(true);
		CL_FlushMemory();
		com_errorEntered = false;
		if (!String::NICmp(com_errorMessage, "Server is full", 14) && CL_NextUpdateServer())
		{
			CL_GetAutoUpdate();
		}
		else
		{
			longjmp(abortframe, -1);
		}
	}
#endif
	else
	{
		CL_Shutdown();
		SVT3_Shutdown(va("Server fatal crashed: %s\n", com_errorMessage));
	}

	Com_Shutdown(code == ERR_VID_FATAL ? true : false);

	Sys_Error("%s", com_errorMessage);
}
void QDECL Com_Error(int code, const char* fmt, ...) id_attribute((format(printf,2,3)));

//bani - moved
void CLT3_ShutdownCGame(void);

/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit_f(void)
{
	// don't try to shutdown if we are in a recursive error
	if (!com_errorEntered)
	{
		SVT3_Shutdown("Server quit\n");
//bani
#ifndef DEDICATED
		CLT3_ShutdownCGame();
#endif
		CL_Shutdown();
		Com_Shutdown(false);
		FS_Shutdown();
	}
	Sys_Quit();
}

/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define MAX_CONSOLE_LINES   32
int com_numConsoleLines;
char* com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine(char* commandLine)
{
	int inq = 0;
	com_consoleLines[0] = commandLine;
	com_numConsoleLines = 1;

	while (*commandLine)
	{
		if (*commandLine == '"')
		{
			inq = !inq;
		}
		// look for a + seperating character
		// if commandLine came from a file, we might have real line seperators
		if (*commandLine == '+' || *commandLine == '\n' || *commandLine == '\r')
		{
			if (com_numConsoleLines == MAX_CONSOLE_LINES)
			{
				return;
			}
			com_consoleLines[com_numConsoleLines] = commandLine + 1;
			com_numConsoleLines++;
			*commandLine = 0;
		}
		commandLine++;
	}
}


/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of wolfconfig.cfg
===================
*/
qboolean Com_SafeMode(void)
{
	int i;

	for (i = 0; i < com_numConsoleLines; i++)
	{
		Cmd_TokenizeString(com_consoleLines[i]);
		if (!String::ICmp(Cmd_Argv(0), "safe") ||
			!String::ICmp(Cmd_Argv(0), "cvar_restart"))
		{
			com_consoleLines[i][0] = 0;
			return true;
		}
	}
	return false;
}


/*
===============
Com_StartupVariable

Searches for command line parameters that are set commands.
If match is not NULL, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets shouls
be after execing the config and default.
===============
*/
void Com_StartupVariable(const char* match)
{
	int i;
	char* s;
	Cvar* cv;

	for (i = 0; i < com_numConsoleLines; i++)
	{
		Cmd_TokenizeString(com_consoleLines[i]);
		if (String::Cmp(Cmd_Argv(0), "set"))
		{
			continue;
		}

		s = Cmd_Argv(1);
		if (!match || !String::Cmp(s, match))
		{
			Cvar_Set(s, Cmd_Argv(2));
			cv = Cvar_Get(s, "", 0);
			cv->flags |= CVAR_USER_CREATED;
//			com_consoleLines[i] = 0;
		}
	}
}


/*
=================
Com_AddStartupCommands

Adds command line parameters as script statements
Commands are seperated by + signs

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Com_AddStartupCommands(void)
{
	int i;
	qboolean added;

	added = false;
	// quote every token, so args with semicolons can work
	for (i = 0; i < com_numConsoleLines; i++)
	{
		if (!com_consoleLines[i] || !com_consoleLines[i][0])
		{
			continue;
		}

		// set commands won't override menu startup
		if (String::NICmp(com_consoleLines[i], "set", 3))
		{
			added = true;
		}
		Cbuf_AddText(com_consoleLines[i]);
		Cbuf_AddText("\n");
	}

	return added;
}

/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

// bk001129 - here we go again: upped from 64
#define MAX_PUSHED_EVENTS               256
// bk001129 - init, also static
static int com_pushedEventsHead = 0;
static int com_pushedEventsTail = 0;
// bk001129 - static
static sysEvent_t com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling(void)
{
	Com_StartupVariable("journal");
	com_journal = Cvar_Get("journal", "0", CVAR_INIT);
	if (!com_journal->integer)
	{
		return;
	}

	if (com_journal->integer == 1)
	{
		common->Printf("Journaling events\n");
		com_journalFile = FS_FOpenFileWrite("journal.dat");
		com_journalDataFile = FS_FOpenFileWrite("journaldata.dat");
	}
	else if (com_journal->integer == 2)
	{
		common->Printf("Replaying journaled events\n");
		FS_FOpenFileRead("journal.dat", &com_journalFile, true);
		FS_FOpenFileRead("journaldata.dat", &com_journalDataFile, true);
	}

	if (!com_journalFile || !com_journalDataFile)
	{
		Cvar_Set("com_journal", "0");
		com_journalFile = 0;
		com_journalDataFile = 0;
		common->Printf("Couldn't open journal files\n");
	}
}

/*
=================
Com_GetRealEvent
=================
*/
sysEvent_t  Com_GetRealEvent(void)
{
	int r;
	sysEvent_t ev;

	// either get an event from the system or the journal file
	if (com_journal->integer == 2)
	{
		r = FS_Read(&ev, sizeof(ev), com_journalFile);
		if (r != sizeof(ev))
		{
			common->FatalError("Error reading from journal file");
		}
		if (ev.evPtrLength)
		{
			ev.evPtr = Mem_Alloc(ev.evPtrLength);
			r = FS_Read(ev.evPtr, ev.evPtrLength, com_journalFile);
			if (r != ev.evPtrLength)
			{
				common->FatalError("Error reading from journal file");
			}
		}
	}
	else
	{
		ev = Sys_GetEvent();

		// write the journal value out if needed
		if (com_journal->integer == 1)
		{
			r = FS_Write(&ev, sizeof(ev), com_journalFile);
			if (r != sizeof(ev))
			{
				common->FatalError("Error writing to journal file");
			}
			if (ev.evPtrLength)
			{
				r = FS_Write(ev.evPtr, ev.evPtrLength, com_journalFile);
				if (r != ev.evPtrLength)
				{
					common->FatalError("Error writing to journal file");
				}
			}
		}
	}

	return ev;
}


/*
=================
Com_InitPushEvent
=================
*/
// bk001129 - added
void Com_InitPushEvent(void)
{
	// clear the static buffer array
	// this requires SE_NONE to be accepted as a valid but NOP event
	memset(com_pushedEvents, 0, sizeof(com_pushedEvents));
	// reset counters while we are at it
	// beware: GetEvent might still return an SE_NONE from the buffer
	com_pushedEventsHead = 0;
	com_pushedEventsTail = 0;
}


/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent(sysEvent_t* event)
{
	sysEvent_t* ev;
	static int printedWarning = 0;	// bk001129 - init, bk001204 - explicit int

	ev = &com_pushedEvents[com_pushedEventsHead & (MAX_PUSHED_EVENTS - 1)];

	if (com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS)
	{

		// don't print the warning constantly, or it can give time for more...
		if (!printedWarning)
		{
			printedWarning = true;
			common->Printf("WARNING: Com_PushEvent overflow\n");
		}

		if (ev->evPtr)
		{
			Mem_Free(ev->evPtr);
		}
		com_pushedEventsTail++;
	}
	else
	{
		printedWarning = false;
	}

	*ev = *event;
	com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t  Com_GetEvent(void)
{
	if (com_pushedEventsHead > com_pushedEventsTail)
	{
		com_pushedEventsTail++;
		return com_pushedEvents[(com_pushedEventsTail - 1) & (MAX_PUSHED_EVENTS - 1)];
	}
	return Com_GetRealEvent();
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket(netadr_t* evFrom, QMsg* buf)
{
	int t1, t2, msec;

	t1 = 0;

	if (com_speeds->integer)
	{
		t1 = Sys_Milliseconds();
	}

	SVT3_PacketEvent(*evFrom, buf);

	if (com_speeds->integer)
	{
		t2 = Sys_Milliseconds();
		msec = t2 - t1;
		if (com_speeds->integer == 3)
		{
			common->Printf("SVT3_PacketEvent time: %i\n", msec);
		}
	}
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/

#ifndef DEDICATED
extern qboolean consoleButtonWasPressed;
#endif

int Com_EventLoop(void)
{
	sysEvent_t ev;
	netadr_t evFrom;
	byte bufData[MAX_MSGLEN_WOLF];
	QMsg buf;

	buf.Init(bufData, sizeof(bufData));

	while (1)
	{
		ev = Com_GetEvent();

		// if no more events are available
		if (ev.evType == SE_NONE)
		{
			// manually send packet events for the loopback channel
			while (NET_GetLoopPacket(NS_CLIENT, &evFrom, &buf))
			{
				CL_PacketEvent(evFrom, &buf);
			}

			while (NET_GetLoopPacket(NS_SERVER, &evFrom, &buf))
			{
				// if the server just shut down, flush the events
				if (com_sv_running->integer)
				{
					Com_RunAndTimeServerPacket(&evFrom, &buf);
				}
			}

			return ev.evTime;
		}


		switch (ev.evType)
		{
		default:
			// bk001129 - was ev.evTime
			common->FatalError("Com_EventLoop: bad event type %i", ev.evType);
			break;
		case SE_NONE:
			break;
		case SE_KEY:
			CL_KeyEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CHAR:
#ifndef DEDICATED
			// fretn
			// we just pressed the console button,
			// so ignore this event
			// this prevents chars appearing at console input
			// when you just opened it
			if (consoleButtonWasPressed)
			{
				consoleButtonWasPressed = false;
				break;
			}
#endif
			CL_CharEvent(ev.evValue);
			break;
		case SE_MOUSE:
			CL_MouseEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_JOYSTICK_AXIS:
			CL_JoystickEvent(ev.evValue, ev.evValue2, ev.evTime);
			break;
		case SE_CONSOLE:
			Cbuf_AddText((char*)ev.evPtr);
			Cbuf_AddText("\n");
			break;
		case SE_PACKET:
			// this cvar allows simulation of connections that
			// drop a lot of packets.  Note that loopback connections
			// don't go through here at all.
			if (com_dropsim->value > 0)
			{
				static int seed;

				if (Q_random(&seed) < com_dropsim->value)
				{
					break;		// drop this packet
				}
			}

			evFrom = *(netadr_t*)ev.evPtr;
			buf.cursize = ev.evPtrLength - sizeof(evFrom);

			// we must copy the contents of the message out, because
			// the event buffers are only large enough to hold the
			// exact payload, but channel messages need to be large
			// enough to hold fragment reassembly
			if ((unsigned)buf.cursize > buf.maxsize)
			{
				common->Printf("Com_EventLoop: oversize packet\n");
				continue;
			}
			memcpy(buf._data, (byte*)((netadr_t*)ev.evPtr + 1), buf.cursize);
			if (com_sv_running->integer)
			{
				Com_RunAndTimeServerPacket(&evFrom, &buf);
			}
			else
			{
				CL_PacketEvent(evFrom, &buf);
			}
			break;
		}

		// free any block data
		if (ev.evPtr)
		{
			Mem_Free(ev.evPtr);
		}
	}

	return 0;	// never reached
}

/*
================
Com_Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int Com_Milliseconds(void)
{
	sysEvent_t ev;

	// get events and push them until we get a null event with the current time
	do
	{

		ev = Com_GetRealEvent();
		if (ev.evType != SE_NONE)
		{
			Com_PushEvent(&ev);
		}
	}
	while (ev.evType != SE_NONE);

	return ev.evTime;
}

//============================================================================

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Com_Error_f(void)
{
	if (Cmd_Argc() > 1)
	{
		common->Error("Testing drop error");
	}
	else
	{
		common->FatalError("Testing fatal error");
	}
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f(void)
{
	float s;
	int start, now;

	if (Cmd_Argc() != 2)
	{
		common->Printf("freeze <seconds>\n");
		return;
	}
	s = String::Atof(Cmd_Argv(1));

	start = Com_Milliseconds();

	while (1)
	{
		now = Com_Milliseconds();
		if ((now - start) * 0.001 > s)
		{
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void Com_Crash_f(void)
{
	*(int*)0 = 0x12345678;
}

/*
=============
Com_CPUSpeed_f
=============
*/
void Com_CPUSpeed_f(void)
{
	common->Printf("CPU Speed: %.2f Mhz\n", Sys_GetCPUSpeed());
}

qboolean CL_CDKeyValidate(const char* key, const char* checksum);

// TTimo: centralizing the cl_cdkey stuff after I discovered a buffer overflow problem with the dedicated server version
//   not sure it's necessary to have different defaults for regular and dedicated, but I don't want to take the risk
#ifndef DEDICATED
char cl_cdkey[34] = "                                ";
#else
char cl_cdkey[34] = "123456789";
#endif

/*
=================
Com_ReadCDKey
=================
*/
void Com_ReadCDKey(const char* filename)
{
	fileHandle_t f;
	char buffer[33];
	char fbuffer[MAX_OSPATH];

	sprintf(fbuffer, "%s/rtcwkey", filename);

	FS_SV_FOpenFileRead(fbuffer, &f);
	if (!f)
	{
		String::NCpyZ(cl_cdkey, "                ", 17);
		return;
	}

	Com_Memset(buffer, 0, sizeof(buffer));

	FS_Read(buffer, 16, f);
	FS_FCloseFile(f);

	if (CL_CDKeyValidate(buffer, NULL))
	{
		String::NCpyZ(cl_cdkey, buffer, 17);
	}
	else
	{
		String::NCpyZ(cl_cdkey, "                ", 17);
	}
}

/*
=================
Com_ReadCDKey
=================
*/
void Com_AppendCDKey(const char* filename)
{
	fileHandle_t f;
	char buffer[33];
	char fbuffer[MAX_OSPATH];

	sprintf(fbuffer, "%s/rtcwkey", filename);

	FS_SV_FOpenFileRead(fbuffer, &f);
	if (!f)
	{
		String::NCpyZ(&cl_cdkey[16], "                ", 17);
		return;
	}

	Com_Memset(buffer, 0, sizeof(buffer));

	FS_Read(buffer, 16, f);
	FS_FCloseFile(f);

	if (CL_CDKeyValidate(buffer, NULL))
	{
		strcat(&cl_cdkey[16], buffer);
	}
	else
	{
		String::NCpyZ(&cl_cdkey[16], "                ", 17);
	}
}

#ifndef DEDICATED	// bk001204
/*
=================
Com_WriteCDKey
=================
*/
static void Com_WriteCDKey(const char* filename, const char* ikey)
{
	fileHandle_t f;
	char fbuffer[MAX_OSPATH];
	char key[17];


	sprintf(fbuffer, "%s/rtcwkey", filename);


	String::NCpyZ(key, ikey, 17);

	if (!CL_CDKeyValidate(key, NULL))
	{
		return;
	}

	f = FS_SV_FOpenFileWrite(fbuffer);
	if (!f)
	{
		common->Printf("Couldn't write %s.\n", filename);
		return;
	}

	FS_Write(key, 16, f);

	FS_Printf(f, "\n// generated by ET, do not modify\r\n");
	FS_Printf(f, "// Do not give this file to ANYONE.\r\n");
#ifdef __MACOS__
	FS_Printf(f, "// Aspyr will NOT ask you to send this file to them.\r\n");
#else
	FS_Printf(f, "// id Software and Activision will NOT ask you to send this file to them.\r\n");
#endif
	FS_FCloseFile(f);
}
#endif

void Com_SetRecommended()
{
	Cvar* com_recommended;
	qboolean goodVideo;
	float cpuSpeed;
	//qboolean goodCPU;
	// will use this for recommended settings as well.. do i outside the lower check so it gets done even with command line stuff
	com_recommended = Cvar_Get("com_recommended", "-1", CVAR_ARCHIVE);
	goodVideo = true;

	cpuSpeed = Sys_GetCPUSpeed();

	if (cpuSpeed > 1500)
	{
		if (goodVideo)
		{
			common->Printf("Found high quality video and fast CPU\n");
			Cbuf_AddText("exec preset_high.cfg\n");
			Cvar_Set("com_recommended", "0");
		}
		else
		{
			common->Printf("Found low quality video and fast CPU\n");
			Cbuf_AddText("exec preset_normal.cfg\n");
			Cvar_Set("com_recommended", "1");
		}
	}
	else if (cpuSpeed > 850)
	{
		if (goodVideo)
		{
			common->Printf("Found high quality video and normal CPU\n");
		}
		else
		{
			common->Printf("Found low quality video and normal CPU\n");
		}
		Cbuf_AddText("exec preset_normal.cfg\n");
		Cvar_Set("com_recommended", "1");
	}
	else if (cpuSpeed < 200)		// do the < 200 check just in case we barf, better than falling back to ugly fast
	{
		if (goodVideo)
		{
			common->Printf("Found high quality video but didn't manage to detect a CPU properly\n");
		}
		else
		{
			common->Printf("Found low quality video but didn't manage to detect a CPU properly\n");
		}
		Cbuf_AddText("exec preset_normal.cfg\n");
		Cvar_Set("com_recommended", "1");
	}
	else
	{
		if (goodVideo)
		{
			common->Printf("Found high quality video and slow CPU\n");
			Cbuf_AddText("exec preset_fast.cfg\n");
			Cvar_Set("com_recommended", "2");
		}
		else
		{
			common->Printf("Found low quality video and slow CPU\n");
			Cbuf_AddText("exec preset_fastest.cfg\n");
			Cvar_Set("com_recommended", "3");
		}
	}


	/*goodCPU = Sys_GetHighQualityCPU();

	if (goodVideo && goodCPU) {
	    common->Printf ("Found high quality video and CPU\n");
	    Cbuf_AddText ("exec highVidhighCPU.cfg\n");
	} else if (goodVideo && !goodCPU) {
	    Cbuf_AddText ("exec highVidlowCPU.cfg\n");
	    common->Printf ("Found high quality video and low quality CPU\n");
	} else if (!goodVideo && goodCPU) {
	    Cbuf_AddText ("exec lowVidhighCPU.cfg\n");
	    common->Printf ("Found low quality video and high quality CPU\n");
	} else {
	    Cbuf_AddText ("exec lowVidlowCPU.cfg\n");
	    common->Printf ("Found low quality video and low quality CPU\n");
	}*/

// (SA) set the cvar so the menu will reflect this on first run
//	Cvar_Set("ui_glCustom", "999");	// 'recommended'
}

void Com_GetGameInfo()
{
	char* f;
	const char* buf;
	char* token;

	memset(&comet_gameInfo, 0, sizeof(comet_gameInfo));

	if (FS_ReadFile("gameinfo.dat", (void**)&f) > 0)
	{

		buf = f;

		while ((token = String::Parse3(&buf)) != NULL && token[0])
		{
			if (!String::ICmp(token, "spEnabled"))
			{
				comet_gameInfo.spEnabled = true;
			}
			else if (!String::ICmp(token, "spGameTypes"))
			{
				while ((token = String::ParseExt(&buf, false)) != NULL && token[0])
				{
					comet_gameInfo.spGameTypes |= (1 << String::Atoi(token));
				}
			}
			else if (!String::ICmp(token, "defaultSPGameType"))
			{
				if ((token = String::ParseExt(&buf, false)) != NULL && token[0])
				{
					comet_gameInfo.defaultSPGameType = String::Atoi(token);
				}
				else
				{
					FS_FreeFile(f);
					common->FatalError("Com_GetGameInfo: bad syntax.");
				}
			}
			else if (!String::ICmp(token, "coopGameTypes"))
			{

				while ((token = String::ParseExt(&buf, false)) != NULL && token[0])
				{
					comet_gameInfo.coopGameTypes |= (1 << String::Atoi(token));
				}
			}
			else if (!String::ICmp(token, "defaultCoopGameType"))
			{
				if ((token = String::ParseExt(&buf, false)) != NULL && token[0])
				{
					comet_gameInfo.defaultCoopGameType = String::Atoi(token);
				}
				else
				{
					FS_FreeFile(f);
					common->FatalError("Com_GetGameInfo: bad syntax.");
				}
			}
			else if (!String::ICmp(token, "defaultGameType"))
			{
				if ((token = String::ParseExt(&buf, false)) != NULL && token[0])
				{
					comet_gameInfo.defaultGameType = String::Atoi(token);
				}
				else
				{
					FS_FreeFile(f);
					common->FatalError("Com_GetGameInfo: bad syntax.");
				}
			}
			else if (!String::ICmp(token, "usesProfiles"))
			{
				if ((token = String::ParseExt(&buf, false)) != NULL && token[0])
				{
					comet_gameInfo.usesProfiles = String::Atoi(token);
				}
				else
				{
					FS_FreeFile(f);
					common->FatalError("Com_GetGameInfo: bad syntax.");
				}
			}
			else
			{
				FS_FreeFile(f);
				common->FatalError("Com_GetGameInfo: bad syntax.");
			}
		}

		// all is good
		FS_FreeFile(f);
	}
}

// bani - checks if profile.pid is valid
// return true if it is
// return false if it isn't(!)
qboolean Com_CheckProfile(char* profile_path)
{
	fileHandle_t f;
	char f_data[32];
	int f_pid;

	//let user override this
	if (com_ignorecrash->integer)
	{
		return true;
	}

	if (FS_FOpenFileRead(profile_path, &f, true) < 0)
	{
		//no profile found, we're ok
		return true;
	}

	if (FS_Read(&f_data, sizeof(f_data) - 1, f) < 0)
	{
		//b0rk3d!
		FS_FCloseFile(f);
		//try to delete corrupted pid file
		FS_Delete(profile_path);
		return false;
	}

	f_pid = String::Atoi(f_data);
	if (f_pid != com_pid->integer)
	{
		//pid doesn't match
		FS_FCloseFile(f);
		return false;
	}

	//we're all ok
	FS_FCloseFile(f);
	return true;
}

//bani - from files.c
char last_fs_gamedir[MAX_OSPATH];
char last_profile_path[MAX_OSPATH];

//bani - track profile changes, delete old profile.pid if we change fs_game(dir)
//hackish, we fiddle with fs_gamedir to make FS_* calls work "right"
void Com_TrackProfile(char* profile_path)
{
	char temp_fs_gamedir[MAX_OSPATH];

//	common->Printf( "Com_TrackProfile: Tracking profile [%s] [%s]\n", fs_gamedir, profile_path );
	//have we changed fs_game(dir)?
	if (String::Cmp(last_fs_gamedir, fs_gamedir))
	{
		if (String::Length(last_fs_gamedir) && String::Length(last_profile_path))
		{
			//save current fs_gamedir
			String::NCpyZ(temp_fs_gamedir, fs_gamedir, sizeof(temp_fs_gamedir));
			//set fs_gamedir temporarily to make FS_* stuff work "right"
			String::NCpyZ(fs_gamedir, last_fs_gamedir, sizeof(fs_gamedir));
			if (FS_FileExists(last_profile_path))
			{
				common->Printf("Com_TrackProfile: Deleting old pid file [%s] [%s]\n", fs_gamedir, last_profile_path);
				FS_Delete(last_profile_path);
			}
			//restore current fs_gamedir
			String::NCpyZ(fs_gamedir, temp_fs_gamedir, sizeof(fs_gamedir));
		}
		//and save current vars for future reference
		String::NCpyZ(last_fs_gamedir, fs_gamedir, sizeof(last_fs_gamedir));
		String::NCpyZ(last_profile_path, profile_path, sizeof(last_profile_path));
	}
}

// bani - writes pid to profile
// returns true if successful
// returns false if not(!!)
qboolean Com_WriteProfile(char* profile_path)
{
	fileHandle_t f;

	if (FS_FileExists(profile_path))
	{
		FS_Delete(profile_path);
	}

	f = FS_FOpenFileWrite(profile_path);
	if (f < 0)
	{
		common->Printf("Com_WriteProfile: Can't write %s.\n", profile_path);
		return false;
	}

	FS_Printf(f, "%d", com_pid->integer);

	FS_FCloseFile(f);

	//track profile changes
	Com_TrackProfile(profile_path);

	return true;
}

/*
=================
Com_Init
=================
*/
void Com_Init(char* commandLine)
{
	try
	{
		char* s;
		int pid;
		// TTimo gcc warning: variable `safeMode' might be clobbered by `longjmp' or `vfork'
		volatile qboolean safeMode = true;

		common->Printf("%s %s %s\n", Q3_VERSION, CPUSTRING, __DATE__);

		if (setjmp(abortframe))
		{
			Sys_Error("Error during initialization");
		}

		GGameType = GAME_ET;
		Sys_SetHomePathSuffix("jlet");

		// bk001129 - do this before anything else decides to push events
		Com_InitPushEvent();

		Cvar_Init();

		// prepare enough of the subsystems to handle
		// cvar and command buffer management
		Com_ParseCommandLine(commandLine);

		Com_InitByteOrder();
		Cbuf_Init();

		Cmd_Init();

		// override anything from the config files with command line args
		Com_StartupVariable(NULL);

		// get the developer cvar set as early as possible
		Com_StartupVariable("developer");

		// bani: init this early
		Com_StartupVariable("com_ignorecrash");
		com_ignorecrash = Cvar_Get("com_ignorecrash", "0", 0);

		// ydnar: init crashed variable as early as possible
		com_crashed = Cvar_Get("com_crashed", "0", CVAR_TEMP);

		// bani: init pid
#ifdef _WIN32
		pid = GetCurrentProcessId();
#elif __linux__
		pid = getpid();
#elif __MACOS__
		pid = getpid();
#endif
		s = va("%d", pid);
		com_pid = Cvar_Get("com_pid", s, CVAR_ROM);

		// done early so bind command exists
		CL_InitKeyCommands();

		FS_InitFilesystem();

		Com_InitJournaling();

		Com_GetGameInfo();

		Cbuf_AddText("exec default.cfg\n");
		Cbuf_AddText("exec language.cfg\n");	// NERVE - SMF

		// skip the q3config.cfg if "safe" is on the command line
		if (!Com_SafeMode())
		{
			const char* cl_profileStr = Cvar_VariableString("cl_profile");

			safeMode = false;
			if (comet_gameInfo.usesProfiles)
			{
				if (!cl_profileStr[0])
				{
					char* defaultProfile = NULL;

					FS_ReadFile("profiles/defaultprofile.dat", (void**)&defaultProfile);

					if (defaultProfile)
					{
						const char* text_p = defaultProfile;
						char* token = String::Parse3(&text_p);

						if (token && *token)
						{
							Cvar_Set("cl_defaultProfile", token);
							Cvar_Set("cl_profile", token);
						}

						FS_FreeFile(defaultProfile);

						cl_profileStr = Cvar_VariableString("cl_defaultProfile");
					}
				}

				if (cl_profileStr[0])
				{
					// bani - check existing pid file and make sure it's ok
					if (!Com_CheckProfile(va("profiles/%s/profile.pid", cl_profileStr)))
					{
#ifndef _DEBUG
						common->Printf("^3WARNING: profile.pid found for profile '%s' - system settings will revert to defaults\n", cl_profileStr);
						// ydnar: set crashed state
						Cbuf_AddText("set com_crashed 1\n");
#endif
					}

					// bani - write a new one
					if (!Com_WriteProfile(va("profiles/%s/profile.pid", cl_profileStr)))
					{
						common->Printf("^3WARNING: couldn't write profiles/%s/profile.pid\n", cl_profileStr);
					}

					// exec the config
					Cbuf_AddText(va("exec profiles/%s/%s\n", cl_profileStr, CONFIG_NAME));
				}
			}
			else
			{
				Cbuf_AddText(va("exec %s\n", CONFIG_NAME));
			}
		}

		Cbuf_AddText("exec autoexec.cfg\n");

		// ydnar: reset crashed state
		Cbuf_AddText("set com_crashed 0\n");

		// execute the queued commands
		Cbuf_Execute();

		// override anything from the config files with command line args
		Com_StartupVariable(NULL);

#if DEDICATED
		// TTimo: default to internet dedicated, not LAN dedicated
		com_dedicated = Cvar_Get("dedicated", "2", CVAR_ROM);
#else
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_LATCH2);
#endif

		// if any archived cvars are modified after this, we will trigger a writing
		// of the config file
		cvar_modifiedFlags &= ~CVAR_ARCHIVE;

		//
		// init commands and vars
		//
		COM_InitCommonCvars();
		// Gordon: no need to latch this in ET, our recoil is framerate independant
		com_maxfps = Cvar_Get("com_maxfps", "85", CVAR_ARCHIVE /*|CVAR_LATCH2*/);
//	com_blood = Cvar_Get ("com_blood", "1", CVAR_ARCHIVE); // Gordon: no longer used?

		com_logfile = Cvar_Get("logfile", "0", CVAR_TEMP);

		com_fixedtime = Cvar_Get("fixedtime", "0", CVAR_CHEAT);
		com_showtrace = Cvar_Get("com_showtrace", "0", CVAR_CHEAT);
		com_dropsim = Cvar_Get("com_dropsim", "0", CVAR_CHEAT);
		com_speeds = Cvar_Get("com_speeds", "0", 0);
		com_timedemo = Cvar_Get("timedemo", "0", CVAR_CHEAT);
		com_cameraMode = Cvar_Get("com_cameraMode", "0", CVAR_CHEAT);

		com_watchdog = Cvar_Get("com_watchdog", "60", CVAR_ARCHIVE);
		com_watchdog_cmd = Cvar_Get("com_watchdog_cmd", "", CVAR_ARCHIVE);

		cl_paused = Cvar_Get("cl_paused", "0", CVAR_ROM);
		sv_paused = Cvar_Get("sv_paused", "0", CVAR_ROM);
		com_sv_running = Cvar_Get("sv_running", "0", CVAR_ROM);
		com_cl_running = Cvar_Get("cl_running", "0", CVAR_ROM);
		com_buildScript = Cvar_Get("com_buildScript", "0", 0);

		com_introPlayed = Cvar_Get("com_introplayed", "0", CVAR_ARCHIVE);
		com_logosPlaying = Cvar_Get("com_logosPlaying", "0", CVAR_ROM);
		com_recommendedSet = Cvar_Get("com_recommendedSet", "0", CVAR_ARCHIVE);

		Cvar_Get("savegame_loading", "0", CVAR_ROM);

#if defined(_WIN32) && defined(_DEBUG)
		com_noErrorInterrupt = Cvar_Get("com_noErrorInterrupt", "0", 0);
#endif

		com_hunkused = Cvar_Get("com_hunkused", "0", 0);

		if (com_dedicated->integer)
		{
			if (!com_viewlog->integer)
			{
				Cvar_Set("viewlog", "1");
			}
		}

		if (com_developer && com_developer->integer)
		{
			Cmd_AddCommand("error", Com_Error_f);
			Cmd_AddCommand("crash", Com_Crash_f);
			Cmd_AddCommand("freeze", Com_Freeze_f);
			Cmd_AddCommand("cpuspeed", Com_CPUSpeed_f);
		}
		Cmd_AddCommand("quit", Com_Quit_f);
		Cmd_AddCommand("writeconfig", Com_WriteConfig_f);

		s = va("%s %s %s", Q3_VERSION, CPUSTRING, __DATE__);
		com_version = Cvar_Get("version", s, CVAR_ROM | CVAR_SERVERINFO);

		Sys_Init();
		Netchan_Init(Com_Milliseconds() & 0xffff);	// pick a port value that should be nice and random
		VM_Init();
		SVT3_Init();

		com_dedicated->modified = false;
		if (!com_dedicated->integer)
		{
			CL_Init();
			Sys_ShowConsole(com_viewlog->integer, false);
		}

		// set com_frameTime so that if a map is started on the
		// command line it will still be able to count on com_frameTime
		// being random enough for a serverid
		com_frameTime = Com_Milliseconds();

		// add + commands from command line
		if (!Com_AddStartupCommands())
		{
			// if the user didn't give any commands, run default action
		}

		// start in full screen ui mode
		Cvar_Set("r_uiFullScreen", "1");

		CL_StartHunkUsers();

		// delay this so potential wicked3d dll can find a wolf window
		if (!com_dedicated->integer)
		{
			Sys_ShowConsole(com_viewlog->integer, false);
		}

		// NERVE - SMF - force recommendedSet and don't do vid_restart if in safe mode
		if (!com_recommendedSet->integer && !safeMode)
		{
			Com_SetRecommended();
			Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
		}
		Cvar_Set("com_recommendedSet", "1");

		if (!com_dedicated->integer)
		{
			//Cvar_Set( "com_logosPlaying", "1" );
			Cbuf_AddText("cinematic etintro.roq\n");
			/*Cvar_Set( "nextmap", "cinematic avlogo.roq" );
			if( !com_introPlayed->integer ) {
			    Cvar_Set( com_introPlayed->name, "1" );
			    //Cvar_Set( "nextmap", "cinematic avlogo.roq" );
			}*/
		}

		com_fullyInitialized = true;
		fs_ProtectKeyFile = true;
		common->Printf("--- Common Initialization Complete ---\n");
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

//==================================================================

void Com_WriteConfigToFile(const char* filename)
{
	fileHandle_t f;

	f = FS_FOpenFileWrite(filename);
	if (!f)
	{
		common->Printf("Couldn't write %s.\n", filename);
		return;
	}

	FS_Printf(f, "// generated by ET, do not modify\n");
	Key_WriteBindings(f);
	Cvar_WriteVariables(f);
	FS_FCloseFile(f);
}


/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration(void)
{
#ifndef DEDICATED
	Cvar* fs;
#endif
	const char* cl_profileStr = Cvar_VariableString("cl_profile");

	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if (!com_fullyInitialized)
	{
		return;
	}

	if (!(cvar_modifiedFlags & CVAR_ARCHIVE))
	{
		return;
	}
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	if (comet_gameInfo.usesProfiles && cl_profileStr[0])
	{
		Com_WriteConfigToFile(va("profiles/%s/%s", cl_profileStr, CONFIG_NAME));
	}
	else
	{
		Com_WriteConfigToFile(CONFIG_NAME);
	}

#ifndef DEDICATED
	fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (UIT3_UsesUniqueCDKey() && fs && fs->string[0] != 0)
	{
		Com_WriteCDKey(fs->string, &cl_cdkey[16]);
	}
	else
	{
		Com_WriteCDKey(BASEGAME, cl_cdkey);
	}
#endif
}


/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f(void)
{
	char filename[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: writeconfig <filename>\n");
		return;
	}

	String::NCpyZ(filename, Cmd_Argv(1), sizeof(filename));
	String::DefaultExtension(filename, sizeof(filename), ".cfg");
	common->Printf("Writing %s.\n", filename);
	Com_WriteConfigToFile(filename);
}

/*
================
Com_ModifyMsec
================
*/
int Com_ModifyMsec(int msec)
{
	int clampTime;

	//
	// modify time for debugging values
	//
	if (com_fixedtime->integer)
	{
		msec = com_fixedtime->integer;
	}
	else if (com_timescale->value)
	{
		msec *= com_timescale->value;
//	} else if (com_cameraMode->integer) {
//		msec *= com_timescale->value;
	}

	// don't let it scale below 1 msec
	if (msec < 1 && com_timescale->value)
	{
		msec = 1;
	}

	if (com_dedicated->integer)
	{
		// dedicated servers don't want to clamp for a much longer
		// period, because it would mess up all the client's views
		// of time.
		if (msec > 500 && msec < 500000)
		{
			common->Printf("Hitch warning: %i msec frame time\n", msec);
		}
		clampTime = 5000;
	}
	else
	if (!com_sv_running->integer)
	{
		// clients of remote servers do not want to clamp time, because
		// it would skew their view of the server's time temporarily
		clampTime = 5000;
	}
	else
	{
		// for local single player gaming
		// we may want to clamp the time to prevent players from
		// flying off edges when something hitches.
		clampTime = 200;
	}

	if (msec > clampTime)
	{
		msec = clampTime;
	}

	return msec;
}

/*
=================
Com_Frame
=================
*/
void Com_Frame(void)
{
	try
	{
		int msec, minMsec;
		static int lastTime;
		int key;

		int timeBeforeFirstEvents;
		int timeBeforeServer;
		int timeBeforeEvents;
		int timeBeforeClient;
		int timeAfter;

		static int watchdogTime = 0;
		static qboolean watchWarn = false;

		if (setjmp(abortframe))
		{
			return;		// an ERR_DROP was thrown
		}

		// bk001204 - init to zero.
		//  also:  might be clobbered by `longjmp' or `vfork'
		timeBeforeFirstEvents = 0;
		timeBeforeServer = 0;
		timeBeforeEvents = 0;
		timeBeforeClient = 0;
		timeAfter = 0;


		// old net chan encryption key
		key = 0x87243987;

		// write config file if anything changed
		Com_WriteConfiguration();

		// if "viewlog" has been modified, show or hide the log console
		if (com_viewlog->modified)
		{
			if (!com_dedicated->value)
			{
				Sys_ShowConsole(com_viewlog->integer, false);
			}
			com_viewlog->modified = false;
		}

		//
		// main event loop
		//
		if (com_speeds->integer)
		{
			timeBeforeFirstEvents = Sys_Milliseconds();
		}

		// we may want to spin here if things are going too fast
		if (!com_dedicated->integer && com_maxfps->integer > 0 && !com_timedemo->integer)
		{
			minMsec = 1000 / com_maxfps->integer;
		}
		else
		{
			minMsec = 1;
		}
		do
		{
			com_frameTime = Com_EventLoop();
			if (lastTime > com_frameTime)
			{
				lastTime = com_frameTime;	// possible on first frame
			}
			msec = com_frameTime - lastTime;
		}
		while (msec < minMsec);
		Cbuf_Execute();

		lastTime = com_frameTime;

		// mess with msec if needed
		com_frameMsec = msec;
		msec = Com_ModifyMsec(msec);

		//
		// server side
		//
		if (com_speeds->integer)
		{
			timeBeforeServer = Sys_Milliseconds();
		}

		SVT3_Frame(msec);

		// if "dedicated" has been modified, start up
		// or shut down the client system.
		// Do this after the server may have started,
		// but before the client tries to auto-connect
		if (com_dedicated->modified)
		{
			// get the latched value
			Cvar_Get("dedicated", "0", 0);
			com_dedicated->modified = false;
			if (!com_dedicated->integer)
			{
				CL_Init();
				Sys_ShowConsole(com_viewlog->integer, false);
			}
			else
			{
				CL_Shutdown();
				Sys_ShowConsole(1, true);
			}
		}

		//
		// client system
		//
		if (!com_dedicated->integer)
		{
			//
			// run event loop a second time to get server to client packets
			// without a frame of latency
			//
			if (com_speeds->integer)
			{
				timeBeforeEvents = Sys_Milliseconds();
			}
			Com_EventLoop();
			Cbuf_Execute();

			//
			// client side
			//
			if (com_speeds->integer)
			{
				timeBeforeClient = Sys_Milliseconds();
			}

			CL_Frame(msec);

			if (com_speeds->integer)
			{
				timeAfter = Sys_Milliseconds();
			}
		}
		else
		{
			timeAfter = Sys_Milliseconds();
		}

		//
		// watchdog
		//
		if (com_dedicated->integer && !com_sv_running->integer && com_watchdog->integer)
		{
			if (watchdogTime == 0)
			{
				watchdogTime = Sys_Milliseconds();
			}
			else
			{
				if (!watchWarn && Sys_Milliseconds() - watchdogTime > (com_watchdog->integer - 4) * 1000)
				{
					common->Printf("WARNING: watchdog will trigger in 4 seconds\n");
					watchWarn = true;
				}
				else if (Sys_Milliseconds() - watchdogTime > com_watchdog->integer * 1000)
				{
					common->Printf("Idle Server with no map - triggering watchdog\n");
					watchdogTime = 0;
					watchWarn = false;
					if (com_watchdog_cmd->string[0] == '\0')
					{
						Cbuf_AddText("quit\n");
					}
					else
					{
						Cbuf_AddText(va("%s\n", com_watchdog_cmd->string));
					}
				}
			}
		}

		//
		// report timing information
		//
		if (com_speeds->integer)
		{
			int all, sv, sev, cev, cl;

			all = timeAfter - timeBeforeServer;
			sv = timeBeforeEvents - timeBeforeServer;
			sev = timeBeforeServer - timeBeforeFirstEvents;
			cev = timeBeforeClient - timeBeforeEvents;
			cl = timeAfter - timeBeforeClient;
			sv -= t3time_game;
			cl -= time_frontend + time_backend;

			common->Printf("frame:%i all:%3i sv:%3i sev:%3i cev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n",
				com_frameNumber, all, sv, sev, cev, cl, t3time_game, time_frontend, time_backend);
		}

		//
		// trace optimization tracking
		//
		if (com_showtrace->integer)
		{

			common->Printf("%4i traces  (%ib %ip) %4i points\n", c_traces,
				c_brush_traces, c_patch_traces, c_pointcontents);
			c_traces = 0;
			c_brush_traces = 0;
			c_patch_traces = 0;
			c_pointcontents = 0;
		}

		// old net chan encryption key
		key = lastTime * 0x87243987;

		com_frameNumber++;
	}
	catch (DropException& e)
	{
		Com_Error(ERR_DROP, "%s", e.What());
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown(qboolean badProfile)
{
	const char* cl_profileStr = Cvar_VariableString("cl_profile");


	// delete pid file
	if (comet_gameInfo.usesProfiles && cl_profileStr[0] && !badProfile)
	{
		if (FS_FileExists(va("profiles/%s/profile.pid", cl_profileStr)))
		{
			FS_Delete(va("profiles/%s/profile.pid", cl_profileStr));
		}
	}

	if (logfile)
	{
		FS_FCloseFile(logfile);
		logfile = 0;
	}

	if (com_journalFile)
	{
		FS_FCloseFile(com_journalFile);
		com_journalFile = 0;
	}
}

/*
===========================================
command line completion
===========================================
*/

void Com_GetHunkInfo(int* hunkused, int* hunkexpected)
{
	*hunkused =     0;
	*hunkexpected = -1;
}

void CL_Disconnect()
{
}
void SCRQH_BeginLoadingPlaque()
{
}
void CL_EstablishConnection(const char* name)
{
}
void Host_Reconnect_f()
{
}
void Cmd_ForwardToServer()
{
}
void CL_Drop()
{
}
void SCR_BeginLoadingPlaque(bool Clear)
{
}
