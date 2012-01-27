/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sbar.c -- status bar code

#include "quakedef.h"


#define STAT_MINUS		10	// num frame for '-' stats digit
image_t		*sb_nums[2][11];
image_t		*sb_colon, *sb_slash;
image_t		*sb_ibar;
image_t		*sb_sbar;
image_t		*sb_scorebar;

image_t      *sb_weapons[7][8];   // 0 is active, 1 is owned, 2-5 are flashes
image_t      *sb_ammo[4];
image_t		*sb_sigil[4];
image_t		*sb_armor[3];
image_t		*sb_items[32];
image_t*	draw_disc;

image_t	*sb_faces[7][2];		// 0 is gibbed, 1 is dead, 2-6 are alive
							// 0 is static, 1 is temporary animation
image_t	*sb_face_invis;
image_t	*sb_face_quad;
image_t	*sb_face_invuln;
image_t	*sb_face_invis_invuln;

qboolean	sb_showscores;

int			sb_lines;			// scan lines to draw

image_t      *rsb_invbar[2];
image_t      *rsb_weapons[5];
image_t      *rsb_items[2];
image_t      *rsb_ammo[3];
image_t      *rsb_teambord;		// PGM 01/19/97 - team color border

//MED 01/04/97 added two more weapons + 3 alternates for grenade launcher
image_t      *hsb_weapons[7][5];   // 0 is active, 1 is owned, 2-5 are flashes
//MED 01/04/97 added array to simplify weapon parsing
int         hipweapons[4] = {HIT_LASER_CANNON_BIT,HIT_MJOLNIR_BIT,4,HIT_PROXIMITY_GUN_BIT};
//MED 01/04/97 added hipnotic items array
image_t      *hsb_items[2];

void Sbar_MiniDeathmatchOverlay (void);
void Sbar_DeathmatchOverlay (void);
void M_DrawPic (int x, int y, image_t *pic);

/*
===============
Sbar_ShowScores

Tab key down
===============
*/
void Sbar_ShowScores (void)
{
	if (sb_showscores)
		return;
	sb_showscores = true;
}

/*
===============
Sbar_DontShowScores

Tab key up
===============
*/
void Sbar_DontShowScores (void)
{
	sb_showscores = false;
}

