/***************************************************************************
                          dsound.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2005/08/29 - Pete
// - changed to 48Khz output
//
// 2003/01/12 - Pete
// - added recording funcs
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_DSOUND

#include "externals.h"

#ifdef _WINDOWS
#define _LPCWAVEFORMATEX_DEFINED
#include <dsound.h>
                                
#include "record.h"
                   
////////////////////////////////////////////////////////////////////////
// dsound globals
////////////////////////////////////////////////////////////////////////

LPDIRECTSOUND lpDS;
LPDIRECTSOUNDBUFFER lpDSBP = NULL;
LPDIRECTSOUNDBUFFER lpDSB = NULL;
DSBUFFERDESC        dsbd;
DSBUFFERDESC        dsbdesc;
DSCAPS              dscaps;
DSBCAPS             dsbcaps;

unsigned long LastWrite=0xffffffff;
unsigned long LastPlay=0;

////////////////////////////////////////////////////////////////////////
// SETUP SOUND
////////////////////////////////////////////////////////////////////////

void SetupSound(void)
{
 HRESULT dsval;WAVEFORMATEX pcmwf;

 dsval = DirectSoundCreate(NULL,&lpDS,NULL);
 if(dsval!=DS_OK) 
  {
   MessageBox(hWMain,"DirectSoundCreate!","Error",MB_OK);
   return;
  }

 if(DS_OK!=IDirectSound_SetCooperativeLevel(lpDS,hWMain, DSSCL_PRIORITY))
  {
   if(DS_OK!=IDirectSound_SetCooperativeLevel(lpDS,hWMain, DSSCL_NORMAL))
    {
     MessageBox(hWMain,"SetCooperativeLevel!","Error",MB_OK);
     return;
    }
  }

 memset(&dsbd,0,sizeof(DSBUFFERDESC));
 dsbd.dwSize = 20;                                     // NT4 hack! sizeof(dsbd);
 dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;                 
 dsbd.dwBufferBytes = 0; 
 dsbd.lpwfxFormat = NULL;

 dsval=IDirectSound_CreateSoundBuffer(lpDS,&dsbd,&lpDSBP,NULL);
 if(dsval!=DS_OK) 
  {
   MessageBox(hWMain, "CreateSoundBuffer (Primary)", "Error",MB_OK);
   return;
  }

 memset(&pcmwf, 0, sizeof(WAVEFORMATEX));
 pcmwf.wFormatTag = WAVE_FORMAT_PCM;

 if(iDisStereo) {pcmwf.nChannels = 1; pcmwf.nBlockAlign = 2;}
 else           {pcmwf.nChannels = 2; pcmwf.nBlockAlign = 4;}

 pcmwf.nSamplesPerSec = 48000;
 
 pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
 pcmwf.wBitsPerSample = 16;

 dsval=IDirectSoundBuffer_SetFormat(lpDSBP,&pcmwf);
 if(dsval!=DS_OK) 
  {
   MessageBox(hWMain, "SetFormat!", "Error",MB_OK);
   return;
  }

 dscaps.dwSize = sizeof(DSCAPS);
 dsbcaps.dwSize = sizeof(DSBCAPS);
 IDirectSound_GetCaps(lpDS,&dscaps);
 IDirectSoundBuffer_GetCaps(lpDSBP,&dsbcaps);

 memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
 dsbdesc.dwSize = 20;                                  // NT4 hack! sizeof(DSBUFFERDESC);
 dsbdesc.dwFlags = DSBCAPS_LOCSOFTWARE | DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
 dsbdesc.dwBufferBytes = SOUNDSIZE;
 dsbdesc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;

 dsval=IDirectSound_CreateSoundBuffer(lpDS,&dsbdesc,&lpDSB,NULL);
 if(dsval!=DS_OK) 
  {
   MessageBox(hWMain,"CreateSoundBuffer (Secondary)", "Error",MB_OK);
   return;
  }

 dsval=IDirectSoundBuffer_Play(lpDSBP,0,0,DSBPLAY_LOOPING);
 if(dsval!=DS_OK) 
  {
   MessageBox(hWMain,"Play (Primary)","Error",MB_OK);
   return;
  }

 dsval=IDirectSoundBuffer_Play(lpDSB,0,0,DSBPLAY_LOOPING);
 if(dsval!=DS_OK) 
  {
   MessageBox(hWMain,"Play (Secondary)","Error",MB_OK);
   return;
  }
}

////////////////////////////////////////////////////////////////////////
// REMOVE SOUND
////////////////////////////////////////////////////////////////////////

void RemoveSound(void)
{ 
 int iRes;

 if(iDoRecord) RecordStop();

 if(lpDSB!=NULL) 
  {
   IDirectSoundBuffer_Stop(lpDSB);
   iRes=IDirectSoundBuffer_Release(lpDSB);
   // FF says such a loop is bad... Demo says it's good... Pete doesn't care
   while(iRes!=0) iRes=IDirectSoundBuffer_Release(lpDSB);
   lpDSB=NULL;
  }

 if(lpDSBP!=NULL) 
  {
   IDirectSoundBuffer_Stop(lpDSBP);
   iRes=IDirectSoundBuffer_Release(lpDSBP);
   // FF says such a loop is bad... Demo says it's good... Pete doesn't care
   while(iRes!=0) iRes=IDirectSoundBuffer_Release(lpDSBP);
   lpDSBP=NULL;
  }

 if(lpDS!=NULL) 
  {
   iRes=IDirectSound_Release(lpDS);
   // FF says such a loop is bad... Demo says it's good... Pete doesn't care
   while(iRes!=0) iRes=IDirectSound_Release(lpDS);
   lpDS=NULL;
  }

}

////////////////////////////////////////////////////////////////////////
// GET BYTES BUFFERED
////////////////////////////////////////////////////////////////////////

unsigned long SoundGetBytesBuffered(void)
{
 unsigned long cplay,cwrite;

 if(LastWrite==0xffffffff) return 0;

 IDirectSoundBuffer_GetCurrentPosition(lpDSB,&cplay,&cwrite);

 if(cplay>SOUNDSIZE) return SOUNDSIZE;

 if(cplay<LastWrite) return LastWrite-cplay;
 return (SOUNDSIZE-cplay)+LastWrite;
}

////////////////////////////////////////////////////////////////////////
// FEED SOUND DATA
////////////////////////////////////////////////////////////////////////

void SoundFeedStreamData(unsigned char* pSound,long lBytes)
{
 LPVOID lpvPtr1, lpvPtr2;
 unsigned long dwBytes1,dwBytes2; 
 unsigned long *lpSS, *lpSD;
 unsigned long dw,cplay,cwrite;
 HRESULT hr;
 unsigned long status;

 if(iDoRecord) RecordBuffer(pSound,lBytes);

 IDirectSoundBuffer_GetStatus(lpDSB,&status);
 if(status&DSBSTATUS_BUFFERLOST)
  {
   if(IDirectSoundBuffer_Restore(lpDSB)!=DS_OK) return;
   IDirectSoundBuffer_Play(lpDSB,0,0,DSBPLAY_LOOPING);
  }

 IDirectSoundBuffer_GetCurrentPosition(lpDSB,&cplay,&cwrite);

 if(LastWrite==0xffffffff) LastWrite=cwrite;

/*
// mmm... security... not needed, I think
 if(LastWrite<cplay)
  {
   if((cplay-LastWrite)<=(unsigned long)lBytes)
    {
     LastWrite=0xffffffff;
     return;
    }
  }
 else
  {
   if(LastWrite<cwrite)
    {
     LastWrite=0xffffffff;
     return;
    }
  }
*/

 hr=IDirectSoundBuffer_Lock(lpDSB,LastWrite,lBytes,
                &lpvPtr1, &dwBytes1, 
                &lpvPtr2, &dwBytes2,
                0);

 if(hr!=DS_OK) {LastWrite=0xffffffff;return;}

 lpSD=(unsigned long *)lpvPtr1;
 dw=dwBytes1>>2;

 lpSS=(unsigned long *)pSound;
 while(dw) {*lpSD++=*lpSS++;dw--;}

 if(lpvPtr2)
  {
   lpSD=(unsigned long *)lpvPtr2;
   dw=dwBytes2>>2;
   while(dw) {*lpSD++=*lpSS++;dw--;}
  }

 IDirectSoundBuffer_Unlock(lpDSB,lpvPtr1,dwBytes1,lpvPtr2,dwBytes2);

 LastWrite+=lBytes;
 if(LastWrite>=SOUNDSIZE) LastWrite-=SOUNDSIZE;
 LastPlay=cplay;
}

#endif



