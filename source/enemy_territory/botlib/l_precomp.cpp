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

/*****************************************************************************
 * name:		l_precomp.c
 *
 * desc:		pre compiler
 *
 *
 *****************************************************************************/

//Notes:			fix: PC_StringizeTokens

#include "../game/q_shared.h"
#include "l_script.h"
#include "l_precomp.h"

#define MAX_DEFINEPARMS         128

//directive name with parse function
typedef struct directive_s
{
	const char* name;
	int (* func)(source_t* source);
} directive_t;

//list with global defines added to every source loaded
define_t* globaldefines;

//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_PushScript(source_t* source, script_t* script)
{
	script_t* s;

	for (s = source->scriptstack; s; s = s->next)
	{
		if (!String::ICmp(s->filename, script->filename))
		{
			SourceError(source, "%s recursively included", script->filename);
			return;
		}	//end if
	}	//end for
		//push the script on the script stack
	script->next = source->scriptstack;
	source->scriptstack = script;
}	//end of the function PC_PushScript
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpandDefineIntoSource(source_t* source, token_t* deftoken, define_t* define);
int PC_ReadDefineParms(source_t* source, define_t* define, token_t** parms, int maxparms)
{
	token_t token, * t, * last;
	int i, done, lastcomma, numparms, indent;
	define_t* newdefine;

	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "define %s missing parms", define->name);
		return qfalse;
	}	//end if
		//
	if (define->numparms > maxparms)
	{
		SourceError(source, "define with more than %d parameters", maxparms);
		return qfalse;
	}	//end if
		//
	for (i = 0; i < define->numparms; i++)
		parms[i] = NULL;
	//if no leading "("
	if (String::Cmp(token.string, "("))
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "define %s missing parms", define->name);
		return qfalse;
	}	//end if
		//read the define parameters
	for (done = 0, numparms = 0, indent = 1; !done; )
	{
		if (numparms >= maxparms)
		{
			SourceError(source, "define %s with too many parms", define->name);
			return qfalse;
		}	//end if
		if (numparms >= define->numparms)
		{
			SourceWarning(source, "define %s has too many parms", define->name);
			return qfalse;
		}	//end if
		parms[numparms] = NULL;
		lastcomma = 1;
		last = NULL;
		while (!done)
		{
			//
			if (!PC_ReadSourceToken(source, &token))
			{
				SourceError(source, "define %s incomplete", define->name);
				return qfalse;
			}	//end if
				//
			if (!String::Cmp(token.string, ","))
			{
				if (indent <= 1)
				{
					if (lastcomma)
					{
						SourceWarning(source, "too many comma's");
					}
					lastcomma = 1;
					break;
				}	//end if
			}	//end if
			lastcomma = 0;
			//
			if (!String::Cmp(token.string, "("))
			{
				indent++;
			}	//end if
			else if (!String::Cmp(token.string, ")"))
			{
				if (--indent <= 0)
				{
					if (!parms[define->numparms - 1])
					{
						SourceWarning(source, "too few define parms");
					}	//end if
					done = 1;
					break;
				}	// end if
			}	//end if
			else if (token.type == TT_NAME)
			{
				newdefine = PC_FindHashedDefine(source->definehash, token.string);
				if (newdefine)
				{
					if (!PC_ExpandDefineIntoSource(source, &token, newdefine))
					{
						return qfalse;
					}
					continue;
				}
			}	// end if
				//
			if (numparms < define->numparms)
			{
				//
				t = PC_CopyToken(&token);
				t->next = NULL;
				if (last)
				{
					last->next = t;
				}
				else
				{
					parms[numparms] = t;
				}
				last = t;
			}	//end if
		}	//end while
		numparms++;
	}	//end for
	return qtrue;
}	//end of the function PC_ReadDefineParms
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_StringizeTokens(token_t* tokens, token_t* token)
{
	token_t* t;

	token->type = TT_STRING;
	token->whitespace_p = NULL;
	token->endwhitespace_p = NULL;
	token->string[0] = '\0';
	strcat(token->string, "\"");
	for (t = tokens; t; t = t->next)
	{
		strncat(token->string, t->string, MAX_TOKEN - String::Length(token->string));
	}	//end for
	strncat(token->string, "\"", MAX_TOKEN - String::Length(token->string));
	return qtrue;
}	//end of the function PC_StringizeTokens
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_MergeTokens(token_t* t1, token_t* t2)
{
	//merging of a name with a name or number
	if (t1->type == TT_NAME && (t2->type == TT_NAME || t2->type == TT_NUMBER))
	{
		strcat(t1->string, t2->string);
		return qtrue;
	}	//end if
		//merging of two strings
	if (t1->type == TT_STRING && t2->type == TT_STRING)
	{
		//remove trailing double quote
		t1->string[String::Length(t1->string) - 1] = '\0';
		//concat without leading double quote
		strcat(t1->string, &t2->string[1]);
		return qtrue;
	}	//end if
		//FIXME: merging of two number of the same sub type
	return qfalse;
}	//end of the function PC_MergeTokens
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
define_t* PC_FindDefine(define_t* defines, char* name)
{
	define_t* d;

	for (d = defines; d; d = d->next)
	{
		if (!String::Cmp(d->name, name))
		{
			return d;
		}
	}	//end for
	return NULL;
}	//end of the function PC_FindDefine
//============================================================================
//
// Parameter:				-
// Returns:					number of the parm
//								if no parm found with the given name -1 is returned
// Changes Globals:		-
//============================================================================
int PC_FindDefineParm(define_t* define, char* name)
{
	token_t* p;
	int i;

	i = 0;
	for (p = define->parms; p; p = p->next)
	{
		if (!String::Cmp(p->string, name))
		{
			return i;
		}
		i++;
	}	//end for
	return -1;
}	//end of the function PC_FindDefineParm
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_AddBuiltinDefines(source_t* source)
{
	int i;
	define_t* define;
	struct _builtin
	{
		const char* string;
		int builtin;
	} builtin[] = {
		{ "__LINE__",    BUILTIN_LINE },
		{ "__FILE__",    BUILTIN_FILE },
		{ "__DATE__",    BUILTIN_DATE },
		{ "__TIME__",    BUILTIN_TIME },
//	{	"__STDC__", BUILTIN_STDC },
		{ NULL, 0 }
	};

	for (i = 0; builtin[i].string; i++)
	{
		define = (define_t*)Mem_Alloc(sizeof(define_t) + String::Length(builtin[i].string) + 1);
		memset(define, 0, sizeof(define_t));
		define->name = (char*)define + sizeof(define_t);
		String::Cpy(define->name, builtin[i].string);
		define->flags |= DEFINE_FIXED;
		define->builtin = builtin[i].builtin;
		//add the define to the source
		PC_AddDefineToHash(define, source->definehash);
	}	//end for
}	//end of the function PC_AddBuiltinDefines
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpandBuiltinDefine(source_t* source, token_t* deftoken, define_t* define,
	token_t** firsttoken, token_t** lasttoken)
{
	token_t* token;
	time_t t;	//to prevent LCC warning
	char* curtime;

	token = PC_CopyToken(deftoken);
	switch (define->builtin)
	{
	case BUILTIN_LINE:
	{
		sprintf(token->string, "%d", deftoken->line);
		token->intvalue = deftoken->line;
		token->floatvalue = deftoken->line;
		token->type = TT_NUMBER;
		token->subtype = TT_DECIMAL | TT_INTEGER;
		*firsttoken = token;
		*lasttoken = token;
		break;
	}		//end case
	case BUILTIN_FILE:
	{
		String::Cpy(token->string, source->scriptstack->filename);
		token->type = TT_NAME;
		token->subtype = String::Length(token->string);
		*firsttoken = token;
		*lasttoken = token;
		break;
	}		//end case
	case BUILTIN_DATE:
	{
		t = time(NULL);
		curtime = ctime(&t);
		String::Cpy(token->string, "\"");
		strncat(token->string, curtime + 4, 7);
		strncat(token->string + 7, curtime + 20, 4);
		strcat(token->string, "\"");
//			free(curtime);
		token->type = TT_NAME;
		token->subtype = String::Length(token->string);
		*firsttoken = token;
		*lasttoken = token;
		break;
	}		//end case
	case BUILTIN_TIME:
	{
		t = time(NULL);
		curtime = ctime(&t);
		String::Cpy(token->string, "\"");
		strncat(token->string, curtime + 11, 8);
		strcat(token->string, "\"");
//			free(curtime);
		token->type = TT_NAME;
		token->subtype = String::Length(token->string);
		*firsttoken = token;
		*lasttoken = token;
		break;
	}		//end case
	case BUILTIN_STDC:
	default:
	{
		*firsttoken = NULL;
		*lasttoken = NULL;
		break;
	}		//end case
	}	//end switch
	return qtrue;
}	//end of the function PC_ExpandBuiltinDefine
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpandDefine(source_t* source, token_t* deftoken, define_t* define,
	token_t** firsttoken, token_t** lasttoken)
{
	token_t* parms[MAX_DEFINEPARMS], * dt, * pt, * t;
	token_t* t1, * t2, * first, * last, * nextpt, token;
	int parmnum, i;

	//if it is a builtin define
	if (define->builtin)
	{
		return PC_ExpandBuiltinDefine(source, deftoken, define, firsttoken, lasttoken);
	}	//end if
		//if the define has parameters
	if (define->numparms)
	{
		if (!PC_ReadDefineParms(source, define, parms, MAX_DEFINEPARMS))
		{
			return qfalse;
		}
	}	//end if
		//empty list at first
	first = NULL;
	last = NULL;
	//create a list with tokens of the expanded define
	for (dt = define->tokens; dt; dt = dt->next)
	{
		parmnum = -1;
		//if the token is a name, it could be a define parameter
		if (dt->type == TT_NAME)
		{
			parmnum = PC_FindDefineParm(define, dt->string);
		}	//end if
			//if it is a define parameter
		if (parmnum >= 0)
		{
			for (pt = parms[parmnum]; pt; pt = pt->next)
			{
				t = PC_CopyToken(pt);
				//add the token to the list
				t->next = NULL;
				if (last)
				{
					last->next = t;
				}
				else
				{
					first = t;
				}
				last = t;
			}	//end for
		}	//end if
		else
		{
			//if stringizing operator_
			if (dt->string[0] == '#' && dt->string[1] == '\0')
			{
				//the stringizing operator_ must be followed by a define parameter
				if (dt->next)
				{
					parmnum = PC_FindDefineParm(define, dt->next->string);
				}
				else
				{
					parmnum = -1;
				}
				//
				if (parmnum >= 0)
				{
					//step over the stringizing operator_
					dt = dt->next;
					//stringize the define parameter tokens
					if (!PC_StringizeTokens(parms[parmnum], &token))
					{
						SourceError(source, "can't stringize tokens");
						return qfalse;
					}	//end if
					t = PC_CopyToken(&token);
				}	//end if
				else
				{
					SourceWarning(source, "stringizing operator_ without define parameter");
					continue;
				}	//end if
			}	//end if
			else
			{
				t = PC_CopyToken(dt);
			}	//end else
				//add the token to the list
			t->next = NULL;
			if (last)
			{
				last->next = t;
			}
			else
			{
				first = t;
			}
			last = t;
		}	//end else
	}	//end for
		//check for the merging operator_
	for (t = first; t; )
	{
		if (t->next)
		{
			//if the merging operator_
			if (t->next->string[0] == '#' && t->next->string[1] == '#')
			{
				t1 = t;
				t2 = t->next->next;
				if (t2)
				{
					if (!PC_MergeTokens(t1, t2))
					{
						SourceError(source, "can't merge %s with %s", t1->string, t2->string);
						return qfalse;
					}	//end if
					PC_FreeToken(t1->next);
					t1->next = t2->next;
					if (t2 == last)
					{
						last = t1;
					}
					PC_FreeToken(t2);
					continue;
				}	//end if
			}	//end if
		}	//end if
		t = t->next;
	}	//end for
		//store the first and last token of the list
	*firsttoken = first;
	*lasttoken = last;
	//free all the parameter tokens
	for (i = 0; i < define->numparms; i++)
	{
		for (pt = parms[i]; pt; pt = nextpt)
		{
			nextpt = pt->next;
			PC_FreeToken(pt);
		}	//end for
	}	//end for
		//
	return qtrue;
}	//end of the function PC_ExpandDefine
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpandDefineIntoSource(source_t* source, token_t* deftoken, define_t* define)
{
	token_t* firsttoken, * lasttoken;

	if (!PC_ExpandDefine(source, deftoken, define, &firsttoken, &lasttoken))
	{
		return qfalse;
	}

	if (firsttoken && lasttoken)
	{
		lasttoken->next = source->tokens;
		source->tokens = firsttoken;
		return qtrue;
	}	//end if
	return qfalse;
}	//end of the function PC_ExpandDefineIntoSource
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_ConvertPath(char* path)
{
	char* ptr;

	//remove double path seperators
	for (ptr = path; *ptr; )
	{
		if ((*ptr == '\\' || *ptr == '/') &&
			(*(ptr + 1) == '\\' || *(ptr + 1) == '/'))
		{
			memmove(ptr, ptr + 1, String::Length(ptr));
		}	//end if
		else
		{
			ptr++;
		}	//end else
	}	//end while
		//set OS dependent path seperators
	for (ptr = path; *ptr; )
	{
		if (*ptr == '/' || *ptr == '\\')
		{
			*ptr = PATHSEPERATOR_CHAR;
		}
		ptr++;
	}	//end while
}	//end of the function PC_ConvertPath
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_include(source_t* source)
{
	script_t* script;
	token_t token;
	char path[_MAX_PATH];

	if (source->skip > 0)
	{
		return qtrue;
	}
	//
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "#include without file name");
		return qfalse;
	}	//end if
	if (token.linescrossed > 0)
	{
		SourceError(source, "#include without file name");
		return qfalse;
	}	//end if
	if (token.type == TT_STRING)
	{
		StripDoubleQuotes(token.string);
		PC_ConvertPath(token.string);
		script = LoadScriptFile(token.string);
		if (!script)
		{
			String::Cpy(path, source->includepath);
			strcat(path, token.string);
			script = LoadScriptFile(path);
		}	//end if
	}	//end if
	else if (token.type == TT_PUNCTUATION && *token.string == '<')
	{
		String::Cpy(path, source->includepath);
		while (PC_ReadSourceToken(source, &token))
		{
			if (token.linescrossed > 0)
			{
				PC_UnreadSourceToken(source, &token);
				break;
			}	//end if
			if (token.type == TT_PUNCTUATION && *token.string == '>')
			{
				break;
			}
			strncat(path, token.string, _MAX_PATH);
		}	//end while
		if (*token.string != '>')
		{
			SourceWarning(source, "#include missing trailing >");
		}	//end if
		if (!String::Length(path))
		{
			SourceError(source, "#include without file name between < >");
			return qfalse;
		}	//end if
		PC_ConvertPath(path);
		script = LoadScriptFile(path);
	}	//end if
	else
	{
		SourceError(source, "#include without file name");
		return qfalse;
	}	//end else
	if (!script)
	{
		SourceError(source, "file %s not found", path);
		return qfalse;
	}	//end if
	PC_PushScript(source, script);
	return qtrue;
}	//end of the function PC_Directive_include
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_WhiteSpaceBeforeToken(token_t* token)
{
	return token->endwhitespace_p - token->whitespace_p > 0;
}	//end of the function PC_WhiteSpaceBeforeToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_ClearTokenWhiteSpace(token_t* token)
{
	token->whitespace_p = NULL;
	token->endwhitespace_p = NULL;
	token->linescrossed = 0;
}	//end of the function PC_ClearTokenWhiteSpace
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_define(source_t* source)
{
	token_t token, * t, * last;
	define_t* define;

	if (source->skip > 0)
	{
		return qtrue;
	}
	//
	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "#define without name");
		return qfalse;
	}	//end if
	if (token.type != TT_NAME)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "expected name after #define, found %s", token.string);
		return qfalse;
	}	//end if
		//check if the define already exists
	define = PC_FindHashedDefine(source->definehash, token.string);
	if (define)
	{
		if (define->flags & DEFINE_FIXED)
		{
			SourceError(source, "can't redefine %s", token.string);
			return qfalse;
		}	//end if
		SourceWarning(source, "redefinition of %s", token.string);
		//unread the define name before executing the #undef directive
		PC_UnreadSourceToken(source, &token);
		if (!PC_Directive_undef(source))
		{
			return qfalse;
		}
		//if the define was not removed (define->flags & DEFINE_FIXED)
		define = PC_FindHashedDefine(source->definehash, token.string);
	}	//end if
		//allocate define
	define = (define_t*)Mem_Alloc(sizeof(define_t) + String::Length(token.string) + 1);
	memset(define, 0, sizeof(define_t));
	define->name = (char*)define + sizeof(define_t);
	String::Cpy(define->name, token.string);
	//add the define to the source
	PC_AddDefineToHash(define, source->definehash);
		//if nothing is defined, just return
	if (!PC_ReadLine(source, &token))
	{
		return qtrue;
	}
	//if it is a define with parameters
	if (!PC_WhiteSpaceBeforeToken(&token) && !String::Cmp(token.string, "("))
	{
		//read the define parameters
		last = NULL;
		if (!PC_CheckTokenString(source, ")"))
		{
			while (1)
			{
				if (!PC_ReadLine(source, &token))
				{
					SourceError(source, "expected define parameter");
					return qfalse;
				}	//end if
					//if it isn't a name
				if (token.type != TT_NAME)
				{
					SourceError(source, "invalid define parameter");
					return qfalse;
				}	//end if
					//
				if (PC_FindDefineParm(define, token.string) >= 0)
				{
					SourceError(source, "two the same define parameters");
					return qfalse;
				}	//end if
					//add the define parm
				t = PC_CopyToken(&token);
				PC_ClearTokenWhiteSpace(t);
				t->next = NULL;
				if (last)
				{
					last->next = t;
				}
				else
				{
					define->parms = t;
				}
				last = t;
				define->numparms++;
				//read next token
				if (!PC_ReadLine(source, &token))
				{
					SourceError(source, "define parameters not terminated");
					return qfalse;
				}	//end if
					//
				if (!String::Cmp(token.string, ")"))
				{
					break;
				}
				//then it must be a comma
				if (String::Cmp(token.string, ","))
				{
					SourceError(source, "define not terminated");
					return qfalse;
				}	//end if
			}	//end while
		}	//end if
		if (!PC_ReadLine(source, &token))
		{
			return qtrue;
		}
	}	//end if
		//read the defined stuff
	last = NULL;
	do
	{
		t = PC_CopyToken(&token);
		if (t->type == TT_NAME && !String::Cmp(t->string, define->name))
		{
			SourceError(source, "recursive define (removed recursion)");
			continue;
		}	//end if
		PC_ClearTokenWhiteSpace(t);
		t->next = NULL;
		if (last)
		{
			last->next = t;
		}
		else
		{
			define->tokens = t;
		}
		last = t;
	}
	while (PC_ReadLine(source, &token));
	//
	if (last)
	{
		//check for merge operators at the beginning or end
		if (!String::Cmp(define->tokens->string, "##") ||
			!String::Cmp(last->string, "##"))
		{
			SourceError(source, "define with misplaced ##");
			return qfalse;
		}	//end if
	}	//end if
	return qtrue;
}	//end of the function PC_Directive_define
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
define_t* PC_DefineFromString(char* string)
{
	script_t* script;
	source_t src;
	token_t* t;
	int res, i;
	define_t* def;

	script = LoadScriptMemory(string, String::Length(string), "*extern");
	//create a new source
	memset(&src, 0, sizeof(source_t));
	String::NCpy(src.filename, "*extern", _MAX_PATH);
	src.scriptstack = script;
	src.definehash = (define_t**)Mem_ClearedAlloc(DEFINEHASHSIZE * sizeof(define_t*));
		//create a define from the source
	res = PC_Directive_define(&src);
	//free any tokens if left
	for (t = src.tokens; t; t = src.tokens)
	{
		src.tokens = src.tokens->next;
		PC_FreeToken(t);
	}	//end for
	def = NULL;
	for (i = 0; i < DEFINEHASHSIZE; i++)
	{
		if (src.definehash[i])
		{
			def = src.definehash[i];
			break;
		}	//end if
	}	//end for
		//
	Mem_Free(src.definehash);
		//
	FreeScript(script);
	//if the define was created succesfully
	if (res > 0)
	{
		return def;
	}
	//free the define if created
	if (src.defines)
	{
		PC_FreeDefine(def);
	}
	//
	return NULL;
}	//end of the function PC_DefineFromString
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_AddDefine(source_t* source, char* string)
{
	define_t* define;

	define = PC_DefineFromString(string);
	if (!define)
	{
		return qfalse;
	}
	PC_AddDefineToHash(define, source->definehash);
	return qtrue;
}	//end of the function PC_AddDefine
//============================================================================
// add a globals define that will be added to all opened sources
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_AddGlobalDefine(char* string)
{
	define_t* define;

	define = PC_DefineFromString(string);
	if (!define)
	{
		return qfalse;
	}
	define->next = globaldefines;
	globaldefines = define;
	return qtrue;
}	//end of the function PC_AddGlobalDefine
//============================================================================
// remove the given global define
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_RemoveGlobalDefine(char* name)
{
	define_t* define;

	define = PC_FindDefine(globaldefines, name);
	if (define)
	{
		PC_FreeDefine(define);
		return qtrue;
	}	//end if
	return qfalse;
}	//end of the function PC_RemoveGlobalDefine
//============================================================================
// remove all globals defines
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_RemoveAllGlobalDefines(void)
{
	define_t* define;

	for (define = globaldefines; define; define = globaldefines)
	{
		globaldefines = globaldefines->next;
		PC_FreeDefine(define);
	}	//end for

	globaldefines = NULL;
}	//end of the function PC_RemoveAllGlobalDefines
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_AddGlobalDefinesToSource(source_t* source)
{
	define_t* define, * newdefine;

	for (define = globaldefines; define; define = define->next)
	{
		newdefine = PC_CopyDefine(source, define);
		PC_AddDefineToHash(newdefine, source->definehash);
	}	//end for
}	//end of the function PC_AddGlobalDefinesToSource
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_if_def(source_t* source, int type)
{
	token_t token;
	define_t* d;
	int skip;

	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "#ifdef without name");
		return qfalse;
	}	//end if
	if (token.type != TT_NAME)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "expected name after #ifdef, found %s", token.string);
		return qfalse;
	}	//end if
	d = PC_FindHashedDefine(source->definehash, token.string);
	skip = (type == INDENT_IFDEF) == (d == NULL);
	PC_PushIndent(source, type, skip);
	return qtrue;
}	//end of the function PC_Directiveif_def
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_ifdef(source_t* source)
{
	return PC_Directive_if_def(source, INDENT_IFDEF);
}	//end of the function PC_Directive_ifdef
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_ifndef(source_t* source)
{
	return PC_Directive_if_def(source, INDENT_IFNDEF);
}	//end of the function PC_Directive_ifndef
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_else(source_t* source)
{
	int type, skip;

	PC_PopIndent(source, &type, &skip);
	if (!type)
	{
		SourceError(source, "misplaced #else");
		return qfalse;
	}	//end if
	if (type == INDENT_ELSE)
	{
		SourceError(source, "#else after #else");
		return qfalse;
	}	//end if
	PC_PushIndent(source, INDENT_ELSE, !skip);
	return qtrue;
}	//end of the function PC_Directive_else
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_endif(source_t* source)
{
	int type, skip;

	PC_PopIndent(source, &type, &skip);
	if (!type)
	{
		SourceError(source, "misplaced #endif");
		return qfalse;
	}	//end if
	return qtrue;
}	//end of the function PC_Directive_endif
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
typedef struct operator_s
{
	int operator_;
	int priority;
	int parentheses;
	struct operator_s* prev, * next;
} operator_t;