/*
===============
Sbar_Init
===============
*/
void Sbar_Init (void)
{
	int		i;

	for (i=0 ; i<10 ; i++)
	{
		sb_nums[0][i] = R_PicFromWad (va("num_%i",i));
		sb_nums[1][i] = R_PicFromWad (va("anum_%i",i));
	}

	sb_nums[0][10] = R_PicFromWad ("num_minus");
	sb_nums[1][10] = R_PicFromWad ("anum_minus");

	sb_colon = R_PicFromWad ("num_colon");
	sb_slash = R_PicFromWad ("num_slash");

	sb_weapons[0][0] = R_PicFromWad ("inv_shotgun");
	sb_weapons[0][1] = R_PicFromWad ("inv_sshotgun");
	sb_weapons[0][2] = R_PicFromWad ("inv_nailgun");
	sb_weapons[0][3] = R_PicFromWad ("inv_snailgun");
	sb_weapons[0][4] = R_PicFromWad ("inv_rlaunch");
	sb_weapons[0][5] = R_PicFromWad ("inv_srlaunch");
	sb_weapons[0][6] = R_PicFromWad ("inv_lightng");

	sb_weapons[1][0] = R_PicFromWad ("inv2_shotgun");
	sb_weapons[1][1] = R_PicFromWad ("inv2_sshotgun");
	sb_weapons[1][2] = R_PicFromWad ("inv2_nailgun");
	sb_weapons[1][3] = R_PicFromWad ("inv2_snailgun");
	sb_weapons[1][4] = R_PicFromWad ("inv2_rlaunch");
	sb_weapons[1][5] = R_PicFromWad ("inv2_srlaunch");
	sb_weapons[1][6] = R_PicFromWad ("inv2_lightng");

	for (i=0 ; i<5 ; i++)
	{
		sb_weapons[2+i][0] = R_PicFromWad (va("inva%i_shotgun",i+1));
		sb_weapons[2+i][1] = R_PicFromWad (va("inva%i_sshotgun",i+1));
		sb_weapons[2+i][2] = R_PicFromWad (va("inva%i_nailgun",i+1));
		sb_weapons[2+i][3] = R_PicFromWad (va("inva%i_snailgun",i+1));
		sb_weapons[2+i][4] = R_PicFromWad (va("inva%i_rlaunch",i+1));
		sb_weapons[2+i][5] = R_PicFromWad (va("inva%i_srlaunch",i+1));
		sb_weapons[2+i][6] = R_PicFromWad (va("inva%i_lightng",i+1));
	}

	sb_ammo[0] = R_PicFromWad ("sb_shells");
	sb_ammo[1] = R_PicFromWad ("sb_nails");
	sb_ammo[2] = R_PicFromWad ("sb_rocket");
	sb_ammo[3] = R_PicFromWad ("sb_cells");

	sb_armor[0] = R_PicFromWad ("sb_armor1");
	sb_armor[1] = R_PicFromWad ("sb_armor2");
	sb_armor[2] = R_PicFromWad ("sb_armor3");

	sb_items[0] = R_PicFromWad ("sb_key1");
	sb_items[1] = R_PicFromWad ("sb_key2");
	sb_items[2] = R_PicFromWad ("sb_invis");
	sb_items[3] = R_PicFromWad ("sb_invuln");
	sb_items[4] = R_PicFromWad ("sb_suit");
	sb_items[5] = R_PicFromWad ("sb_quad");

	draw_disc = R_PicFromWad ("disc");

	sb_sigil[0] = R_PicFromWad ("sb_sigil1");
	sb_sigil[1] = R_PicFromWad ("sb_sigil2");
	sb_sigil[2] = R_PicFromWad ("sb_sigil3");
	sb_sigil[3] = R_PicFromWad ("sb_sigil4");

	sb_faces[4][0] = R_PicFromWad ("face1");
	sb_faces[4][1] = R_PicFromWad ("face_p1");
	sb_faces[3][0] = R_PicFromWad ("face2");
	sb_faces[3][1] = R_PicFromWad ("face_p2");
	sb_faces[2][0] = R_PicFromWad ("face3");
	sb_faces[2][1] = R_PicFromWad ("face_p3");
	sb_faces[1][0] = R_PicFromWad ("face4");
	sb_faces[1][1] = R_PicFromWad ("face_p4");
	sb_faces[0][0] = R_PicFromWad ("face5");
	sb_faces[0][1] = R_PicFromWad ("face_p5");

	sb_face_invis = R_PicFromWad ("face_invis");
	sb_face_invuln = R_PicFromWad ("face_invul2");
	sb_face_invis_invuln = R_PicFromWad ("face_inv2");
	sb_face_quad = R_PicFromWad ("face_quad");

	Cmd_AddCommand ("+showscores", Sbar_ShowScores);
	Cmd_AddCommand ("-showscores", Sbar_DontShowScores);

	sb_sbar = R_PicFromWad ("sbar");
	sb_ibar = R_PicFromWad ("ibar");
	sb_scorebar = R_PicFromWad ("scorebar");

//MED 01/04/97 added new hipnotic weapons
	if (hipnotic)
	{
	  hsb_weapons[0][0] = R_PicFromWad ("inv_laser");
	  hsb_weapons[0][1] = R_PicFromWad ("inv_mjolnir");
	  hsb_weapons[0][2] = R_PicFromWad ("inv_gren_prox");
	  hsb_weapons[0][3] = R_PicFromWad ("inv_prox_gren");
	  hsb_weapons[0][4] = R_PicFromWad ("inv_prox");

	  hsb_weapons[1][0] = R_PicFromWad ("inv2_laser");
	  hsb_weapons[1][1] = R_PicFromWad ("inv2_mjolnir");
	  hsb_weapons[1][2] = R_PicFromWad ("inv2_gren_prox");
	  hsb_weapons[1][3] = R_PicFromWad ("inv2_prox_gren");
	  hsb_weapons[1][4] = R_PicFromWad ("inv2_prox");

	  for (i=0 ; i<5 ; i++)
	  {
		 hsb_weapons[2+i][0] = R_PicFromWad (va("inva%i_laser",i+1));
		 hsb_weapons[2+i][1] = R_PicFromWad (va("inva%i_mjolnir",i+1));
		 hsb_weapons[2+i][2] = R_PicFromWad (va("inva%i_gren_prox",i+1));
		 hsb_weapons[2+i][3] = R_PicFromWad (va("inva%i_prox_gren",i+1));
		 hsb_weapons[2+i][4] = R_PicFromWad (va("inva%i_prox",i+1));
	  }

	  hsb_items[0] = R_PicFromWad ("sb_wsuit");
	  hsb_items[1] = R_PicFromWad ("sb_eshld");
	}

	if (rogue)
	{
		rsb_invbar[0] = R_PicFromWad ("r_invbar1");
		rsb_invbar[1] = R_PicFromWad ("r_invbar2");

		rsb_weapons[0] = R_PicFromWad ("r_lava");
		rsb_weapons[1] = R_PicFromWad ("r_superlava");
		rsb_weapons[2] = R_PicFromWad ("r_gren");
		rsb_weapons[3] = R_PicFromWad ("r_multirock");
		rsb_weapons[4] = R_PicFromWad ("r_plasma");

		rsb_items[0] = R_PicFromWad ("r_shield1");
        rsb_items[1] = R_PicFromWad ("r_agrav1");

// PGM 01/19/97 - team color border
        rsb_teambord = R_PicFromWad ("r_teambord");
// PGM 01/19/97 - team color border

		rsb_ammo[0] = R_PicFromWad ("r_ammolava");
		rsb_ammo[1] = R_PicFromWad ("r_ammomulti");
		rsb_ammo[2] = R_PicFromWad ("r_ammoplasma");
	}
}


