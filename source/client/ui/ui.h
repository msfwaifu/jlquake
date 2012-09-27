//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#define SMALLCHAR_WIDTH     8
#define SMALLCHAR_HEIGHT    16

#define BIGCHAR_WIDTH       16
#define BIGCHAR_HEIGHT      16

struct vrect_t
{
	int x;
	int y;
	int width;
	int height;
};

struct viddef_t
{
	int width;
	int height;
};

extern viddef_t viddef;					// global video state
extern image_t* char_texture;
extern image_t* char_smalltexture;
extern vec4_t g_color_table[32];

void UI_AdjustFromVirtualScreen(float* x, float* y, float* w, float* h);
void UI_DrawPic(int x, int y, image_t* pic, float alpha = 1);
void UI_DrawNamedPic(int x, int y, const char* name);
void UI_DrawStretchPic(int x, int y, int w, int h, image_t* gl, float alpha = 1);
void UI_DrawStretchNamedPic(int x, int y, int w, int h, const char* name);
void UI_DrawStretchPicWithColour(int x, int y, int w, int h, image_t* pic, byte* colour);
void UI_DrawSubPic(int x, int y, image_t* pic, int srcx, int srcy, int width, int height);
void UI_TileClear(int x, int y, int w, int h, image_t* pic);
void UI_NamedTileClear(int x, int y, int w, int h, const char* name);
void UI_Fill(int x, int y, int w, int h, float r, float g, float b, float a);
void UI_FillPal(int x, int y, int w, int h, int c);
void UI_DrawCharBase(int x, int y, int num, int w, int h, image_t* image, int numberOfColumns,
	int numberOfRows, float r = 1, float g = 1, float b = 1, float a = 1);
void UI_DrawChar(int x, int y, int num, float r = 1, float g = 1, float b = 1, float a = 1);
void UI_DrawString(int x, int y, const char* str, int mask = 0);
void UI_DrawSmallString(int x, int y, const char* str);

void SCR_FillRect(float x, float y, float width, float height, const float* color);
void SCR_DrawPic(float x, float y, float width, float height, qhandle_t hShader);
void SCR_DrawNamedPic(float x, float y, float width, float height, const char* picname);
void SCR_DrawSmallChar(int x, int y, int ch);
void SCR_DrawStringExt(int x, int y, float size, const char* string, float* setColor, bool forceColor);
void SCR_DrawBigString(int x, int y, const char* s, float alpha);		// draws a string with embedded color control characters with fade
void SCR_DrawBigStringColor(int x, int y, const char* s, vec4_t color);	// ignores embedded color control characters
void SCR_DrawSmallString(int x, int y, const char* string);

extern Cvar* scr_viewsize;
extern Cvar* crosshair;
extern float h2_introTime;

void SCR_InitCommon();
void SCR_EndLoadingPlaque();
void SCR_DrawDebugGraph();

bool Field_KeyDownEvent(field_t* edit, int key);
void Field_CharEvent(field_t* edit, int ch);
void Field_Draw(field_t* edit, int x, int y, bool showCursor);
void Field_BigDraw(field_t* edit, int x, int y, bool showCursor);

float CalcFov(float fov_x, float width, float height);

void UI_Init();
void UI_KeyEvent(int key, bool down);
void UI_KeyDownEvent(int key);
void UI_CharEvent(int key);
void UI_MouseEvent(int dx, int dy);
void UI_DrawMenu();
bool UI_IsFullscreen();
void UI_ForceMenuOff();
void UI_SetMainMenu();
void UI_SetInGameMenu();
