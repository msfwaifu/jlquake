// cl.input.c  -- builds an intended movement command to send to the server

#include "quakedef.h"

Cvar*	cl_nodelta;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t	in_mlook, in_klook;
kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed, in_use, in_jump, in_attack;
kbutton_t	in_up, in_down, in_crouch;

int			in_impulse;

static Cvar*	m_filter;

static int	mouse_move_x;
static int	mouse_move_y;
static int	old_mouse_x, old_mouse_y;

void KeyDown (kbutton_t *b)
{
	int		k;
	char	*c;
	
	c = Cmd_Argv(1);
	if (c[0])
		k = String::Atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key
	
	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Con_Printf ("Three keys down for a button!\n");
		return;
	}
	
	if (b->state & 1)
		return;		// still down
	b->state |= 1 + 2;	// down + impulse down
}

void KeyUp (kbutton_t *b)
{
	int		k;
	char	*c;
	
	c = Cmd_Argv(1);
	if (c[0])
		k = String::Atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)
	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void IN_KLookDown (void) {KeyDown(&in_klook);}
void IN_KLookUp (void) {KeyUp(&in_klook);}
void IN_MLookDown (void) {KeyDown(&in_mlook);}
void IN_MLookUp (void) {
KeyUp(&in_mlook);
if ( !(in_mlook.state&1) &&  lookspring->value)
	V_StartPitchDrift();
}
void IN_UpDown(void) {KeyDown(&in_up);}
void IN_UpUp(void) {KeyUp(&in_up);}
void IN_DownDown(void) {KeyDown(&in_down);}
void IN_DownUp(void) {KeyUp(&in_down);}
void IN_LeftDown(void) {KeyDown(&in_left);}
void IN_LeftUp(void) {KeyUp(&in_left);}
void IN_RightDown(void) {KeyDown(&in_right);}
void IN_RightUp(void) {KeyUp(&in_right);}
void IN_ForwardDown(void) {KeyDown(&in_forward);}
void IN_ForwardUp(void) {KeyUp(&in_forward);}
void IN_BackDown(void) {KeyDown(&in_back);}
void IN_BackUp(void) {KeyUp(&in_back);}
void IN_LookupDown(void) {KeyDown(&in_lookup);}
void IN_LookupUp(void) {KeyUp(&in_lookup);}
void IN_LookdownDown(void) {KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {KeyUp(&in_lookdown);}
void IN_MoveleftDown(void) {KeyDown(&in_moveleft);}
void IN_MoveleftUp(void) {KeyUp(&in_moveleft);}
void IN_MoverightDown(void) {KeyDown(&in_moveright);}
void IN_MoverightUp(void) {KeyUp(&in_moveright);}

void IN_SpeedDown(void) {KeyDown(&in_speed);}
void IN_SpeedUp(void) {KeyUp(&in_speed);}
void IN_StrafeDown(void) {KeyDown(&in_strafe);}
void IN_StrafeUp(void) {KeyUp(&in_strafe);}

void IN_AttackDown(void) {KeyDown(&in_attack);}
void IN_AttackUp(void) {KeyUp(&in_attack);}

void IN_UseDown (void) {KeyDown(&in_use);}
void IN_UseUp (void) {KeyUp(&in_use);}
void IN_JumpDown (void) {KeyDown(&in_jump);}
void IN_JumpUp (void) {KeyUp(&in_jump);}

void IN_Impulse (void) {in_impulse=String::Atoi(Cmd_Argv(1));}

void IN_CrouchDown (void)
{
	int state;

	if (in_keyCatchers == 0)
	{
		state = in_crouch.state;
		KeyDown(&in_crouch);
	}
}

void IN_CrouchUp (void)
{
	int state;

	if (in_keyCatchers == 0)
	{
		state = in_crouch.state;

		KeyUp(&in_crouch);
	}
}

/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CL_KeyState (kbutton_t *key)
{
	float		val;
	qboolean	impulsedown, impulseup, down;
	
	impulsedown = key->state & 2;
	impulseup = key->state & 4;
	down = key->state & 1;
	val = 0;
	
	if (impulsedown && !impulseup)
	{
		if (down)
			val = 0.5;	// pressed and held this frame
		else
			val = 0;	//	I_Error ();
	}
	if (impulseup && !impulsedown)
	{
		if (down)
			val = 0;	//	I_Error ();
		else
			val = 0;	// released this frame
	}
	if (!impulsedown && !impulseup)
	{
		if (down)
			val = 1.0;	// held the entire frame
		else
			val = 0;	// up the entire frame
	}
	if (impulsedown && impulseup)
	{
		if (down)
			val = 0.75;	// released and re-pressed this frame
		else
			val = 0.25;	// pressed and released this frame
	}

	key->state &= 1;		// clear impulses
	
	return val;
}




//==========================================================================

Cvar*	cl_upspeed;
Cvar*	cl_forwardspeed;
Cvar*	cl_backspeed;
Cvar*	cl_sidespeed;

Cvar*	cl_movespeedkey;

Cvar*	cl_yawspeed;
Cvar*	cl_pitchspeed;

Cvar*	cl_anglespeedkey;


/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles (void)
{
	float	speed;
	float	up, down;
	
	if ((in_speed.state & 1) || cl.spectator)
		speed = host_frametime * cl_anglespeedkey->value;
	else
		speed = host_frametime;

	if (!(in_strafe.state & 1))
	{
		cl.viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
		cl.viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);
		cl.viewangles[YAW] = AngleMod(cl.viewangles[YAW]);
	}
	if (in_klook.state & 1)
	{
		V_StopPitchDrift ();
		cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_forward);
		cl.viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_back);
	}
	
	// FIXME: This is a cheap way of doing this, it belongs in V_CalcViewRoll
	// but I don't see where I can get the yaw velocity, I have to get on to other things so here it is

	if ((CL_KeyState (&in_left)!=0) && (cl.v.movetype==MOVETYPE_FLY))
		cl.idealroll=-10;
	else if ((CL_KeyState (&in_right)!=0) && (cl.v.movetype==MOVETYPE_FLY))
		cl.idealroll=10;
	else
		cl.idealroll=0;

	up = CL_KeyState (&in_lookup);
	down = CL_KeyState(&in_lookdown);
	
	cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * up;
	cl.viewangles[PITCH] += speed*cl_pitchspeed->value * down;

	if (up || down)
		V_StopPitchDrift ();
		
	if (cl.viewangles[PITCH] > 80)
		cl.viewangles[PITCH] = 80;
	if (cl.viewangles[PITCH] < -70)
		cl.viewangles[PITCH] = -70;

	if (cl.viewangles[ROLL] > 50)
		cl.viewangles[ROLL] = 50;
	if (cl.viewangles[ROLL] < -50)
		cl.viewangles[ROLL] = -50;
		
}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove (hwusercmd_t *cmd)
{	
	CL_AdjustAngles ();
	
	Com_Memset(cmd, 0, sizeof(*cmd));

	if (cl.v.cameramode)	// Stuck in a different camera so don't move
	{
		return;
	}
	
	VectorCopy (cl.viewangles, cmd->angles);
	if (in_strafe.state & 1)
	{
//		cmd->sidemove += cl_sidespeed.value * CL_KeyState (&in_right);
//		cmd->sidemove -= cl_sidespeed.value * CL_KeyState (&in_left);
		cmd->sidemove += 225 * CL_KeyState (&in_right);
		cmd->sidemove -= 225 * CL_KeyState (&in_left);
	}

//	cmd->sidemove += cl_sidespeed.value * CL_KeyState (&in_moveright);
//	cmd->sidemove -= cl_sidespeed.value * CL_KeyState (&in_moveleft);
	cmd->sidemove += 225 * CL_KeyState (&in_moveright);
	cmd->sidemove -= 225 * CL_KeyState (&in_moveleft);

	cmd->upmove += cl_upspeed->value * CL_KeyState (&in_up);
	cmd->upmove -= cl_upspeed->value * CL_KeyState (&in_down);

	if (! (in_klook.state & 1) )
	{	
//		cmd->forwardmove += cl_forwardspeed.value * CL_KeyState (&in_forward);
//		cmd->forwardmove -= cl_backspeed.value * CL_KeyState (&in_back);
		cmd->forwardmove += 200 * CL_KeyState (&in_forward);
		cmd->forwardmove -= 200 * CL_KeyState (&in_back);
	}	

//
// adjust for speed key
//
	if ((cl.spectator || cl_forwardspeed->value > 200 || in_speed.state & 1) && cl.v.hasted <= 1)
	{
		cmd->forwardmove *= cl_movespeedkey->value;
		cmd->sidemove *= cl_movespeedkey->value;
		cmd->upmove *= cl_movespeedkey->value;
	}	

	// Hasted player?
	if (cl.v.hasted)
	{
		cmd->forwardmove = cmd->forwardmove * cl.v.hasted;
		cmd->sidemove = cmd->sidemove * cl.v.hasted;
		cmd->upmove = cmd->upmove * cl.v.hasted;
	}

	cmd->light_level = (byte)cl_lightlevel->value;
}