//=============================================================================

// drawing routines are relative to the status bar location

/*
=============
Sbar_DrawPic
=============
*/
void Sbar_DrawPic (int x, int y, image_t *pic)
{
	if (cl.qh_gametype == GAME_DEATHMATCH)
		UI_DrawPic (x /* + ((viddef.width - 320)>>1)*/, y + (viddef.height-SBAR_HEIGHT), pic);
	else
		UI_DrawPic (x + ((viddef.width - 320)>>1), y + (viddef.height-SBAR_HEIGHT), pic);
}

/*
=============
Sbar_DrawTransPic
=============
*/
void Sbar_DrawTransPic (int x, int y, image_t *pic)
{
	if (cl.qh_gametype == GAME_DEATHMATCH)
		UI_DrawPic (x /*+ ((viddef.width - 320)>>1)*/, y + (viddef.height-SBAR_HEIGHT), pic);
	else
		UI_DrawPic (x + ((viddef.width - 320)>>1), y + (viddef.height-SBAR_HEIGHT), pic);
}

/*
================
Sbar_DrawCharacter

Draws one solid graphics character
================
*/
void Sbar_DrawCharacter (int x, int y, int num)
{
	if (cl.qh_gametype == GAME_DEATHMATCH)
		Draw_Character ( x /*+ ((viddef.width - 320)>>1) */ + 4 , y + viddef.height-SBAR_HEIGHT, num);
	else
		Draw_Character ( x + ((viddef.width - 320)>>1) + 4 , y + viddef.height-SBAR_HEIGHT, num);
}

/*
================
Sbar_DrawString
================
*/
void Sbar_DrawString (int x, int y, char *str)
{
	if (cl.qh_gametype == GAME_DEATHMATCH)
		Draw_String (x /*+ ((viddef.width - 320)>>1)*/, y+ viddef.height-SBAR_HEIGHT, str);
	else
		Draw_String (x + ((viddef.width - 320)>>1), y+ viddef.height-SBAR_HEIGHT, str);
}

/*
=============
Sbar_itoa
=============
*/
int Sbar_itoa (int num, char *buf)
{
	char	*str;
	int		pow10;
	int		dig;

	str = buf;

	if (num < 0)
	{
		*str++ = '-';
		num = -num;
	}

	for (pow10 = 10 ; num >= pow10 ; pow10 *= 10)
	;

	do
	{
		pow10 /= 10;
		dig = num/pow10;
		*str++ = '0'+dig;
		num -= dig*pow10;
	} while (pow10 != 1);

	*str = 0;

	return str-buf;
}


/*
=============
Sbar_DrawNum
=============
*/
void Sbar_DrawNum (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Sbar_DrawTransPic (x,y,sb_nums[color][frame]);
		x += 24;
		ptr++;
	}
}

//=============================================================================

int		fragsort[MAX_CLIENTS_Q1];

char	scoreboardtext[MAX_CLIENTS_Q1][20];
int		scoreboardtop[MAX_CLIENTS_Q1];
int		scoreboardbottom[MAX_CLIENTS_Q1];
int		scoreboardlines;

/*
===============
Sbar_SortFrags
===============
*/
void Sbar_SortFrags (void)
{
	int		i, j, k;

// sort by frags
	scoreboardlines = 0;
	for (i=0 ; i<cl.qh_maxclients ; i++)
	{
		if (cl.q1_players[i].name[0])
		{
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
		}
	}

	for (i=0 ; i<scoreboardlines ; i++)
		for (j=0 ; j<scoreboardlines-1-i ; j++)
			if (cl.q1_players[fragsort[j]].frags < cl.q1_players[fragsort[j+1]].frags)
			{
				k = fragsort[j];
				fragsort[j] = fragsort[j+1];
				fragsort[j+1] = k;
			}
}

int	Sbar_ColorForMap (int m)
{
	return m < 128 ? m + 8 : m + 8;
}

