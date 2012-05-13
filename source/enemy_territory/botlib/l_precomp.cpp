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