void CL_MouseEvent(int mx, int my)
{
	mouse_move_x += mx;
	mouse_move_y += my;
}

void CL_MouseMove(hwusercmd_t *cmd)
{
	if (cl.v.cameramode)	// Stuck in a different camera so don't move
	{
		Com_Memset(cmd, 0, sizeof(*cmd));
		return;
	}

	int mouse_x = mouse_move_x;
	int mouse_y = mouse_move_y;
	if (m_filter->value)
	{
		mouse_x = (mouse_x + old_mouse_x) * 0.5;
		mouse_y = (mouse_y + old_mouse_y) * 0.5;
	}

	old_mouse_x = mouse_move_x;
	old_mouse_y = mouse_move_y;

	mouse_x *= sensitivity->value;
	mouse_y *= sensitivity->value;

// add mouse X/Y movement to cmd
	if ( (in_strafe.state & 1) || (lookstrafe->value && (in_mlook.state & 1) ))
		cmd->sidemove += m_side->value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw->value * mouse_x;

	if (in_mlook.state & 1)
		V_StopPitchDrift ();
		
	if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch->value * mouse_y;
		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	}
	else
	{
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward->value * mouse_y;
		else
			cmd->forwardmove -= m_forward->value * mouse_y;
	}

	if (cl.idealroll == 0) // Did keyboard set it already??
	{
		if ((mouse_x <0) && (cl.v.movetype==MOVETYPE_FLY))
			cl.idealroll=-10;
		else if ((mouse_x >0) && (cl.v.movetype==MOVETYPE_FLY))
			cl.idealroll=10;
	}
	mouse_move_x = 0;
	mouse_move_y = 0;
}

