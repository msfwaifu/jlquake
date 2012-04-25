
#include "../progsvm/progsvm.h"	// defs shared with qcc
#include "progdefs.h"			// generated by program cdefs

#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,qhedict_t,area)

//============================================================================

extern globalvars_t* pr_global_struct;

extern int pr_edict_size;				// in bytes

extern int* pr_string_index;
extern char* pr_global_strings;
extern int pr_string_count;

//============================================================================

void PR_Init(void);

void PR_ExecuteProgram(func_t fnum);
void PR_LoadProgs(void);
void PR_LoadStrings(void);
void PR_LoadInfoStrings(void);

void PR_Profile_f(void);

qhedict_t* ED_Alloc(void);
qhedict_t* ED_Alloc_Temp(void);
void ED_Free(qhedict_t* ed);

char* ED_NewString(const char* string);
// returns a copy of the string allocated from the server's string heap

void ED_Print(qhedict_t* ed);
void ED_Write(fileHandle_t f, qhedict_t* ed);
const char* ED_ParseEdict(const char* data, qhedict_t* ent);

void ED_WriteGlobals(fileHandle_t f);
const char* ED_ParseGlobals(const char* data);

void ED_LoadFromFile(const char* data);

//define EDICT_NUM(n) ((qhedict_t *)(sv.edicts+ (n)*pr_edict_size))
//define NUM_FOR_EDICT(e) (((byte *)(e) - sv.edicts)/pr_edict_size)

qhedict_t* EDICT_NUM(int n);
int NUM_FOR_EDICT(qhedict_t* e);

#define NEXT_EDICT(e) ((qhedict_t*)((byte*)e + pr_edict_size))

#define EDICT_TO_PROG(e) ((byte*)e - (byte*)sv.edicts)
#define PROG_TO_EDICT(e) ((qhedict_t*)((byte*)sv.edicts + e))

//============================================================================

#define G_FLOAT(o) (pr_globals[o])
#define G_INT(o) (*(int*)&pr_globals[o])
#define G_EDICT(o) ((qhedict_t*)((byte*)sv.edicts + *(int*)&pr_globals[o]))
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))
#define G_VECTOR(o) (&pr_globals[o])
#define G_STRING(o) (PR_GetString(*(string_t*)&pr_globals[o]))
#define G_FUNCTION(o) (*(func_t*)&pr_globals[o])

#define E_FLOAT(e,o) (((float*)&e->v)[o])
#define E_INT(e,o) (*(int*)&((float*)&e->v)[o])
#define E_VECTOR(e,o) (&((float*)&e->v)[o])
#define E_STRING(e,o) (PR_GetString(*(string_t*)&((float*)&e->v)[o]))

extern int type_size[8];

typedef void (*builtin_t)(void);
extern builtin_t* pr_builtins;
extern int pr_numbuiltins;

extern unsigned short pr_crc;

void PR_RunError(const char* error, ...);

void ED_PrintEdicts(void);
void ED_PrintNum(int ent);

eval_t* GetEdictFieldValue(qhedict_t* ed, const char* field);

extern Cvar* max_temp_edicts;

extern qboolean ignore_precache;

void ED_ClearEdict(qhedict_t* e);