typedef struct value_s
{
	signed long int intvalue;
	double floatvalue;
	int parentheses;
	struct value_s* prev, * next;
} value_t;

int PC_OperatorPriority(int op)
{
	switch (op)
	{
	case P_MUL: return 15;
	case P_DIV: return 15;
	case P_MOD: return 15;
	case P_ADD: return 14;
	case P_SUB: return 14;

	case P_LOGIC_AND: return 7;
	case P_LOGIC_OR: return 6;
	case P_LOGIC_GEQ: return 12;
	case P_LOGIC_LEQ: return 12;
	case P_LOGIC_EQ: return 11;
	case P_LOGIC_UNEQ: return 11;

	case P_LOGIC_NOT: return 16;
	case P_LOGIC_GREATER: return 12;
	case P_LOGIC_LESS: return 12;

	case P_RSHIFT: return 13;
	case P_LSHIFT: return 13;

	case P_BIN_AND: return 10;
	case P_BIN_OR: return 8;
	case P_BIN_XOR: return 9;
	case P_BIN_NOT: return 16;

	case P_COLON: return 5;
	case P_QUESTIONMARK: return 5;
	}	//end switch
	return qfalse;
}	//end of the function PC_OperatorPriority

//#define AllocValue()			GetClearedMemory(sizeof(value_t));
//#define FreeValue(val)		FreeMemory(val)
//#define AllocOperator(op)		op = (operator_t *) GetClearedMemory(sizeof(operator_t));
//#define FreeOperator(op)		FreeMemory(op);

