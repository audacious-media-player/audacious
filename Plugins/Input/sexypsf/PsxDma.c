/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2004  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  
USA
 */

#include <string.h>

#include "PsxCommon.h"

// Dma0/1   in Mdec.c
// Dma3     in CdRom.c
// Dma8     in PsxSpd.c
// Dma11/12 in PsxSio2.c

void psxDma4(u32 madr, u32 bcr, u32 chcr) { // SPU
	int size;

	switch (chcr) {
		case 0x01000201: //cpu to spu transfer
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 4 - SPU mem2spu *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			size = (bcr >> 16) * (bcr & 0xffff) * 2;
			SPU2writeDMA4Mem((u16 *)PSXM(madr), size);
#if 0
			PSX_INT(4, (size * 80) / BIAS);
#endif
			break;
		case 0x01000200: //spu to cpu transfer
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 4 - SPU spu2mem *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			size = (bcr >> 16) * (bcr & 0xffff) * 2;
    		SPU2readDMA4Mem((u16 *)PSXM(madr), size);
#if 0
			PSX_INT(4, (size * 80) / BIAS);
#endif
			break;
#ifdef PSXDMA_LOG
		default:
			PSXDMA_LOG("*** DMA 4 - SPU unknown *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
			break;
#endif
	}
}

void psxDma4Interrupt() {
#if 0
	HW_DMA4_CHCR &= ~0x01000000;
	DMA_INTERRUPT(4);
#endif
	SPU2interruptDMA4();
}

void psxDma6(u32 madr, u32 bcr, u32 chcr) {
	u32 *mem = (u32 *)PSXM(madr);

#ifdef PSXDMA_LOG
	PSXDMA_LOG("*** DMA 6 - OT *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif

	if (chcr == 0x11000002) {
		while (bcr--) {
			*mem-- = (madr - 4) & 0xffffff;
			madr -= 4;
		}
		mem++; *mem = 0xffffff;
	} else {
		// Unknown option
#ifdef PSXDMA_LOG
		PSXDMA_LOG("*** DMA 6 - OT unknown *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
	}
#if 0
	HW_DMA6_CHCR &= ~0x01000000;
	DMA_INTERRUPT(6);
#endif
}

void psxDma7(u32 madr, u32 bcr, u32 chcr) {
	int size;

	switch (chcr) {
		case 0x01000201: //cpu to spu2 transfer
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 7 - SPU2 mem2spu *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			size = (bcr >> 16) * (bcr & 0xffff) * 2;
			SPU2writeDMA7Mem((u16 *)PSXM(madr), size);
#if 0
			PSX_INT(7, (size * 80) / BIAS);
#endif
			break;
		case 0x01000200: //spu2 to cpu transfer
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 7 - SPU2 spu2mem *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			size = (bcr >> 16) * (bcr & 0xffff) * 2;
    		SPU2readDMA7Mem((u16 *)PSXM(madr), size);
#if 0
			PSX_INT(7, (size * 80) / BIAS);
#endif
			break;
#ifdef PSXDMA_LOG
		default:
			PSXDMA_LOG("*** DMA 7 - SPU2 unknown *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
			break;
#endif
	}
}

void psxDma7Interrupt() {
#if 0
	HW_DMA7_CHCR &= ~0x01000000;
	DMA_INTERRUPT2(0);
	SPU2interruptDMA7();
#endif
}