/*
===============
Sbar_UpdateScoreboard
===============
*/
void Sbar_UpdateScoreboard (void)
{
	int		i, k;
	int		top, bottom;
	q1player_info_t	*s;

	Sbar_SortFrags ();

// draw the text
	Com_Memset(scoreboardtext, 0, sizeof(scoreboardtext));

	for (i=0 ; i<scoreboardlines; i++)
	{
		k = fragsort[i];
		s = &cl.q1_players[k];
		sprintf (&scoreboardtext[i][1], "%3i %s", s->frags, s->name);

		top = s->topcolor << 4;
		bottom = s->bottomcolor <<4;
		scoreboardtop[i] = Sbar_ColorForMap (top);
		scoreboardbottom[i] = Sbar_ColorForMap (bottom);
	}
}



/*
===============
Sbar_SoloScoreboard
===============
*/
void Sbar_SoloScoreboard (void)
{
	char	str[80];
	int		minutes, seconds, tens, units;
	int		l;

	sprintf (str,"Monsters:%3i /%3i", cl.qh_stats[STAT_MONSTERS], cl.qh_stats[STAT_TOTALMONSTERS]);
	Sbar_DrawString (8, 4, str);

	sprintf (str,"Secrets :%3i /%3i", cl.qh_stats[STAT_SECRETS], cl.qh_stats[STAT_TOTALSECRETS]);
	Sbar_DrawString (8, 12, str);

// time
	minutes = cl.qh_serverTimeFloat / 60;
	seconds = cl.qh_serverTimeFloat - 60*minutes;
	tens = seconds / 10;
	units = seconds - 10*tens;
	sprintf (str,"Time :%3i:%i%i", minutes, tens, units);
	Sbar_DrawString (184, 4, str);

// draw level name
	l = String::Length(cl.qh_levelname);
	Sbar_DrawString (232 - l*4, 12, cl.qh_levelname);
}

/*
===============
Sbar_DrawScoreboard
===============
*/
void Sbar_DrawScoreboard (void)
{
	Sbar_SoloScoreboard ();
	if (cl.qh_gametype == GAME_DEATHMATCH)
		Sbar_DeathmatchOverlay ();
#if 0
	int		i, j, c;
	int		x, y;
	int		l;
	int		top, bottom;
	q1player_info_t	*s;

	if (cl.gametype != GAME_DEATHMATCH)
	{
		Sbar_SoloScoreboard ();
		return;
	}

	Sbar_UpdateScoreboard ();

	l = scoreboardlines <= 6 ? scoreboardlines : 6;

	for (i=0 ; i<l ; i++)
	{
		x = 20*(i&1);
		y = i/2 * 8;

		s = &cl.scores[fragsort[i]];
		if (!s->name[0])
			continue;

	// draw background
		top = s->colors & 0xf0;
		bottom = (s->colors & 15)<<4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		UI_FillPal ( x*8+10 + ((viddef.width - 320)>>1), y + viddef.height - SBAR_HEIGHT, 28, 4, top);
		UI_FillPal ( x*8+10 + ((viddef.width - 320)>>1), y+4 + viddef.height - SBAR_HEIGHT, 28, 4, bottom);

	// draw text
		for (j=0 ; j<20 ; j++)
		{
			c = scoreboardtext[i][j];
			if (c == 0 || c == ' ')
				continue;
			Sbar_DrawCharacter ( (x+j)*8, y, c);
		}
	}
#endif
}

//=============================================================================