#define MAX_VALUES      64
#define MAX_OPERATORS   64
#define AllocValue(val)									\
	if (numvalues >= MAX_VALUES) {						\
		SourceError(source, "out of value space\n");	  \
		error = 1;										\
		break;											\
	}													\
	else { \
		val = &value_heap[numvalues++]; }
#define FreeValue(val)
//
#define AllocOperator(op)								\
	if (numoperators >= MAX_OPERATORS) {				\
		SourceError(source, "out of operator_ space\n");  \
		error = 1;										\
		break;											\
	}													\
	else { \
		op = &operator_heap[numoperators++]; }
#define FreeOperator(op)

int PC_EvaluateTokens(source_t* source, token_t* tokens, signed long int* intvalue,
	double* floatvalue, int integer)
{
	operator_t* o, * firstoperator, * lastoperator;
	value_t* v, * firstvalue, * lastvalue, * v1, * v2;
	token_t* t;
	int brace = 0;
	int parentheses = 0;
	int error = 0;
	int lastwasvalue = 0;
	int negativevalue = 0;
	int questmarkintvalue = 0;
	double questmarkfloatvalue = 0;
	int gotquestmarkvalue = qfalse;
	int lastoperatortype = 0;
	//
	operator_t operator_heap[MAX_OPERATORS];
	int numoperators = 0;
	value_t value_heap[MAX_VALUES];
	int numvalues = 0;

	firstoperator = lastoperator = NULL;
	firstvalue = lastvalue = NULL;
	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}
	for (t = tokens; t; t = t->next)
	{
		switch (t->type)
		{
		case TT_NAME:
		{
			if (lastwasvalue || negativevalue)
			{
				SourceError(source, "syntax error in #if/#elif");
				error = 1;
				break;
			}		//end if
			if (String::Cmp(t->string, "defined"))
			{
				SourceError(source, "undefined name %s in #if/#elif", t->string);
				error = 1;
				break;
			}		//end if
			t = t->next;
			if (!String::Cmp(t->string, "("))
			{
				brace = qtrue;
				t = t->next;
			}		//end if
			if (!t || t->type != TT_NAME)
			{
				SourceError(source, "defined without name in #if/#elif");
				error = 1;
				break;
			}		//end if
					//v = (value_t *) GetClearedMemory(sizeof(value_t));
			AllocValue(v);
			if (PC_FindHashedDefine(source->definehash, t->string))
			{
				v->intvalue = 1;
				v->floatvalue = 1;
			}		//end if
			else
			{
				v->intvalue = 0;
				v->floatvalue = 0;
			}		//end else
			v->parentheses = parentheses;
			v->next = NULL;
			v->prev = lastvalue;
			if (lastvalue)
			{
				lastvalue->next = v;
			}
			else
			{
				firstvalue = v;
			}
			lastvalue = v;
			if (brace)
			{
				t = t->next;
				if (!t || String::Cmp(t->string, ")"))
				{
					SourceError(source, "defined without ) in #if/#elif");
					error = 1;
					break;
				}		//end if
			}		//end if
			brace = qfalse;
			// defined() creates a value
			lastwasvalue = 1;
			break;
		}		//end case
		case TT_NUMBER:
		{
			if (lastwasvalue)
			{
				SourceError(source, "syntax error in #if/#elif");
				error = 1;
				break;
			}		//end if
					//v = (value_t *) GetClearedMemory(sizeof(value_t));
			AllocValue(v);
			if (negativevalue)
			{
				v->intvalue = -(signed int)t->intvalue;
				v->floatvalue = -t->floatvalue;
			}		//end if
			else
			{
				v->intvalue = t->intvalue;
				v->floatvalue = t->floatvalue;
			}		//end else
			v->parentheses = parentheses;
			v->next = NULL;
			v->prev = lastvalue;
			if (lastvalue)
			{
				lastvalue->next = v;
			}
			else
			{
				firstvalue = v;
			}
			lastvalue = v;
			//last token was a value
			lastwasvalue = 1;
			//
			negativevalue = 0;
			break;
		}		//end case
		case TT_PUNCTUATION:
		{
			if (negativevalue)
			{
				SourceError(source, "misplaced minus sign in #if/#elif");
				error = 1;
				break;
			}		//end if
			if (t->subtype == P_PARENTHESESOPEN)
			{
				parentheses++;
				break;
			}		//end if
			else if (t->subtype == P_PARENTHESESCLOSE)
			{
				parentheses--;
				if (parentheses < 0)
				{
					SourceError(source, "too many ) in #if/#elsif");
					error = 1;
				}		//end if
				break;
			}		//end else if
					//check for invalid operators on floating point values
			if (!integer)
			{
				if (t->subtype == P_BIN_NOT || t->subtype == P_MOD ||
					t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
					t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
					t->subtype == P_BIN_XOR)
				{
					SourceError(source, "illigal operator_ %s on floating point operands\n", t->string);
					error = 1;
					break;
				}		//end if
			}		//end if
			switch (t->subtype)
			{
			case P_LOGIC_NOT:
			case P_BIN_NOT:
			{
				if (lastwasvalue)
				{
					SourceError(source, "! or ~ after value in #if/#elif");
					error = 1;
					break;
				}			//end if
				break;
			}			//end case
			case P_INC:
			case P_DEC:
			{
				SourceError(source, "++ or -- used in #if/#elif");
				break;
			}			//end case
			case P_SUB:
			{
				if (!lastwasvalue)
				{
					negativevalue = 1;
					break;
				}			//end if
			}			//end case

			case P_MUL:
			case P_DIV:
			case P_MOD:
			case P_ADD:

			case P_LOGIC_AND:
			case P_LOGIC_OR:
			case P_LOGIC_GEQ:
			case P_LOGIC_LEQ:
			case P_LOGIC_EQ:
			case P_LOGIC_UNEQ:

			case P_LOGIC_GREATER:
			case P_LOGIC_LESS:

			case P_RSHIFT:
			case P_LSHIFT:

			case P_BIN_AND:
			case P_BIN_OR:
			case P_BIN_XOR:

			case P_COLON:
			case P_QUESTIONMARK:
			{
				if (!lastwasvalue)
				{
					SourceError(source, "operator_ %s after operator_ in #if/#elif", t->string);
					error = 1;
					break;
				}			//end if
				break;
			}			//end case
			default:
			{
				SourceError(source, "invalid operator_ %s in #if/#elif", t->string);
				error = 1;
				break;
			}			//end default
			}		//end switch
			if (!error && !negativevalue)
			{
				//o = (operator_t *) GetClearedMemory(sizeof(operator_t));
				AllocOperator(o);
				o->operator_ = t->subtype;
				o->priority = PC_OperatorPriority(t->subtype);
				o->parentheses = parentheses;
				o->next = NULL;
				o->prev = lastoperator;
				if (lastoperator)
				{
					lastoperator->next = o;
				}
				else
				{
					firstoperator = o;
				}
				lastoperator = o;
				lastwasvalue = 0;
			}		//end if
			break;
		}		//end case
		default:
		{
			SourceError(source, "unknown %s in #if/#elif", t->string);
			error = 1;
			break;
		}		//end default
		}	//end switch
		if (error)
		{
			break;
		}
	}	//end for
	if (!error)
	{
		if (!lastwasvalue)
		{
			SourceError(source, "trailing operator_ in #if/#elif");
			error = 1;
		}	//end if
		else if (parentheses)
		{
			SourceError(source, "too many ( in #if/#elif");
			error = 1;
		}	//end else if
	}	//end if
		//
	gotquestmarkvalue = qfalse;
	questmarkintvalue = 0;
	questmarkfloatvalue = 0;
	//while there are operators
	while (!error && firstoperator)
	{
		v = firstvalue;
		for (o = firstoperator; o->next; o = o->next)
		{
			//if the current operator_ is nested deeper in parentheses
			//than the next operator_
			if (o->parentheses > o->next->parentheses)
			{
				break;
			}
			//if the current and next operator_ are nested equally deep in parentheses
			if (o->parentheses == o->next->parentheses)
			{
				//if the priority of the current operator_ is equal or higher
				//than the priority of the next operator_
				if (o->priority >= o->next->priority)
				{
					break;
				}
			}	//end if
				//if the arity of the operator_ isn't equal to 1
			if (o->operator_ != P_LOGIC_NOT &&
				o->operator_ != P_BIN_NOT)
			{
				v = v->next;
			}
			//if there's no value or no next value
			if (!v)
			{
				SourceError(source, "mising values in #if/#elif");
				error = 1;
				break;
			}	//end if
		}	//end for
		if (error)
		{
			break;
		}
		v1 = v;
		v2 = v->next;
		switch (o->operator_)
		{
		case P_LOGIC_NOT:       v1->intvalue = !v1->intvalue;
			v1->floatvalue = !v1->floatvalue; break;
		case P_BIN_NOT:         v1->intvalue = ~v1->intvalue;
			break;
		case P_MUL:             v1->intvalue *= v2->intvalue;
			v1->floatvalue *= v2->floatvalue; break;
		case P_DIV:             if (!v2->intvalue || !v2->floatvalue)
			{
				SourceError(source, "divide by zero in #if/#elif\n");
				error = 1;
				break;
			}
			v1->intvalue /= v2->intvalue;
			v1->floatvalue /= v2->floatvalue; break;
		case P_MOD:             if (!v2->intvalue)
			{
				SourceError(source, "divide by zero in #if/#elif\n");
				error = 1;
				break;
			}
			v1->intvalue %= v2->intvalue; break;
		case P_ADD:             v1->intvalue += v2->intvalue;
			v1->floatvalue += v2->floatvalue; break;
		case P_SUB:             v1->intvalue -= v2->intvalue;
			v1->floatvalue -= v2->floatvalue; break;
		case P_LOGIC_AND:       v1->intvalue = v1->intvalue && v2->intvalue;
			v1->floatvalue = v1->floatvalue && v2->floatvalue; break;
		case P_LOGIC_OR:        v1->intvalue = v1->intvalue || v2->intvalue;
			v1->floatvalue = v1->floatvalue || v2->floatvalue; break;
		case P_LOGIC_GEQ:       v1->intvalue = v1->intvalue >= v2->intvalue;
			v1->floatvalue = v1->floatvalue >= v2->floatvalue; break;
		case P_LOGIC_LEQ:       v1->intvalue = v1->intvalue <= v2->intvalue;
			v1->floatvalue = v1->floatvalue <= v2->floatvalue; break;
		case P_LOGIC_EQ:        v1->intvalue = v1->intvalue == v2->intvalue;
			v1->floatvalue = v1->floatvalue == v2->floatvalue; break;
		case P_LOGIC_UNEQ:      v1->intvalue = v1->intvalue != v2->intvalue;
			v1->floatvalue = v1->floatvalue != v2->floatvalue; break;
		case P_LOGIC_GREATER:   v1->intvalue = v1->intvalue > v2->intvalue;
			v1->floatvalue = v1->floatvalue > v2->floatvalue; break;
		case P_LOGIC_LESS:      v1->intvalue = v1->intvalue < v2->intvalue;
			v1->floatvalue = v1->floatvalue < v2->floatvalue; break;
		case P_RSHIFT:          v1->intvalue >>= v2->intvalue;
			break;
		case P_LSHIFT:          v1->intvalue <<= v2->intvalue;
			break;
		case P_BIN_AND:         v1->intvalue &= v2->intvalue;
			break;
		case P_BIN_OR:          v1->intvalue |= v2->intvalue;
			break;
		case P_BIN_XOR:         v1->intvalue ^= v2->intvalue;
			break;
		case P_COLON:
		{
			if (!gotquestmarkvalue)
			{
				SourceError(source, ": without ? in #if/#elif");
				error = 1;
				break;
			}		//end if
			if (integer)
			{
				if (!questmarkintvalue)
				{
					v1->intvalue = v2->intvalue;
				}
			}		//end if
			else
			{
				if (!questmarkfloatvalue)
				{
					v1->floatvalue = v2->floatvalue;
				}
			}		//end else
			gotquestmarkvalue = qfalse;
			break;
		}		//end case
		case P_QUESTIONMARK:
		{
			if (gotquestmarkvalue)
			{
				SourceError(source, "? after ? in #if/#elif");
				error = 1;
				break;
			}		//end if
			questmarkintvalue = v1->intvalue;
			questmarkfloatvalue = v1->floatvalue;
			gotquestmarkvalue = qtrue;
			break;
		}		//end if
		}	//end switch
		if (error)
		{
			break;
		}
		lastoperatortype = o->operator_;
		//if not an operator_ with arity 1
		if (o->operator_ != P_LOGIC_NOT &&
			o->operator_ != P_BIN_NOT)
		{
			//remove the second value if not question mark operator_
			if (o->operator_ != P_QUESTIONMARK)
			{
				v = v->next;
			}
			//
			if (v->prev)
			{
				v->prev->next = v->next;
			}
			else
			{
				firstvalue = v->next;
			}
			if (v->next)
			{
				v->next->prev = v->prev;
			}
			else
			{
				lastvalue = v->prev;
			}
			//FreeMemory(v);
			FreeValue(v);
		}	//end if
			//remove the operator_
		if (o->prev)
		{
			o->prev->next = o->next;
		}
		else
		{
			firstoperator = o->next;
		}
		if (o->next)
		{
			o->next->prev = o->prev;
		}
		else
		{
			lastoperator = o->prev;
		}
		//FreeMemory(o);
		FreeOperator(o);
	}	//end while
	if (firstvalue)
	{
		if (intvalue)
		{
			*intvalue = firstvalue->intvalue;
		}
		if (floatvalue)
		{
			*floatvalue = firstvalue->floatvalue;
		}
	}	//end if
	for (o = firstoperator; o; o = lastoperator)
	{
		lastoperator = o->next;
		//FreeMemory(o);
		FreeOperator(o);
	}	//end for
	for (v = firstvalue; v; v = lastvalue)
	{
		lastvalue = v->next;
		//FreeMemory(v);
		FreeValue(v);
	}	//end for
	if (!error)
	{
		return qtrue;
	}
	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}
	return qfalse;
}	//end of the function PC_EvaluateTokens
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Evaluate(source_t* source, signed long int* intvalue,
	double* floatvalue, int integer)
{
	token_t token, * firsttoken, * lasttoken;
	token_t* t, * nexttoken;
	define_t* define;
	int defined = qfalse;

	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}
	//
	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "no value after #if/#elif");
		return qfalse;
	}	//end if
	firsttoken = NULL;
	lasttoken = NULL;
	do
	{
		//if the token is a name
		if (token.type == TT_NAME)
		{
			if (defined)
			{
				defined = qfalse;
				t = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}	//end if
			else if (!String::Cmp(token.string, "defined"))
			{
				defined = qtrue;
				t = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}	//end if
			else
			{
				//then it must be a define
				define = PC_FindHashedDefine(source->definehash, token.string);
				if (!define)
				{
					SourceError(source, "can't evaluate %s, not defined", token.string);
					return qfalse;
				}	//end if
				if (!PC_ExpandDefineIntoSource(source, &token, define))
				{
					return qfalse;
				}
			}	//end else
		}	//end if
			//if the token is a number or a punctuation
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION)
		{
			t = PC_CopyToken(&token);
			t->next = NULL;
			if (lasttoken)
			{
				lasttoken->next = t;
			}
			else
			{
				firsttoken = t;
			}
			lasttoken = t;
		}	//end else
		else//can't evaluate the token
		{
			SourceError(source, "can't evaluate %s", token.string);
			return qfalse;
		}	//end else
	}
	while (PC_ReadLine(source, &token));
	//
	if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer))
	{
		return qfalse;
	}
	//
	for (t = firsttoken; t; t = nexttoken)
	{
		nexttoken = t->next;
		PC_FreeToken(t);
	}	//end for
		//
	return qtrue;
}	//end of the function PC_Evaluate
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_DollarEvaluate(source_t* source, signed long int* intvalue,
	double* floatvalue, int integer)
{
	int indent, defined = qfalse;
	token_t token, * firsttoken, * lasttoken;
	token_t* t, * nexttoken;
	define_t* define;

	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}
	//
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "no leading ( after $evalint/$evalfloat");
		return qfalse;
	}	//end if
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "nothing to evaluate");
		return qfalse;
	}	//end if
	indent = 1;
	firsttoken = NULL;
	lasttoken = NULL;
	do
	{
		//if the token is a name
		if (token.type == TT_NAME)
		{
			if (defined)
			{
				defined = qfalse;
				t = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}	//end if
			else if (!String::Cmp(token.string, "defined"))
			{
				defined = qtrue;
				t = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}	//end if
			else
			{
				//then it must be a define
				define = PC_FindHashedDefine(source->definehash, token.string);
				if (!define)
				{
					SourceError(source, "can't evaluate %s, not defined", token.string);
					return qfalse;
				}	//end if
				if (!PC_ExpandDefineIntoSource(source, &token, define))
				{
					return qfalse;
				}
			}	//end else
		}	//end if
			//if the token is a number or a punctuation
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION)
		{
			if (*token.string == '(')
			{
				indent++;
			}
			else if (*token.string == ')')
			{
				indent--;
			}
			if (indent <= 0)
			{
				break;
			}
			t = PC_CopyToken(&token);
			t->next = NULL;
			if (lasttoken)
			{
				lasttoken->next = t;
			}
			else
			{
				firsttoken = t;
			}
			lasttoken = t;
		}	//end else
		else//can't evaluate the token
		{
			SourceError(source, "can't evaluate %s", token.string);
			return qfalse;
		}	//end else
	}
	while (PC_ReadSourceToken(source, &token));
	//
	if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer))
	{
		return qfalse;
	}
	//
	for (t = firsttoken; t; t = nexttoken)
	{
		nexttoken = t->next;
		PC_FreeToken(t);
	}	//end for
		//
	return qtrue;
}	//end of the function PC_DollarEvaluate
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_elif(source_t* source)
{
	signed long int value;
	int type, skip;

	PC_PopIndent(source, &type, &skip);
	if (!type || type == INDENT_ELSE)
	{
		SourceError(source, "misplaced #elif");
		return qfalse;
	}	//end if
	if (!PC_Evaluate(source, &value, NULL, qtrue))
	{
		return qfalse;
	}
	skip = (value == 0);
	PC_PushIndent(source, INDENT_ELIF, skip);
	return qtrue;
}	//end of the function PC_Directive_elif
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_if(source_t* source)
{
	signed long int value;
	int skip;

	if (!PC_Evaluate(source, &value, NULL, qtrue))
	{
		return qfalse;
	}
	skip = (value == 0);
	PC_PushIndent(source, INDENT_IF, skip);
	return qtrue;
}	//end of the function PC_Directive
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_line(source_t* source)
{
	SourceError(source, "#line directive not supported");
	return qfalse;
}	//end of the function PC_Directive_line
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_error(source_t* source)
{
	token_t token;

	String::Cpy(token.string, "");
	PC_ReadSourceToken(source, &token);
	SourceError(source, "#error directive: %s", token.string);
	return qfalse;
}	//end of the function PC_Directive_error
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_pragma(source_t* source)
{
	token_t token;

	SourceWarning(source, "#pragma directive not supported");
	while (PC_ReadLine(source, &token))
		;
	return qtrue;
}	//end of the function PC_Directive_pragma
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void UnreadSignToken(source_t* source)
{
	token_t token;

	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	String::Cpy(token.string, "-");
	token.type = TT_PUNCTUATION;
	token.subtype = P_SUB;
	PC_UnreadSourceToken(source, &token);
}	//end of the function UnreadSignToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_eval(source_t* source)
{
	signed long int value;
	token_t token;

	if (!PC_Evaluate(source, &value, NULL, qtrue))
	{
		return qfalse;
	}
	//
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(token.string, "%d", abs(value));
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return qtrue;
}	//end of the function PC_Directive_eval
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Directive_evalfloat(source_t* source)
{
	double value;
	token_t token;

	if (!PC_Evaluate(source, NULL, &value, qfalse))
	{
		return qfalse;
	}
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(token.string, "%1.2f", Q_fabs(value));
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return qtrue;
}	//end of the function PC_Directive_evalfloat
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
directive_t directives[20] =
{
	{"if", PC_Directive_if},
	{"ifdef", PC_Directive_ifdef},
	{"ifndef", PC_Directive_ifndef},
	{"elif", PC_Directive_elif},
	{"else", PC_Directive_else},
	{"endif", PC_Directive_endif},
	{"include", PC_Directive_include},
	{"define", PC_Directive_define},
	{"undef", PC_Directive_undef},
	{"line", PC_Directive_line},
	{"error", PC_Directive_error},
	{"pragma", PC_Directive_pragma},
	{"eval", PC_Directive_eval},
	{"evalfloat", PC_Directive_evalfloat},
	{NULL, NULL}
};

