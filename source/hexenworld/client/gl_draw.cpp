
// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"
#include "glquake.h"

byte		*draw_chars;				// 8*8 graphic characters
byte		*draw_smallchars;			// Small characters for status bar
byte		*draw_menufont; 			// Big Menu Font
image_t		*draw_disc[MAX_DISC] = 
{
	NULL  // make the first one null for sure
};
image_t*	draw_backtile;

image_t*	char_texture;
image_t*	cs_texture; // crosshair texture
image_t*	char_smalltexture;
image_t*	char_menufonttexture;

int	trans_level = 0;

static byte cs_data[64] = {
	0xff, 0xff, 0xff, 0x4f, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x4f, 0xff, 0xff, 0xff, 0xff,
	0x4f, 0xff, 0x4f, 0xff, 0x4f, 0xff, 0x4f, 0xff,
	0xff, 0xff, 0xff, 0x4f, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x4f, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

image_t		*conback;

//=============================================================================
/* Support Routines */

/*
===============
Draw_Init
===============
*/
void Draw_Init (void)
{
	int		i;
	byte	*dest, *src;
	int		x, y;
	char	ver[40];
	int		start;
	int		f, fstep;
	char	temp[MAX_QPATH];

	// load the console background and the charset
	// by hand, because we need to write the version
	// string into the background before turning
	// it into a texture

	draw_chars = COM_LoadHunkFile ("gfx/menu/conchars.lmp");
	for (i=0 ; i<256*128 ; i++)
		if (draw_chars[i] == 0)
			draw_chars[i] = 255;	// proper transparent color

	byte* draw_chars32 = R_ConvertImage8To32(draw_chars, 256, 128, IMG8MODE_Normal);
	char_texture = R_CreateImage("charset", draw_chars32, 256, 128, false, false, GL_CLAMP, false);
	delete[] draw_chars32;
	GL_Bind(char_texture);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	byte* cs_data32 = R_ConvertImage8To32(cs_data, 8, 8, IMG8MODE_Normal);
	cs_texture = R_CreateImage("crosshair", cs_data32, 8, 8, false, false, GL_CLAMP, false);
	delete[] cs_data32;

	draw_smallchars = (byte*)R_GetWadLumpByName("tinyfont");
	for (i=0 ; i<128*32 ; i++)
		if (draw_smallchars[i] == 0)
			draw_smallchars[i] = 255;	// proper transparent color

	// now turn them into textures
	byte* draw_smallchars32 = R_ConvertImage8To32(draw_smallchars, 128, 32, IMG8MODE_Normal);
	char_smalltexture = R_CreateImage("smallcharset", draw_smallchars32, 128, 32, false, false, GL_CLAMP, false);
	delete[] draw_smallchars32;

	char_menufonttexture = R_FindImageFile("gfx/menu/bigfont2.lmp", false, false, GL_CLAMP, false, IMG8MODE_Holey);

	start = Hunk_LowMark ();

	int cbwidth;
	int cbheight;
	byte* pic32;
	R_LoadImage("gfx/menu/conback.lmp", &pic32, &cbwidth, &cbheight);
	if (!pic32)
		Sys_Error ("Couldn't load gfx/menu/conback.lmp");

	conback = R_CreateImage("conback", pic32, cbwidth, cbheight, false, false, GL_CLAMP, false);
	delete[] pic32;

	// free loaded console
	Hunk_FreeToLowMark (start);

	//
	// get the other pics we need
	//
	for(i=MAX_DISC-1;i>=0;i--)
	{
		sprintf(temp,"gfx/menu/skull%d.lmp",i);
		draw_disc[i] = UI_CachePic(temp);
	}
	draw_backtile = UI_CachePicRepeat("gfx/menu/backtile.lmp");
}



/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character (int x, int y, unsigned int num)
{
	int				row, col;
	float			frow, fcol, xsize,ysize;

	num &= 511;

	if (num == 32)
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num>>5;
	col = num&31;

	xsize = 0.03125;
	ysize = 0.0625;
	fcol = col*xsize;
	frow = row*ysize;

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	GL_Bind (char_texture);

	qglColor4f (1,1,1,1);
	DoQuad(x, y, fcol, frow, x + 8, y + 8, fcol + xsize, frow + ysize);
}

/*
================
Draw_String
================
*/
void Draw_String (int x, int y, const char *str)
{
	while (*str)
	{
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
}

void Draw_RedString (int x, int y, const char *str)
{
	while (*str)
	{
		Draw_Character (x, y, ((unsigned char)(*str))+256);
		str++;
		x += 8;
	}
}

void Draw_Crosshair(void)
{
	extern QCvar* crosshair;
	extern QCvar* cl_crossx;
	extern QCvar* cl_crossy;
	int x, y;
	extern vrect_t		scr_vrect;

	if (crosshair->value == 2) {
		x = scr_vrect.x + scr_vrect.width/2 - 3 + cl_crossx->value; 
		y = scr_vrect.y + scr_vrect.height/2 - 3 + cl_crossy->value;
		GL_Bind (cs_texture);

		GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

		qglColor4f (1,1,1,1);
		DoQuad(x - 4, y - 4, 0, 0, x + 12, y + 12, 1, 1);
	} else if (crosshair->value)
		Draw_Character (scr_vrect.x + scr_vrect.width/2-4 + cl_crossx->value, 
			scr_vrect.y + scr_vrect.height/2-4 + cl_crossy->value, 
			'+');
}


//==========================================================================
//
// Draw_SmallCharacter
//
// Draws a small character that is clipped at the bottom edge of the
// screen.
//
//==========================================================================
void Draw_SmallCharacter (int x, int y, int num)
{
	byte			*dest;
	byte			*source;
	unsigned short	*pusdest;
	int				drawline;
	int				row, col;
	float			frow, fcol, xsize,ysize;

	if(num < 32)
	{
		num = 0;
	}
	else if(num >= 'a' && num <= 'z')
	{
		num -= 64;
	}
	else if(num > '_')
	{
		num = 0;
	}
	else
	{
		num -= 32;
	}

	if (num == 0) return;

	if (y <= -8)
		return; 		// totally off screen

	if(y >= viddef.height)
	{ // Totally off screen
		return;
	}

	row = num>>4;
	col = num&15;

	xsize = 0.0625;
	ysize = 0.25;
	fcol = col*xsize;
	frow = row*ysize;

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	GL_Bind (char_smalltexture);

	qglColor4f (1,1,1,1);
	DoQuad(x, y, fcol, frow, x + 8, y + 8, fcol + xsize, frow + ysize);
}

//==========================================================================
//
// Draw_SmallString
//
//==========================================================================
void Draw_SmallString(int x, int y, const char *str)
{
	while (*str)
	{
		Draw_SmallCharacter (x, y, *str);
		str++;
		x += 6;
	}
}

int M_DrawBigCharacter (int x, int y, int num, int numNext)
{
	byte			*dest;
	byte			*source;
	unsigned short	*pusdest;
	int				drawline;
	int				row, col;
	float			frow, fcol, xsize,ysize;
	int				add;

	if (num == ' ') return 32;

	if (num == '/') num = 26;
	else num -= 65;

	if (num < 0 || num >= 27)  // only a-z and /
		return 0;

	if (numNext == '/') numNext = 26;
	else numNext -= 65;

	row = num/8;
	col = num%8;

	xsize = 0.125;
	ysize = 0.25;
	fcol = col*xsize;
	frow = row*ysize;

	GL_Bind (char_menufonttexture);

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);

	qglColor4f (1,1,1,1);
	DoQuad(x, y, fcol, frow, x + 20, y + 20, fcol + xsize, frow + ysize);

	if (numNext < 0 || numNext >= 27) return 0;

	add = 0;
	if (num == (int)'C'-65 && numNext == (int)'P'-65)
		add = 3;

	return BigCharWidth[num][numNext] + add;
}


/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground(int lines)
{
	int y = (viddef.height * 3) >> 2;
	if (lines > y)
	{
		UI_DrawStretchPic(0, lines - viddef.height, viddef.width, viddef.height, conback);
	}
	else
	{
		UI_DrawStretchPic(0, lines - viddef.height, viddef.width, viddef.height, conback, (float)(1.2 * lines) / y);
	}

	// hack the version number directly into the pic
	y = lines - 14;
	if (!cls.download)
	{
		char ver[80];
		sprintf(ver, "GL HexenWorld %4.2f", VERSION); // JACK: ZOID! Passing
													   // parms?!
		int x = viddef.width - (QStr::Length(ver) * 8 + 11);
		for (int i = 0; i < QStr::Length(ver); i++)
		{
			Draw_Character(x + i * 8, y, ver[i] | 0x100);
		}
	}
}


/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h)
{
	qglColor3f (1,1,1);
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
	GL_Bind (draw_backtile);
	DoQuad(x, y, x / 64.0, y / 64.0, x + w, y + h, (x + w) / 64.0, (y + h) / 64.0);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
	qglDisable (GL_TEXTURE_2D);
	qglColor3f (host_basepal[c*3]/255.0,
		host_basepal[c*3+1]/255.0,
		host_basepal[c*3+2]/255.0);

	DoQuad(x, y, 0, 0, x + w, y + h, 0, 0);
	qglColor3f (1,1,1);
	glEnable (GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	int bx,by,ex,ey;
	int c;

	GL_State(GLS_DEFAULT | GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_TEXTURE_2D);

//	qglColor4f (248.0/255.0, 220.0/255.0, 120.0/255.0, 0.2);
	qglColor4f (208.0/255.0, 180.0/255.0, 80.0/255.0, 0.2);
	DoQuad(0, 0, 0, 0, viddef.width, viddef.height, 0, 0);

	qglColor4f (208.0/255.0, 180.0/255.0, 80.0/255.0, 0.035);
	for(c=0;c<40;c++)
	{
		bx = rand() % viddef.width-20;
		by = rand() % viddef.height-20;
		ex = bx + (rand() % 40) + 20;
		ey = by + (rand() % 40) + 20;
		if (bx < 0) bx = 0;
		if (by < 0) by = 0;
		if (ex > viddef.width) ex = viddef.width;
		if (ey > viddef.height) ey = viddef.height;

		DoQuad(bx, by, 0, 0, ex, ey, 0, 0);
	}

	qglColor4f (1,1,1,1);
	glEnable (GL_TEXTURE_2D);

	GL_State(GLS_DEFAULT | GLS_ATEST_GE_80 | GLS_DEPTHTEST_DISABLE);
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc (void)
{
	static int index = 0;

	if (!draw_disc[index]) 
	{
		return;
	}

	index++;
	if (index >= MAX_DISC) index = 0;

	qglDrawBuffer  (GL_FRONT);

	UI_DrawPic (viddef.width - 28, 4, draw_disc[index]);

	qglDrawBuffer  (GL_BACK);
}


/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc (void)
{
}