/*
===============
Sbar_DrawInventory
===============
*/
void Sbar_DrawInventory (void)
{
	int		i;
	char	num[6];
	float	time;
	int		flashon;

	if (rogue)
	{
		if ( cl.qh_stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
			Sbar_DrawPic (0, -24, rsb_invbar[0]);
		else
			Sbar_DrawPic (0, -24, rsb_invbar[1]);
	}
	else
	{
		Sbar_DrawPic (0, -24, sb_ibar);
	}

// weapons
	for (i=0 ; i<7 ; i++)
	{
		if (cl.q1_items & (IT_SHOTGUN<<i) )
		{
			time = cl.q1_item_gettime[i];
			flashon = (int)((cl.qh_serverTimeFloat - time)*10);
			if (flashon >= 10)
			{
				if ( cl.qh_stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN<<i)  )
					flashon = 1;
				else
					flashon = 0;
			}
			else
				flashon = (flashon%5) + 2;

         Sbar_DrawPic (i*24, -16, sb_weapons[flashon][i]);
		}
	}

// MED 01/04/97
// hipnotic weapons
    if (hipnotic)
    {
      int grenadeflashing=0;
      for (i=0 ; i<4 ; i++)
      {
         if (cl.q1_items & (1<<hipweapons[i]) )
         {
            time = cl.q1_item_gettime[hipweapons[i]];
            flashon = (int)((cl.qh_serverTimeFloat - time)*10);
            if (flashon >= 10)
            {
               if ( cl.qh_stats[STAT_ACTIVEWEAPON] == (1<<hipweapons[i])  )
                  flashon = 1;
               else
                  flashon = 0;
            }
            else
               flashon = (flashon%5) + 2;

            // check grenade launcher
            if (i==2)
            {
               if (cl.q1_items & HIT_PROXIMITY_GUN)
               {
                  if (flashon)
                  {
                     grenadeflashing = 1;
                     Sbar_DrawPic (96, -16, hsb_weapons[flashon][2]);
                  }
               }
            }
            else if (i==3)
            {
               if (cl.q1_items & (IT_SHOTGUN<<4))
               {
                  if (flashon && !grenadeflashing)
                  {
                     Sbar_DrawPic (96, -16, hsb_weapons[flashon][3]);
                  }
                  else if (!grenadeflashing)
                  {
                     Sbar_DrawPic (96, -16, hsb_weapons[0][3]);
                  }
               }
               else
                  Sbar_DrawPic (96, -16, hsb_weapons[flashon][4]);
            }
            else
               Sbar_DrawPic (176 + (i*24), -16, hsb_weapons[flashon][i]);
         }
      }
    }

	if (rogue)
	{
    // check for powered up weapon.
		if ( cl.qh_stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
		{
			for (i=0;i<5;i++)
			{
				if (cl.qh_stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
				{
					Sbar_DrawPic ((i+2)*24, -16, rsb_weapons[i]);
				}
			}
		}
	}

// ammo counts
	for (i=0 ; i<4 ; i++)
	{
		sprintf (num, "%3i",cl.qh_stats[STAT_SHELLS+i] );
		if (num[0] != ' ')
			Sbar_DrawCharacter ( (6*i+1)*8 - 2, -24, 18 + num[0] - '0');
		if (num[1] != ' ')
			Sbar_DrawCharacter ( (6*i+2)*8 - 2, -24, 18 + num[1] - '0');
		if (num[2] != ' ')
			Sbar_DrawCharacter ( (6*i+3)*8 - 2, -24, 18 + num[2] - '0');
	}

	flashon = 0;
   // items
   for (i=0 ; i<6 ; i++)
      if (cl.q1_items & (1<<(17+i)))
      {
         time = cl.q1_item_gettime[17+i];
         if (!(time && time > cl.qh_serverTimeFloat - 2 && flashon ))
         {
         //MED 01/04/97 changed keys
            if (!hipnotic || (i>1))
            {
               Sbar_DrawPic (192 + i*16, -16, sb_items[i]);
            }
         }
      }
   //MED 01/04/97 added hipnotic items
   // hipnotic items
   if (hipnotic)
   {
      for (i=0 ; i<2 ; i++)
         if (cl.q1_items & (1<<(24+i)))
         {
            time = cl.q1_item_gettime[24+i];
            if (!(time && time > cl.qh_serverTimeFloat - 2 && flashon ))
            {
               Sbar_DrawPic (288 + i*16, -16, hsb_items[i]);
            }
         }
   }

	if (rogue)
	{
	// new rogue items
		for (i=0 ; i<2 ; i++)
		{
			if (cl.q1_items & (1<<(29+i)))
			{
				time = cl.q1_item_gettime[29+i];

				if (!(time &&	time > cl.qh_serverTimeFloat - 2 && flashon ))
				{
					Sbar_DrawPic (288 + i*16, -16, rsb_items[i]);
				}
			}
		}
	}
	else
	{
	// sigils
		for (i=0 ; i<4 ; i++)
		{
			if (cl.q1_items & (1<<(28+i)))
			{
				time = cl.q1_item_gettime[28+i];
				if (!(time &&	time > cl.qh_serverTimeFloat - 2 && flashon ))
					Sbar_DrawPic (320-32 + i*8, -16, sb_sigil[i]);
			}
		}
	}
}

//=============================================================================

/*
===============
Sbar_DrawFrags
===============
*/
void Sbar_DrawFrags (void)
{
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	int				xofs;
	char			num[12];
	q1player_info_t	*s;

	Sbar_SortFrags ();

// draw the text
	l = scoreboardlines <= 4 ? scoreboardlines : 4;

	x = 23;
	if (cl.qh_gametype == GAME_DEATHMATCH)
		xofs = 0;
	else
		xofs = (viddef.width - 320)>>1;
	y = viddef.height - SBAR_HEIGHT - 23;

	for (i=0 ; i<l ; i++)
	{
		k = fragsort[i];
		s = &cl.q1_players[k];
		if (!s->name[0])
			continue;

	// draw background
		top = s->topcolor << 4;
		bottom = s->bottomcolor << 4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		UI_FillPal (xofs + x*8 + 10, y, 28, 4, top);
		UI_FillPal (xofs + x*8 + 10, y+4, 28, 3, bottom);

	// draw number
		f = s->frags;
		sprintf (num, "%3i",f);

		Sbar_DrawCharacter ( (x+1)*8 , -24, num[0]);
		Sbar_DrawCharacter ( (x+2)*8 , -24, num[1]);
		Sbar_DrawCharacter ( (x+3)*8 , -24, num[2]);

		if (k == cl.viewentity - 1)
		{
			Sbar_DrawCharacter (x*8+2, -24, 16);
			Sbar_DrawCharacter ( (x+4)*8-4, -24, 17);
		}
		x+=4;
	}
}

//=============================================================================


/*
===============
Sbar_DrawFace
===============
*/
void Sbar_DrawFace (void)
{
	int		f, anim;

// PGM 01/19/97 - team color drawing
// PGM 03/02/97 - fixed so color swatch only appears in CTF modes
	if (rogue &&
        (cl.qh_maxclients != 1) &&
        (teamplay->value>3) &&
        (teamplay->value<7))
	{
		int				top, bottom;
		int				xofs;
		char			num[12];
		q1player_info_t	*s;
		
		s = &cl.q1_players[cl.viewentity - 1];
		// draw background
		top = s->topcolor << 4;
		bottom = s->bottomcolor << 4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		if (cl.qh_gametype == GAME_DEATHMATCH)
			xofs = 113;
		else
			xofs = ((viddef.width - 320)>>1) + 113;

		Sbar_DrawPic (112, 0, rsb_teambord);
		UI_FillPal (xofs, viddef.height-SBAR_HEIGHT+3, 22, 9, top);
		UI_FillPal (xofs, viddef.height-SBAR_HEIGHT+12, 22, 9, bottom);

		// draw number
		f = s->frags;
		sprintf (num, "%3i",f);

		if (top==8)
		{
			if (num[0] != ' ')
				Sbar_DrawCharacter(109, 3, 18 + num[0] - '0');
			if (num[1] != ' ')
				Sbar_DrawCharacter(116, 3, 18 + num[1] - '0');
			if (num[2] != ' ')
				Sbar_DrawCharacter(123, 3, 18 + num[2] - '0');
		}
		else
		{
			Sbar_DrawCharacter ( 109, 3, num[0]);
			Sbar_DrawCharacter ( 116, 3, num[1]);
			Sbar_DrawCharacter ( 123, 3, num[2]);
		}
		
		return;
	}
// PGM 01/19/97 - team color drawing

	if ( (cl.q1_items & (IT_INVISIBILITY | IT_INVULNERABILITY) )
	== (IT_INVISIBILITY | IT_INVULNERABILITY) )
	{
		Sbar_DrawPic (112, 0, sb_face_invis_invuln);
		return;
	}
	if (cl.q1_items & IT_QUAD)
	{
		Sbar_DrawPic (112, 0, sb_face_quad );
		return;
	}
	if (cl.q1_items & IT_INVISIBILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invis );
		return;
	}
	if (cl.q1_items & IT_INVULNERABILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invuln);
		return;
	}

	if (cl.qh_stats[STAT_HEALTH] >= 100)
		f = 4;
	else
		f = cl.qh_stats[STAT_HEALTH] / 20;

	if (cl.qh_serverTimeFloat <= cl.q1_faceanimtime)
	{
		anim = 1;
	}
	else
		anim = 0;
	Sbar_DrawPic (112, 0, sb_faces[f][anim]);
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw (void)
{
	if (scr_con_current == viddef.height)
		return;		// console is full screen

	if (sb_lines && viddef.width > 320) 
		UI_TileClear(0, viddef.height - sb_lines, viddef.width, sb_lines, draw_backtile);

	if (sb_lines > 24)
	{
		Sbar_DrawInventory ();
		if (cl.qh_maxclients != 1)
			Sbar_DrawFrags ();
	}

	if (sb_showscores || cl.qh_stats[STAT_HEALTH] <= 0)
	{
		Sbar_DrawPic (0, 0, sb_scorebar);
		Sbar_DrawScoreboard ();
	}
	else if (sb_lines)
	{
		Sbar_DrawPic (0, 0, sb_sbar);

   // keys (hipnotic only)
      //MED 01/04/97 moved keys here so they would not be overwritten
      if (hipnotic)
      {
         if (cl.q1_items & IT_KEY1)
            Sbar_DrawPic (209, 3, sb_items[0]);
         if (cl.q1_items & IT_KEY2)
            Sbar_DrawPic (209, 12, sb_items[1]);
      }
   // armor
		if (cl.q1_items & IT_INVULNERABILITY)
		{
			Sbar_DrawNum (24, 0, 666, 3, 1);
			Sbar_DrawPic (0, 0, draw_disc);
		}
		else
		{
			if (rogue)
			{
				Sbar_DrawNum (24, 0, cl.qh_stats[STAT_ARMOR], 3,
								cl.qh_stats[STAT_ARMOR] <= 25);
				if (cl.q1_items & RIT_ARMOR3)
					Sbar_DrawPic (0, 0, sb_armor[2]);
				else if (cl.q1_items & RIT_ARMOR2)
					Sbar_DrawPic (0, 0, sb_armor[1]);
				else if (cl.q1_items & RIT_ARMOR1)
					Sbar_DrawPic (0, 0, sb_armor[0]);
			}
			else
			{
				Sbar_DrawNum (24, 0, cl.qh_stats[STAT_ARMOR], 3
				, cl.qh_stats[STAT_ARMOR] <= 25);
				if (cl.q1_items & IT_ARMOR3)
					Sbar_DrawPic (0, 0, sb_armor[2]);
				else if (cl.q1_items & IT_ARMOR2)
					Sbar_DrawPic (0, 0, sb_armor[1]);
				else if (cl.q1_items & IT_ARMOR1)
					Sbar_DrawPic (0, 0, sb_armor[0]);
			}
		}

	// face
		Sbar_DrawFace ();

	// health
		Sbar_DrawNum (136, 0, cl.qh_stats[STAT_HEALTH], 3
		, cl.qh_stats[STAT_HEALTH] <= 25);

	// ammo icon
		if (rogue)
		{
			if (cl.q1_items & RIT_SHELLS)
				Sbar_DrawPic (224, 0, sb_ammo[0]);
			else if (cl.q1_items & RIT_NAILS)
				Sbar_DrawPic (224, 0, sb_ammo[1]);
			else if (cl.q1_items & RIT_ROCKETS)
				Sbar_DrawPic (224, 0, sb_ammo[2]);
			else if (cl.q1_items & RIT_CELLS)
				Sbar_DrawPic (224, 0, sb_ammo[3]);
			else if (cl.q1_items & RIT_LAVA_NAILS)
				Sbar_DrawPic (224, 0, rsb_ammo[0]);
			else if (cl.q1_items & RIT_PLASMA_AMMO)
				Sbar_DrawPic (224, 0, rsb_ammo[1]);
			else if (cl.q1_items & RIT_MULTI_ROCKETS)
				Sbar_DrawPic (224, 0, rsb_ammo[2]);
		}
		else
		{
			if (cl.q1_items & IT_SHELLS)
				Sbar_DrawPic (224, 0, sb_ammo[0]);
			else if (cl.q1_items & IT_NAILS)
				Sbar_DrawPic (224, 0, sb_ammo[1]);
			else if (cl.q1_items & IT_ROCKETS)
				Sbar_DrawPic (224, 0, sb_ammo[2]);
			else if (cl.q1_items & IT_CELLS)
				Sbar_DrawPic (224, 0, sb_ammo[3]);
		}

		Sbar_DrawNum (248, 0, cl.qh_stats[STAT_AMMO], 3,
					  cl.qh_stats[STAT_AMMO] <= 10);
	}

	if (viddef.width > 320) {
		if (cl.qh_gametype == GAME_DEATHMATCH)
			Sbar_MiniDeathmatchOverlay ();
	}
}

//=============================================================================

/*
==================
Sbar_IntermissionNumber

==================
*/
void Sbar_IntermissionNumber (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		UI_DrawPic (x,y,sb_nums[color][frame]);
		x += 24;
		ptr++;
	}
}