int PC_ReadDirective(source_t* source)
{
	token_t token;
	int i;

	//read the directive name
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "found # without name");
		return qfalse;
	}	//end if
		//directive name must be on the same line
	if (token.linescrossed > 0)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "found # at end of line");
		return qfalse;
	}	//end if
		//if if is a name
	if (token.type == TT_NAME)
	{
		//find the precompiler directive
		for (i = 0; directives[i].name; i++)
		{
			if (!String::Cmp(directives[i].name, token.string))
			{
				return directives[i].func(source);
			}	//end if
		}	//end for
	}	//end if
	SourceError(source, "unknown precompiler directive %s", token.string);
	return qfalse;
}	//end of the function PC_ReadDirective
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_DollarDirective_evalint(source_t* source)
{
	signed long int value;
	token_t token;

	if (!PC_DollarEvaluate(source, &value, NULL, qtrue))
	{
		return qfalse;
	}
	//
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(token.string, "%d", abs(value));
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
	token.intvalue = value;
	token.floatvalue = value;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return qtrue;
}	//end of the function PC_DollarDirective_evalint
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_DollarDirective_evalfloat(source_t* source)
{
	double value;
	token_t token;

	if (!PC_DollarEvaluate(source, NULL, &value, qfalse))
	{
		return qfalse;
	}
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(token.string, "%1.2f", Q_fabs(value));
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	token.intvalue = (unsigned long)value;
	token.floatvalue = value;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return qtrue;
}	//end of the function PC_DollarDirective_evalfloat
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
directive_t dollardirectives[20] =
{
	{"evalint", PC_DollarDirective_evalint},
	{"evalfloat", PC_DollarDirective_evalfloat},
	{NULL, NULL}
};

