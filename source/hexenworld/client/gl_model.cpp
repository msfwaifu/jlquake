// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "glquake.h"

static byte	mod_novis[BSP29_MAX_MAP_LEAFS/8];

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	Com_Memset(mod_novis, 0xff, sizeof(mod_novis));

	R_ModelInit();
}

/*
===============
Mod_Init

Caches the data if needed
===============
*/
void *Mod_Extradata (model_t *mod)
{
	return mod->q1_cache;
}

/*
===============
Mod_PointInLeaf
===============
*/
mbrush29_leaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mbrush29_node_t		*node;
	float		d;
	cplane_t	*plane;
	
	if (!model || !model->brush29_nodes)
		Sys_Error ("Mod_PointInLeaf: bad model");

	node = model->brush29_nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mbrush29_leaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
static byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[BSP29_MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->brush29_numleafs+7)>>3;	
	out = decompressed;

#if 0
	Com_Memcpy(out, in, row);
#else
	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;		
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
#endif
	
	return decompressed;
}

byte *Mod_LeafPVS (mbrush29_leaf_t *leaf, model_t *model)
{
	if (leaf == model->brush29_leafs)
		return mod_novis;
	return Mod_DecompressVis (leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	R_FreeModels();
	Mod_Init();
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
qhandle_t Mod_ForName (char *name, qboolean crash)
{
	if (!name[0])
		Sys_Error ("Mod_ForName: NULL name");
		
//
// search the currently loaded models
//
	for (int i = 1; i < tr.numModels; i++)
		if (!QStr::Cmp(tr.models[i]->name, name))
			return tr.models[i]->index;
			
	model_t	*mod = R_AllocModel();
	QStr::Cpy(mod->name, name);

	void	*d;
	unsigned *buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap

//
// load the file
//
	buf = (unsigned *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
	{
		if (crash)
			Sys_Error ("Mod_NumForName: %s not found", mod->name);
		return 0;
	}
	
//
// allocate a new model
//
	loadmodel = mod;

//
// fill it in
//

// call the apropriate loader
	switch (LittleLong(*(unsigned *)buf))
	{
	case RAPOLYHEADER:
		Mod_LoadMdlModelNew (mod, buf);
		break;
	case IDPOLYHEADER:
		Mod_LoadMdlModel (mod, buf);
		break;
		
	case IDSPRITE1HEADER:
		Mod_LoadSpriteModel (mod, buf);
		break;
	
	default:
		Mod_LoadBrush29Model (mod, buf);
		break;
	}

	return mod->index;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
qhandle_t Mod_LoadWorld(const char *name)
{
	if (!name[0])
		Sys_Error ("Mod_ForName: NULL name");
		
	model_t	*mod = R_AllocModel();
	QStr::Cpy(mod->name, name);

	unsigned *buf;
	byte	stackbuf[1024];		// avoid dirtying the cache heap

//
// load the file
//
	buf = (unsigned *)COM_LoadStackFile (mod->name, stackbuf, sizeof(stackbuf));
	if (!buf)
	{
		return 0;
	}
	
//
// allocate a new model
//
	loadmodel = mod;

//
// fill it in
//

// call the apropriate loader
	Mod_LoadBrush29Model (mod, buf);

	tr.worldModel = mod;
	return mod->index;
}

/*
================
Mod_Print
================
*/
void Mod_Print (void)
{
	Con_Printf ("Cached models:\n");
	for (int i = 0; i < tr.numModels; i++)
	{
		Con_Printf ("%8p : %s\n", tr.models[i]->q1_cache, tr.models[i]->name);
	}
}

model_t* Mod_GetModel(qhandle_t handle)
{
	if (handle < 1 || handle >= tr.numModels || !tr.models[handle])
	{
		return tr.models[0];
	}
	return tr.models[handle];
}

void Mod_CalcScaleOffset(qhandle_t Handle, float ScaleX, float ScaleY, float ScaleZ, float ScaleZOrigin, vec3_t Out)
{
	model_t* Model = Mod_GetModel(Handle);
	if (Model->type != MOD_MESH1)
	{
		throw QException("Not an alias model");
	}

	mesh1hdr_t* AliasHdr = (mesh1hdr_t*)Mod_Extradata(Model);

	Out[0] = -(ScaleX - 1.0) * (AliasHdr->scale[0] * 127.95 + AliasHdr->scale_origin[0]);
	Out[1] = -(ScaleY - 1.0) * (AliasHdr->scale[1] * 127.95 + AliasHdr->scale_origin[1]);
	Out[2] = -(ScaleZ - 1.0) * (ScaleZOrigin * 2.0 * AliasHdr->scale[2] * 127.95 + AliasHdr->scale_origin[2]);
}

int Mod_GetNumFrames(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->q1_numframes;
}

int Mod_GetFlags(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->q1_flags;
}

bool Mod_IsAliasModel(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->type == MOD_MESH1;
}

const char* Mod_GetName(qhandle_t Handle)
{
	return Mod_GetModel(Handle)->name;
}