/*
==================
Sbar_DeathmatchOverlay

==================
*/
void Sbar_DeathmatchOverlay (void)
{
	image_t			*pic;
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	char			num[12];
	q1player_info_t	*s;

	pic = R_CachePic ("gfx/ranking.lmp");
	M_DrawPic ((320-R_GetImageWidth(pic))/2, 8, pic);

// scores
	Sbar_SortFrags ();

// draw the text
	l = scoreboardlines;

	x = 80 + ((viddef.width - 320)>>1);
	y = 40;
	for (i=0 ; i<l ; i++)
	{
		k = fragsort[i];
		s = &cl.q1_players[k];
		if (!s->name[0])
			continue;

	// draw background
		top = s->topcolor << 4;
		bottom = s->bottomcolor << 4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		UI_FillPal ( x, y, 40, 4, top);
		UI_FillPal ( x, y+4, 40, 4, bottom);

	// draw number
		f = s->frags;
		sprintf (num, "%3i",f);

		Draw_Character ( x+8 , y, num[0]);
		Draw_Character ( x+16 , y, num[1]);
		Draw_Character ( x+24 , y, num[2]);

		if (k == cl.viewentity - 1)
			Draw_Character ( x - 8, y, 12);

#if 0
{
	int				total;
	int				n, minutes, tens, units;

	// draw time
		total = cl.completed_time - s->entertime;
		minutes = (int)total/60;
		n = total - minutes*60;
		tens = n/10;
		units = n%10;

		sprintf (num, "%3i:%i%i", minutes, tens, units);

		Draw_String ( x+48 , y, num);
}
#endif

	// draw name
		Draw_String (x+64, y, s->name);

		y += 10;
	}
}