int PC_ReadDollarDirective(source_t* source)
{
	token_t token;
	int i;

	//read the directive name
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "found $ without name");
		return qfalse;
	}	//end if
		//directive name must be on the same line
	if (token.linescrossed > 0)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "found $ at end of line");
		return qfalse;
	}	//end if
		//if if is a name
	if (token.type == TT_NAME)
	{
		//find the precompiler directive
		for (i = 0; dollardirectives[i].name; i++)
		{
			if (!String::Cmp(dollardirectives[i].name, token.string))
			{
				return dollardirectives[i].func(source);
			}	//end if
		}	//end for
	}	//end if
	PC_UnreadSourceToken(source, &token);
	SourceError(source, "unknown precompiler directive %s", token.string);
	return qfalse;
}	//end of the function PC_ReadDirective

//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ReadToken(source_t* source, token_t* token)
{
	define_t* define;

	while (1)
	{
		if (!PC_ReadSourceToken(source, token))
		{
			return qfalse;
		}
		//check for precompiler directives
		if (token->type == TT_PUNCTUATION && *token->string == '#')
		{
			//read the precompiler directive
			if (!PC_ReadDirective(source))
			{
				return qfalse;
			}
			continue;
		}	//end if
		if (token->type == TT_PUNCTUATION && *token->string == '$')
		{
			//read the precompiler directive
			if (!PC_ReadDollarDirective(source))
			{
				return qfalse;
			}
			continue;
		}	//end if
			//if skipping source because of conditional compilation
		if (source->skip)
		{
			continue;
		}
		// recursively concatenate strings that are behind each other
		if (token->type == TT_STRING)
		{
			token_t newtoken;
			if (PC_ReadToken(source, &newtoken))
			{
				if (newtoken.type == TT_STRING)
				{
					token->string[String::Length(token->string) - 1] = '\0';
					if (String::Length(token->string) + String::Length(newtoken.string + 1) + 1 >= MAX_TOKEN)
					{
						SourceError(source, "string longer than MAX_TOKEN %d\n", MAX_TOKEN);
						return qfalse;
					}
					strcat(token->string, newtoken.string + 1);
				}
				else
				{
					PC_UnreadToken(source, &newtoken);
				}
			}
		}	//end if
			//if the token is a name
		if (token->type == TT_NAME)
		{
			//check if the name is a define macro
			define = PC_FindHashedDefine(source->definehash, token->string);
			//if it is a define macro
			if (define)
			{
				//expand the defined macro
				if (!PC_ExpandDefineIntoSource(source, token, define))
				{
					return qfalse;
				}
				continue;
			}	//end if
		}	//end if
			//copy token for unreading
		memcpy(&source->token, token, sizeof(token_t));
		//found a token
		return qtrue;
	}	//end while
}	//end of the function PC_ReadToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpectTokenString(source_t* source, const char* string)
{
	token_t token;

	if (!PC_ReadToken(source, &token))
	{
		SourceError(source, "couldn't find expected %s", string);
		return qfalse;
	}	//end if

	if (String::Cmp(token.string, string))
	{
		SourceError(source, "expected %s, found %s", string, token.string);
		return qfalse;
	}	//end if
	return qtrue;
}	//end of the function PC_ExpectTokenString
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpectTokenType(source_t* source, int type, int subtype, token_t* token)
{
	char str[MAX_TOKEN];

	if (!PC_ReadToken(source, token))
	{
		SourceError(source, "couldn't read expected token");
		return qfalse;
	}	//end if

	if (token->type != type)
	{
		String::Cpy(str, "");
		if (type == TT_STRING)
		{
			String::Cpy(str, "string");
		}
		if (type == TT_LITERAL)
		{
			String::Cpy(str, "literal");
		}
		if (type == TT_NUMBER)
		{
			String::Cpy(str, "number");
		}
		if (type == TT_NAME)
		{
			String::Cpy(str, "name");
		}
		if (type == TT_PUNCTUATION)
		{
			String::Cpy(str, "punctuation");
		}
		SourceError(source, "expected a %s, found %s", str, token->string);
		return qfalse;
	}	//end if
	if (token->type == TT_NUMBER)
	{
		if ((token->subtype & subtype) != subtype)
		{
			if (subtype & TT_DECIMAL)
			{
				String::Cpy(str, "decimal");
			}
			if (subtype & TT_HEX)
			{
				String::Cpy(str, "hex");
			}
			if (subtype & TT_OCTAL)
			{
				String::Cpy(str, "octal");
			}
			if (subtype & TT_BINARY)
			{
				String::Cpy(str, "binary");
			}
			if (subtype & TT_LONG)
			{
				strcat(str, " long");
			}
			if (subtype & TT_UNSIGNED)
			{
				strcat(str, " unsigned");
			}
			if (subtype & TT_FLOAT)
			{
				strcat(str, " float");
			}
			if (subtype & TT_INTEGER)
			{
				strcat(str, " integer");
			}
			SourceError(source, "expected %s, found %s", str, token->string);
			return qfalse;
		}	//end if
	}	//end if
	else if (token->type == TT_PUNCTUATION)
	{
		if (token->subtype != subtype)
		{
			SourceError(source, "found %s", token->string);
			return qfalse;
		}	//end if
	}	//end else if
	return qtrue;
}	//end of the function PC_ExpectTokenType
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpectAnyToken(source_t* source, token_t* token)
{
	if (!PC_ReadToken(source, token))
	{
		SourceError(source, "couldn't read expected token");
		return qfalse;
	}	//end if
	else
	{
		return qtrue;
	}	//end else
}	//end of the function PC_ExpectAnyToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_CheckTokenString(source_t* source, const char* string)
{
	token_t tok;

	if (!PC_ReadToken(source, &tok))
	{
		return qfalse;
	}
	//if the token is available
	if (!String::Cmp(tok.string, string))
	{
		return qtrue;
	}
	//
	PC_UnreadSourceToken(source, &tok);
	return qfalse;
}	//end of the function PC_CheckTokenString
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_CheckTokenType(source_t* source, int type, int subtype, token_t* token)
{
	token_t tok;

	if (!PC_ReadToken(source, &tok))
	{
		return qfalse;
	}
	//if the type matches
	if (tok.type == type &&
		(tok.subtype & subtype) == subtype)
	{
		memcpy(token, &tok, sizeof(token_t));
		return qtrue;
	}	//end if
		//
	PC_UnreadSourceToken(source, &tok);
	return qfalse;
}	//end of the function PC_CheckTokenType
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_SkipUntilString(source_t* source, char* string)
{
	token_t token;

	while (PC_ReadToken(source, &token))
	{
		if (!String::Cmp(token.string, string))
		{
			return qtrue;
		}
	}	//end while
	return qfalse;
}	//end of the function PC_SkipUntilString
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_UnreadLastToken(source_t* source)
{
	PC_UnreadSourceToken(source, &source->token);
}	//end of the function PC_UnreadLastToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_UnreadToken(source_t* source, token_t* token)
{
	PC_UnreadSourceToken(source, token);
}	//end of the function PC_UnreadToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_SetIncludePath(source_t* source, char* path)
{
	String::NCpy(source->includepath, path, MAX_QPATH);
	//add trailing path seperator
	if (source->includepath[String::Length(source->includepath) - 1] != '\\' &&
		source->includepath[String::Length(source->includepath) - 1] != '/')
	{
		strcat(source->includepath, PATHSEPERATOR_STR);
	}	//end if
}	//end of the function PC_SetIncludePath
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_SetPunctuations(source_t* source, punctuation_t* p)
{
	source->punctuations = p;
}	//end of the function PC_SetPunctuations
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
source_t* LoadSourceFile(const char* filename)
{
	source_t* source;
	script_t* script;

	script = LoadScriptFile(filename);
	if (!script)
	{
		return NULL;
	}

	script->next = NULL;

	source = (source_t*)Mem_Alloc(sizeof(source_t));
	memset(source, 0, sizeof(source_t));

	String::NCpy(source->filename, filename, MAX_QPATH);
	source->scriptstack = script;
	source->tokens = NULL;
	source->defines = NULL;
	source->indentstack = NULL;
	source->skip = 0;

	source->definehash = (define_t**)Mem_ClearedAlloc(DEFINEHASHSIZE * sizeof(define_t*));
	PC_AddGlobalDefinesToSource(source);
	return source;
}	//end of the function LoadSourceFile
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
source_t* LoadSourceMemory(char* ptr, int length, char* name)
{
	source_t* source;
	script_t* script;

	script = LoadScriptMemory(ptr, length, name);
	if (!script)
	{
		return NULL;
	}
	script->next = NULL;

	source = (source_t*)Mem_Alloc(sizeof(source_t));
	memset(source, 0, sizeof(source_t));

	String::NCpy(source->filename, name, MAX_QPATH);
	source->scriptstack = script;
	source->tokens = NULL;
	source->defines = NULL;
	source->indentstack = NULL;
	source->skip = 0;

	source->definehash = (define_t**)Mem_ClearedAlloc(DEFINEHASHSIZE * sizeof(define_t*));
	PC_AddGlobalDefinesToSource(source);
	return source;
}	//end of the function LoadSourceMemory
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void FreeSource(source_t* source)
{
	script_t* script;
	token_t* token;
	define_t* define;
	indent_t* indent;
	int i;

	//free all the scripts
	while (source->scriptstack)
	{
		script = source->scriptstack;
		source->scriptstack = source->scriptstack->next;
		FreeScript(script);
	}	//end for
		//free all the tokens
	while (source->tokens)
	{
		token = source->tokens;
		source->tokens = source->tokens->next;
		PC_FreeToken(token);
	}	//end for
	for (i = 0; i < DEFINEHASHSIZE; i++)
	{
		while (source->definehash[i])
		{
			define = source->definehash[i];
			source->definehash[i] = source->definehash[i]->hashnext;
			PC_FreeDefine(define);
		}	//end while
	}	//end for
		//free all indents
	while (source->indentstack)
	{
		indent = source->indentstack;
		source->indentstack = source->indentstack->next;
		Mem_Free(indent);
	}	//end for
	//
	if (source->definehash)
	{
		Mem_Free(source->definehash);
	}
		//free the source itself
	Mem_Free(source);
}	//end of the function FreeSource
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================

