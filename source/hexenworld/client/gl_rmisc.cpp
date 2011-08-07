// r_misc.c

#include "quakedef.h"
#include "../../client/render_local.h"

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

void R_DrawName(vec3_t origin, char *Name, int Red)
{
	if (!Name)
	{
		return;
	}

	vec4_t eye;
	vec4_t Out;
	R_TransformModelToClip(origin, tr.viewParms.world.modelMatrix, tr.viewParms.projectionMatrix, eye, Out);

	if (eye[2] > -r_znear->value)
	{
		return;
	}

	vec4_t normalized;
	vec4_t window;
	R_TransformClipToWindow(Out, &tr.viewParms, normalized, window);
	int u = tr.refdef.x + (int)window[0];
	int v = tr.refdef.y + tr.refdef.height - (int)window[1];

	u -= QStr::Length(Name) * 4;

	if(cl_siege)
	{
		if(Red>10)
		{
			Red-=10;
			Draw_Character (u, v, 145);//key
			u+=8;
		}
		if(Red>0&&Red<3)//def
		{
			if(Red==true)
				Draw_Character (u, v, 143);//shield
			else
				Draw_Character (u, v, 130);//crown
			Draw_RedString(u+8, v, Name);
		}
		else if(!Red)
		{
			Draw_Character (u, v, 144);//sword
			Draw_String (u+8, v, Name);
		}
		else
			Draw_String (u+8, v, Name);
	}
	else
		Draw_String (u, v, Name);
}