int MakeChar (int i)
{
	i &= ~3;
	if (i < -127*4)
		i = -127*4;
	if (i > 127*4)
		i = 127*4;
	return i;
}

/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove (hwusercmd_t *cmd)
{
	int		i;
	int		ms;

//
// allways dump the first two message, because it may contain leftover inputs
// from the last level
//
	if (++cl.movemessages <= 2)
		return;
//
// figure button bits
//	
	if ( in_attack.state & 3 )
		cmd->buttons |= 1;
	in_attack.state &= ~2;
	
	if (in_jump.state & 3)
		cmd->buttons |= 2;
	in_jump.state &= ~2;

	if (in_crouch.state & 1)
		cmd->buttons |= 4;

	// send milliseconds of time to apply the move
	ms = host_frametime * 1000;
	if (ms > 250)
		ms = 100;		// time was unreasonable
	cmd->msec = ms;

	VectorCopy (cl.viewangles, cmd->angles);

	cmd->impulse = in_impulse;
	in_impulse = 0;


//
// chop down so no extra bits are kept that the server wouldn't get
//
	if ((int)cl.v.artifact_active & ARTFLAG_FROZEN)
	{
		cmd->forwardmove = 0;
		cmd->sidemove = 0;
		cmd->upmove = 0;
	}
	else
	{
		cmd->forwardmove = MakeChar (cmd->forwardmove);
		cmd->sidemove = MakeChar (cmd->sidemove);
		cmd->upmove = MakeChar (cmd->upmove);
	}

	for (i=0 ; i<3 ; i++)
		cmd->angles[i] = ((int)(cmd->angles[i]*65536.0/360)&65535) * (360.0/65536.0);
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	QMsg		buf;
	byte		data[128];
	int			i;
	hwusercmd_t	*cmd;

	if (clc.demoplaying)
		return; // sendcmds come from the demo

	// save this command off for prediction
	i = clc.netchan.outgoingSequence & UPDATE_MASK_HW;
	cmd = &cl.hw_frames[i].cmd;
	cl.hw_frames[i].senttime = realtime;
	cl.hw_frames[i].receivedtime = -1;		// we haven't gotten a reply yet

	// get basic movement from keyboard
	CL_BaseMove (cmd);

	// allow mice or other external controllers to add to the move
	CL_MouseMove(cmd);

	// if we are spectator, try autocam
	if (cl.spectator)
		Cam_Track(cmd);

	CL_FinishMove(cmd);

	Cam_FinishMove(cmd);

// send this and the previous cmds in the message, so
// if the last packet was dropped, it can be recovered
	buf.InitOOB(data, 128);

	buf.WriteByte(h2clc_move);
	i = (clc.netchan.outgoingSequence-2) & UPDATE_MASK_HW;
	MSG_WriteUsercmd (&buf, &cl.hw_frames[i].cmd, false);
	i = (clc.netchan.outgoingSequence-1) & UPDATE_MASK_HW;
	MSG_WriteUsercmd (&buf, &cl.hw_frames[i].cmd, false);
	i = (clc.netchan.outgoingSequence) & UPDATE_MASK_HW;
	MSG_WriteUsercmd (&buf, &cl.hw_frames[i].cmd, true);

//	Con_Printf("I  %hd %hd %hd\n",cmd->forwardmove, cmd->sidemove, cmd->upmove);

	// request delta compression of entities
	if (clc.netchan.outgoingSequence - cl.validsequence >= UPDATE_BACKUP_HW-1)
		cl.validsequence = 0;

	if (cl.validsequence && !cl_nodelta->value && cls.state == CA_ACTIVE &&
		!clc.demorecording)
	{
		cl.hw_frames[clc.netchan.outgoingSequence&UPDATE_MASK_HW].delta_sequence = cl.validsequence;
		buf.WriteByte(hwclc_delta);
		buf.WriteByte(cl.validsequence&255);
	}
	else
		cl.hw_frames[clc.netchan.outgoingSequence&UPDATE_MASK_HW].delta_sequence = -1;

	if (clc.demorecording)
		CL_WriteDemoCmd(cmd);

//
// deliver the message
//
	Netchan_Transmit (&clc.netchan, buf.cursize, buf._data);	
}



