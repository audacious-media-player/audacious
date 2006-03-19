/***************************************************************************
                            dma.c  -  description
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
// 2004/04/04 - Pete
// - changed plugin to emulate PS2 spu
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_DMA

#include "externals.h"
#include "registers.h"
#include "debug.h"

////////////////////////////////////////////////////////////////////////
// READ DMA (many values)
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2readDMA4Mem(unsigned short * pusPSXMem,int iSize)
{
 int i;

#ifdef _WINDOWS
 if(iDebugMode==1) 
  {
   logprintf("READDMA4 %X - %X\r\n",spuAddr2[0],iSize);
   
   if(spuAddr2[0]<=0x1fff)
    logprintf("# OUTPUT AREA ACCESS #############\r\n");
  }

#endif

 for(i=0;i<iSize;i++)
  {
   *pusPSXMem++=spuMem[spuAddr2[0]];                  // spu addr 0 got by writeregister
   spuAddr2[0]++;                                     // inc spu addr
   if(spuAddr2[0]>0xfffff) spuAddr2[0]=0;             // wrap
  }

 spuAddr2[0]+=0x20; //?????
 

 iSpuAsyncWait=0;

 // got from J.F. and Kanodin... is it needed?
 regArea[(PS2_C0_ADMAS)>>1]=0;                         // Auto DMA complete
 spuStat2[0]=0x80;                                     // DMA complete
}

EXPORT_GCC void CALLBACK SPU2readDMA7Mem(unsigned short * pusPSXMem,int iSize)
{
 int i;

#ifdef _WINDOWS
 if(iDebugMode==1) 
  { 
   logprintf("READDMA7 %X - %X\r\n",spuAddr2[1],iSize);
   
   if(spuAddr2[1]<=0x1fff)
    logprintf("# OUTPUT AREA ACCESS #############\r\n");
  }
#endif

 for(i=0;i<iSize;i++)
  {
   *pusPSXMem++=spuMem[spuAddr2[1]];                   // spu addr 1 got by writeregister
   spuAddr2[1]++;                                      // inc spu addr
   if(spuAddr2[1]>0xfffff) spuAddr2[1]=0;              // wrap
  }

 spuAddr2[1]+=0x20; //?????

 iSpuAsyncWait=0;

 // got from J.F. and Kanodin... is it needed?
 regArea[(PS2_C1_ADMAS)>>1]=0;                         // Auto DMA complete
 spuStat2[1]=0x80;                                     // DMA complete
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// to investigate: do sound data updates by writedma affect spu
// irqs? Will an irq be triggered, if new data is written to
// the memory irq address?

////////////////////////////////////////////////////////////////////////
// WRITE DMA (many values)
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2writeDMA4Mem(unsigned short * pusPSXMem,int iSize)
{
 int i;

#ifdef _WINDOWS
 if(iDebugMode==1) 
  {
   logprintf("WRITEDMA4 %X - %X\r\n",spuAddr2[0],iSize);
   
   if(spuAddr2[0]>=0x2000 && spuAddr2[0]<=0x27ff)
    logprintf("# RAW INPUT ###############\r\n");
  } 
#endif

 for(i=0;i<iSize;i++)
  {
   spuMem[spuAddr2[0]] = *pusPSXMem++;                 // spu addr 0 got by writeregister
   spuAddr2[0]++;                                      // inc spu addr
   if(spuAddr2[0]>0xfffff) spuAddr2[0]=0;              // wrap
  }
 
 iSpuAsyncWait=0;

 // got from J.F. and Kanodin... is it needed?
 spuStat2[0]=0x80;                                     // DMA complete
}

EXPORT_GCC void CALLBACK SPU2writeDMA7Mem(unsigned short * pusPSXMem,int iSize)
{
 int i;

#ifdef _WINDOWS
 if(iDebugMode==1) 
  {
   logprintf("WRITEDMA7 %X - %X\r\n",spuAddr2[1],iSize);
   if(spuAddr2[1]>=0x2000 && spuAddr2[1]<=0x27ff)
    logprintf("# RAW INPUT ###############\r\n");
  } 
#endif

 for(i=0;i<iSize;i++)
  {
   spuMem[spuAddr2[1]] = *pusPSXMem++;                 // spu addr 1 got by writeregister
   spuAddr2[1]++;                                      // inc spu addr
   if(spuAddr2[1]>0xfffff) spuAddr2[1]=0;              // wrap
  }
 
 iSpuAsyncWait=0;

 // got from J.F. and Kanodin... is it needed?
 spuStat2[1]=0x80;                                     // DMA complete
}

////////////////////////////////////////////////////////////////////////
// INTERRUPTS
////////////////////////////////////////////////////////////////////////

void InterruptDMA4(void) 
{
// taken from linuzappz NULL spu2
//	spu2Rs16(CORE0_ATTR)&= ~0x30;
//	spu2Rs16(REG__1B0) = 0;
//	spu2Rs16(SPU2_STATX_WRDY_M)|= 0x80;

#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("IRQDMA4\r\n");
#endif

 spuCtrl2[0]&=~0x30;
 regArea[(PS2_C0_ADMAS)>>1]=0;
 spuStat2[0]|=0x80;
}
                       
EXPORT_GCC void CALLBACK SPU2interruptDMA4(void) 
{
 InterruptDMA4();
}

void InterruptDMA7(void) 
{
// taken from linuzappz NULL spu2
//	spu2Rs16(CORE1_ATTR)&= ~0x30;
//	spu2Rs16(REG__5B0) = 0;
//	spu2Rs16(SPU2_STATX_DREQ)|= 0x80;

#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("IRQDMA7\r\n");
#endif

 spuCtrl2[1]&=~0x30;
 regArea[(PS2_C1_ADMAS)>>1]=0;
 spuStat2[1]|=0x80;
}

EXPORT_GCC void CALLBACK SPU2interruptDMA7(void) 
{
 InterruptDMA7();
}

