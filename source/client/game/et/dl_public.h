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

#ifndef _DL_PUBLIC_H_
#define _DL_PUBLIC_H_

enum dlStatus_t
{
	DL_CONTINUE,
	DL_DONE,
	DL_FAILED
};

bool DL_BeginDownload( const char* localName, const char* remoteName, int debug );
dlStatus_t DL_DownloadLoop();
void DL_Shutdown();

#endif