#define MAX_SOURCEFILES     64

source_t* sourceFiles[MAX_SOURCEFILES];

int PC_LoadSourceHandle(const char* filename)
{
	source_t* source;
	int i;

	for (i = 1; i < MAX_SOURCEFILES; i++)
	{
		if (!sourceFiles[i])
		{
			break;
		}
	}	//end for
	if (i >= MAX_SOURCEFILES)
	{
		return 0;
	}
	PS_SetBaseFolder("");
	source = LoadSourceFile(filename);
	if (!source)
	{
		return 0;
	}
	sourceFiles[i] = source;
	return i;
}	//end of the function PC_LoadSourceHandle
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
int PC_FreeSourceHandle(int handle)
{
	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return qfalse;
	}
	if (!sourceFiles[handle])
	{
		return qfalse;
	}

	FreeSource(sourceFiles[handle]);
	sourceFiles[handle] = NULL;
	return qtrue;
}	//end of the function PC_FreeSourceHandle
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
int PC_ReadTokenHandle(int handle, etpc_token_t* pc_token)
{
	token_t token;
	int ret;

	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return 0;
	}
	if (!sourceFiles[handle])
	{
		return 0;
	}

	ret = PC_ReadToken(sourceFiles[handle], &token);
	String::Cpy(pc_token->string, token.string);
	pc_token->type = token.type;
	pc_token->subtype = token.subtype;
	pc_token->intvalue = token.intvalue;
	pc_token->floatvalue = token.floatvalue;
	pc_token->line = token.line;
	pc_token->linescrossed = token.linescrossed;
	if (pc_token->type == TT_STRING)
	{
		StripDoubleQuotes(pc_token->string);
	}
	return ret;
}	//end of the function PC_ReadTokenHandle
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_UnreadLastTokenHandle(int handle)
{
	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return;
	}
	if (!sourceFiles[handle])
	{
		return;
	}

	PC_UnreadSourceToken(sourceFiles[handle], &sourceFiles[handle]->token);
}	//end of the function PC_UnreadLastTokenHandle
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
int PC_SourceFileAndLine(int handle, char* filename, int* line)
{
	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return qfalse;
	}
	if (!sourceFiles[handle])
	{
		return qfalse;
	}

	String::Cpy(filename, sourceFiles[handle]->filename);
	if (sourceFiles[handle]->scriptstack)
	{
		*line = sourceFiles[handle]->scriptstack->line;
	}
	else
	{
		*line = 0;
	}
	return qtrue;
}	//end of the function PC_SourceFileAndLine
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
void PC_SetBaseFolder(char* path)
{
	PS_SetBaseFolder(path);
}	//end of the function PC_SetBaseFolder
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
void PC_CheckOpenSourceHandles(void)
{
	int i;

	for (i = 1; i < MAX_SOURCEFILES; i++)
	{
		if (sourceFiles[i])
		{
			common->Printf(S_COLOR_RED "Error: file %s still open in precompiler\n", sourceFiles[i]->scriptstack->filename);
		}	//end if
	}	//end for
}	//end of the function PC_CheckOpenSourceHandles
