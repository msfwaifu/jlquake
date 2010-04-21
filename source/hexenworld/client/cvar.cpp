// cvar.c -- dynamic variable tracking

#ifdef SERVERONLY 
#include "qwsvdef.h"
#else
#include "quakedef.h"
#endif

void Cvar_Changed(QCvar* var)
{
#ifdef SERVERONLY
	if (var->flags & CVAR_SERVERINFO && var->name[0] != '*')
	{
		Info_SetValueForKey(svs.info, var->name, var->string, MAX_SERVERINFO_STRING,
			64, 64, !sv_highchars->value, false);
		SV_BroadcastCommand ("fullserverinfo \"%s\"\n", svs.info);
	}
#else
	if (var->flags & CVAR_USERINFO && var->name[0] != '*')
	{
		Info_SetValueForKey(cls.userinfo, var->name, var->string, MAX_INFO_STRING, 64, 64,
			QStr::ICmp(var->name, "name") != 0, QStr::ICmp(var->name, "team") == 0);
		if (cls.state >= ca_connected)
		{
			cls.netchan.message.WriteByte(clc_stringcmd);
			cls.netchan.message.WriteString2(va("setinfo \"%s\" \"%s\"\n", var->name, var->string));
		}
	}
#endif
}