/*
==================
Sbar_DeathmatchOverlay

==================
*/
void Sbar_MiniDeathmatchOverlay (void)
{
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	char			num[12];
	q1player_info_t	*s;
	int				numlines;

	if (viddef.width < 512 || !sb_lines)
		return;

// scores
	Sbar_SortFrags ();

// draw the text
	l = scoreboardlines;
	y = viddef.height - sb_lines;
	numlines = sb_lines/8;
	if (numlines < 3)
		return;

	//find us
	for (i = 0; i < scoreboardlines; i++)
		if (fragsort[i] == cl.viewentity - 1)
			break;

    if (i == scoreboardlines) // we're not there
            i = 0;
    else // figure out start
            i = i - numlines/2;

    if (i > scoreboardlines - numlines)
            i = scoreboardlines - numlines;
    if (i < 0)
            i = 0;

	x = 324;
	for (/* */; i < scoreboardlines && y < (int)viddef.height - 8 ; i++)
	{
		k = fragsort[i];
		s = &cl.q1_players[k];
		if (!s->name[0])
			continue;

	// draw background
		top = s->topcolor << 4;
		bottom = s->bottomcolor << 4;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		UI_FillPal ( x, y+1, 40, 3, top);
		UI_FillPal ( x, y+4, 40, 4, bottom);

	// draw number
		f = s->frags;
		sprintf (num, "%3i",f);

		Draw_Character ( x+8 , y, num[0]);
		Draw_Character ( x+16 , y, num[1]);
		Draw_Character ( x+24 , y, num[2]);

		if (k == cl.viewentity - 1) {
			Draw_Character ( x, y, 16);
			Draw_Character ( x + 32, y, 17);
		}

#if 0
{
	int				total;
	int				n, minutes, tens, units;

	// draw time
		total = cl.completed_time - s->entertime;
		minutes = (int)total/60;
		n = total - minutes*60;
		tens = n/10;
		units = n%10;

		sprintf (num, "%3i:%i%i", minutes, tens, units);

		Draw_String ( x+48 , y, num);
}
#endif

	// draw name
		Draw_String (x+48, y, s->name);

		y += 8;
	}
}