/*
===========
Force_CenterView_f
===========
*/
void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	Cmd_AddCommand ("+moveup",IN_UpDown);
	Cmd_AddCommand ("-moveup",IN_UpUp);
	Cmd_AddCommand ("+movedown",IN_DownDown);
	Cmd_AddCommand ("-movedown",IN_DownUp);
	Cmd_AddCommand ("+left",IN_LeftDown);
	Cmd_AddCommand ("-left",IN_LeftUp);
	Cmd_AddCommand ("+right",IN_RightDown);
	Cmd_AddCommand ("-right",IN_RightUp);
	Cmd_AddCommand ("+forward",IN_ForwardDown);
	Cmd_AddCommand ("-forward",IN_ForwardUp);
	Cmd_AddCommand ("+back",IN_BackDown);
	Cmd_AddCommand ("-back",IN_BackUp);
	Cmd_AddCommand ("+lookup", IN_LookupDown);
	Cmd_AddCommand ("-lookup", IN_LookupUp);
	Cmd_AddCommand ("+lookdown", IN_LookdownDown);
	Cmd_AddCommand ("-lookdown", IN_LookdownUp);
	Cmd_AddCommand ("+strafe", IN_StrafeDown);
	Cmd_AddCommand ("-strafe", IN_StrafeUp);
	Cmd_AddCommand ("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand ("+moveright", IN_MoverightDown);
	Cmd_AddCommand ("-moveright", IN_MoverightUp);
	Cmd_AddCommand ("+speed", IN_SpeedDown);
	Cmd_AddCommand ("-speed", IN_SpeedUp);
	Cmd_AddCommand ("+attack", IN_AttackDown);
	Cmd_AddCommand ("-attack", IN_AttackUp);
	Cmd_AddCommand ("+use", IN_UseDown);
	Cmd_AddCommand ("-use", IN_UseUp);
	Cmd_AddCommand ("+jump", IN_JumpDown);
	Cmd_AddCommand ("-jump", IN_JumpUp);
	Cmd_AddCommand ("impulse", IN_Impulse);
	Cmd_AddCommand ("+klook", IN_KLookDown);
	Cmd_AddCommand ("-klook", IN_KLookUp);
	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);
	Cmd_AddCommand ("+crouch", IN_CrouchDown);
	Cmd_AddCommand ("-crouch", IN_CrouchUp);
	Cmd_AddCommand ("force_centerview", Force_CenterView_f);

	cl_nodelta = Cvar_Get("cl_nodelta","0", 0);
    m_filter = Cvar_Get("m_filter", "0", 0);
}

/*
============
CL_ClearStates
============
*/
void CL_ClearStates (void)
{
/*	in_mlook.state = 0;
	in_klook.state = 0;
	in_left.state = 0;
	in_right.state = 0;
	in_forward.state = 0;
	in_back.state = 0;
	in_lookup.state = 0;
	in_lookdown.state = 0;
	in_moveleft.state = 0;
	in_moveright.state = 0;
	in_strafe.state = 0;
	in_speed.state = 0;
	in_use.state = 0;
	in_jump.state = 0;
	in_attack.state = 0;
	in_up.state = 0;
	in_down.state = 0;
	in_crouch.state = 0;*/
}