/*
==================
Sbar_IntermissionOverlay

==================
*/
void Sbar_IntermissionOverlay (void)
{
	image_t	*pic;
	int		dig;
	int		num;

	if (cl.qh_gametype == GAME_DEATHMATCH)
	{
		Sbar_DeathmatchOverlay ();
		return;
	}

	pic = R_CachePic ("gfx/complete.lmp");
	UI_DrawPic (64, 24, pic);

	pic = R_CachePic ("gfx/inter.lmp");
	UI_DrawPic (0, 56, pic);

// time
	dig = cl.qh_completed_time/60;
	Sbar_IntermissionNumber (160, 64, dig, 3, 0);
	num = cl.qh_completed_time - dig*60;
	UI_DrawPic (234,64,sb_colon);
	UI_DrawPic (246,64,sb_nums[0][num/10]);
	UI_DrawPic (266,64,sb_nums[0][num%10]);

	Sbar_IntermissionNumber (160, 104, cl.qh_stats[STAT_SECRETS], 3, 0);
	UI_DrawPic (232,104,sb_slash);
	Sbar_IntermissionNumber (240, 104, cl.qh_stats[STAT_TOTALSECRETS], 3, 0);

	Sbar_IntermissionNumber (160, 144, cl.qh_stats[STAT_MONSTERS], 3, 0);
	UI_DrawPic (232,144,sb_slash);
	Sbar_IntermissionNumber (240, 144, cl.qh_stats[STAT_TOTALMONSTERS], 3, 0);

}


/*
==================
Sbar_FinaleOverlay

==================
*/
void Sbar_FinaleOverlay (void)
{
	image_t	*pic;

	pic = R_CachePic ("gfx/finale.lmp");
	UI_DrawPic ( (viddef.width-R_GetImageWidth(pic))/2, 16, pic);
}
