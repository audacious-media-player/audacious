/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2002 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * a2m.cpp - A2M Loader by Simon Peter <dn.tlp@gmx.net>
 *
 * NOTES:
 * This loader detects and loads version 1, 4, 5 & 8 files.
 *
 * version 1-4 files:
 * Following commands are ignored:
 * FF1 - FF9, FAx - FEx
 *
 * version 5-8 files:
 * Instrument panning is ignored. Flags byte is ignored.
 * Following commands are ignored:
 * Gxy, Hxy, Kxy - &xy
 */

#include "a2m.h"

const unsigned int Ca2mLoader::MAXFREQ = 2000,
Ca2mLoader::MINCOPY = ADPLUG_A2M_MINCOPY,
Ca2mLoader::MAXCOPY = ADPLUG_A2M_MAXCOPY,
Ca2mLoader::COPYRANGES = ADPLUG_A2M_COPYRANGES,
Ca2mLoader::CODESPERRANGE = ADPLUG_A2M_CODESPERRANGE,
Ca2mLoader::TERMINATE = 256,
Ca2mLoader::FIRSTCODE = ADPLUG_A2M_FIRSTCODE,
Ca2mLoader::MAXCHAR = FIRSTCODE + COPYRANGES * CODESPERRANGE - 1,
Ca2mLoader::SUCCMAX = MAXCHAR + 1,
Ca2mLoader::TWICEMAX = ADPLUG_A2M_TWICEMAX,
Ca2mLoader::ROOT = 1, Ca2mLoader::MAXBUF = 42 * 1024,
Ca2mLoader::MAXDISTANCE = 21389, Ca2mLoader::MAXSIZE = 21389 + MAXCOPY;

const unsigned short Ca2mLoader::bitvalue[14] =
  {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};

const signed short Ca2mLoader::copybits[COPYRANGES] =
  {4, 6, 8, 10, 12, 14};

const signed short Ca2mLoader::copymin[COPYRANGES] =
  {0, 16, 80, 336, 1360, 5456};

CPlayer *Ca2mLoader::factory(Copl *newopl)
{
  return new Ca2mLoader(newopl);
}

bool Ca2mLoader::load(const std::string &filename, const CFileProvider &fp)
{
  binistream *f = fp.open(filename); if(!f) return false;
  struct {
    char id[10];
    unsigned long crc;
    unsigned char version,numpats;
  } ch;
  int i,j,k,t;
  unsigned int l;
  unsigned char *org, *orgptr;
  unsigned long alength;
  unsigned short len[9], *secdata, *secptr;
  const unsigned char convfx[16] = {0,1,2,23,24,3,5,4,6,9,17,13,11,19,7,14};
  const unsigned char convinf1[16] = {0,1,2,6,7,8,9,4,5,3,10,11,12,13,14,15};
  const unsigned char newconvfx[] = {0,1,2,3,4,5,6,23,24,21,10,11,17,13,7,19,
				     255,255,22,25,255,15,255,255,255,255,255,
				     255,255,255,255,255,255,255,255,14,255};

  // read header
  f->readString(ch.id, 10); ch.crc = f->readInt(4);
  ch.version = f->readInt(1); ch.numpats = f->readInt(1);

  // file validation section
  if(strncmp(ch.id,"_A2module_",10) || (ch.version != 1 && ch.version != 5 &&
					ch.version != 4 && ch.version != 8)) {
    fp.close(f);
    return false;
  }

  // load, depack & convert section
  nop = ch.numpats; length = 128; restartpos = 0; activechan = 0xffff;
  if(ch.version == 1 || ch.version == 4) {
    for(i=0;i<5;i++) len[i] = f->readInt(2);
    t = 9;
  } else {
    for(i=0;i<9;i++) len[i] = f->readInt(2);
    t = 18;
  }

  // block 0
  secdata = new unsigned short [len[0] / 2];
  if(ch.version == 1 || ch.version == 5) {
    for(i=0;i<len[0]/2;i++) secdata[i] = f->readInt(2);
    org = new unsigned char [MAXBUF]; orgptr = org;
    sixdepak(secdata,org,len[0]);
  } else {
    orgptr = (unsigned char *)secdata;
    for(i=0;i<len[0];i++) orgptr[i] = f->readInt(1);
  }
  memcpy(songname,orgptr,43); orgptr += 43;
  memcpy(author,orgptr,43); orgptr += 43;
  memcpy(instname,orgptr,250*33); orgptr += 250*33;
  for(i=0;i<250;i++) {	// instruments
    inst[i].data[0] = *(orgptr+i*13+10);
    inst[i].data[1] = *(orgptr+i*13);
    inst[i].data[2] = *(orgptr+i*13+1);
    inst[i].data[3] = *(orgptr+i*13+4);
    inst[i].data[4] = *(orgptr+i*13+5);
    inst[i].data[5] = *(orgptr+i*13+6);
    inst[i].data[6] = *(orgptr+i*13+7);
    inst[i].data[7] = *(orgptr+i*13+8);
    inst[i].data[8] = *(orgptr+i*13+9);
    inst[i].data[9] = *(orgptr+i*13+2);
    inst[i].data[10] = *(orgptr+i*13+3);
    if(ch.version == 1 || ch.version == 4)
      inst[i].misc = *(orgptr+i*13+11);
    else
      inst[i].misc = 0;
    inst[i].slide = *(orgptr+i*13+12);
  }

  orgptr += 250*13;
  memcpy(order,orgptr,128); orgptr += 128;
  bpm = *orgptr; orgptr += 1;
  initspeed = *orgptr;
  // v5-8 files have an additional flag byte here
  if(ch.version == 1 || ch.version == 5)
    delete [] org;
  delete [] secdata;

  // blocks 1-4 or 1-8
  alength = len[1];
  for(i=0;i<(ch.version == 1 || ch.version == 4 ? ch.numpats/16 : ch.numpats/8);i++)
    alength += len[i+2];

  secdata = new unsigned short [alength / 2];
  if(ch.version == 1 || ch.version == 5) {
    for(l=0;l<alength/2;l++) secdata[l] = f->readInt(2);
    org = new unsigned char [MAXBUF * (ch.numpats / (ch.version == 1 ? 16 : 8) + 1)];
    orgptr = org; secptr = secdata;
    orgptr += sixdepak(secptr,orgptr,len[1]); secptr += len[1] / 2;
    if(ch.version == 1) {
      if(ch.numpats > 16)
	orgptr += sixdepak(secptr,orgptr,len[2]); secptr += len[2] / 2;
      if(ch.numpats > 32)
	orgptr += sixdepak(secptr,orgptr,len[3]); secptr += len[3] / 2;
      if(ch.numpats > 48)
	sixdepak(secptr,orgptr,len[4]);
    } else {
      if(ch.numpats > 8)
	orgptr += sixdepak(secptr,orgptr,len[2]); secptr += len[2] / 2;
      if(ch.numpats > 16)
	orgptr += sixdepak(secptr,orgptr,len[3]); secptr += len[3] / 2;
      if(ch.numpats > 24)
	orgptr += sixdepak(secptr,orgptr,len[4]); secptr += len[4] / 2;
      if(ch.numpats > 32)
	orgptr += sixdepak(secptr,orgptr,len[5]); secptr += len[5] / 2;
      if(ch.numpats > 40)
	orgptr += sixdepak(secptr,orgptr,len[6]); secptr += len[6] / 2;
      if(ch.numpats > 48)
	orgptr += sixdepak(secptr,orgptr,len[7]); secptr += len[7] / 2;
      if(ch.numpats > 56)
	sixdepak(secptr,orgptr,len[8]);
    }
    delete [] secdata;
  } else {
    org = (unsigned char *)secdata;
    for(l=0;l<alength;l++) org[l] = f->readInt(1);
  }

  for(i=0;i<64*9;i++)		// patterns
    trackord[i/9][i%9] = i+1;

  if(ch.version == 1 || ch.version == 4) {
    for(i=0;i<ch.numpats;i++)
      for(j=0;j<64;j++)
	for(k=0;k<9;k++) {
	  tracks[i*9+k][j].note = org[i*64*t*4+j*t*4+k*4] == 255 ? 127 : org[i*64*t*4+j*t*4+k*4];
	  tracks[i*9+k][j].inst = org[i*64*t*4+j*t*4+k*4+1];
	  tracks[i*9+k][j].command = convfx[org[i*64*t*4+j*t*4+k*4+2]];
	  tracks[i*9+k][j].param2 = org[i*64*t*4+j*t*4+k*4+3] & 0x0f;
	  if(tracks[i*9+k][j].command != 14)
	    tracks[i*9+k][j].param1 = org[i*64*t*4+j*t*4+k*4+3] >> 4;
	  else {
	    tracks[i*9+k][j].param1 = convinf1[org[i*64*t*4+j*t*4+k*4+3] >> 4];
	    if(tracks[i*9+k][j].param1 == 15 && !tracks[i*9+k][j].param2) {	// convert key-off
	      tracks[i*9+k][j].command = 8;
	      tracks[i*9+k][j].param1 = 0;
	      tracks[i*9+k][j].param2 = 0;
	    }
	  }
	  if(tracks[i*9+k][j].command == 14) {
	    switch(tracks[i*9+k][j].param1) {
	    case 2: // convert define waveform
	      tracks[i*9+k][j].command = 25;
	      tracks[i*9+k][j].param1 = tracks[i*9+k][j].param2;
	      tracks[i*9+k][j].param2 = 0xf;
	      break;
	    case 8: // convert volume slide up
	      tracks[i*9+k][j].command = 26;
	      tracks[i*9+k][j].param1 = tracks[i*9+k][j].param2;
	      tracks[i*9+k][j].param2 = 0;
	      break;
	    case 9: // convert volume slide down
	      tracks[i*9+k][j].command = 26;
	      tracks[i*9+k][j].param1 = 0;
	      break;
	    }
	  }
	}
  } else {
    for(i=0;i<ch.numpats;i++)
      for(j=0;j<9;j++)
	for(k=0;k<64;k++) {
	  tracks[i*9+j][k].note = org[i*64*t*4+j*64*4+k*4] == 255 ? 127 : org[i*64*t*4+j*64*4+k*4];
	  tracks[i*9+j][k].inst = org[i*64*t*4+j*64*4+k*4+1];
	  tracks[i*9+j][k].command = newconvfx[org[i*64*t*4+j*64*4+k*4+2]];
	  tracks[i*9+j][k].param1 = org[i*64*t*4+j*64*4+k*4+3] >> 4;
	  tracks[i*9+j][k].param2 = org[i*64*t*4+j*64*4+k*4+3] & 0x0f;
	}
  }

  if(ch.version == 1 || ch.version == 5)
    delete [] org;
  else
    delete [] secdata;
  fp.close(f);
  rewind(0);
  return true;
}

float Ca2mLoader::getrefresh()
{
	if(tempo != 18)
		return (float) (tempo);
	else
		return 18.2f;
}

/*** private methods *************************************/

void Ca2mLoader::inittree()
{
	unsigned short i;

	for(i=2;i<=TWICEMAX;i++) {
		dad[i] = i / 2;
		freq[i] = 1;
	}

	for(i=1;i<=MAXCHAR;i++) {
		leftc[i] = 2 * i;
		rghtc[i] = 2 * i + 1;
	}
}

void Ca2mLoader::updatefreq(unsigned short a,unsigned short b)
{
	do {
		freq[dad[a]] = freq[a] + freq[b];
		a = dad[a];
		if(a != ROOT)
			if(leftc[dad[a]] == a)
				b = rghtc[dad[a]];
			else
				b = leftc[dad[a]];
	} while(a != ROOT);

	if(freq[ROOT] == MAXFREQ)
		for(a=1;a<=TWICEMAX;a++)
			freq[a] >>= 1;
}

void Ca2mLoader::updatemodel(unsigned short code)
{
	unsigned short a=code+SUCCMAX,b,c,code1,code2;

	freq[a]++;
	if(dad[a] != ROOT) {
		code1 = dad[a];
		if(leftc[code1] == a)
			updatefreq(a,rghtc[code1]);
		else
			updatefreq(a,leftc[code1]);

		do {
			code2 = dad[code1];
			if(leftc[code2] == code1)
				b = rghtc[code2];
			else
				b = leftc[code2];

			if(freq[a] > freq[b]) {
				if(leftc[code2] == code1)
					rghtc[code2] = a;
				else
					leftc[code2] = a;

				if(leftc[code1] == a) {
					leftc[code1] = b;
					c = rghtc[code1];
				} else {
					rghtc[code1] = b;
					c = leftc[code1];
				}

				dad[b] = code1;
				dad[a] = code2;
				updatefreq(b,c);
				a = b;
			}

			a = dad[a];
			code1 = dad[a];
		} while(code1 != ROOT);
	}
}

unsigned short Ca2mLoader::inputcode(unsigned short bits)
{
	unsigned short i,code=0;

	for(i=1;i<=bits;i++) {
		if(!ibitcount) {
			if(ibitcount == MAXBUF)
				ibufcount = 0;
			ibitbuffer = wdbuf[ibufcount];
			ibufcount++;
			ibitcount = 15;
		} else
			ibitcount--;

		if(ibitbuffer > 0x7fff)
			code |= bitvalue[i-1];
		ibitbuffer <<= 1;
	}

	return code;
}

unsigned short Ca2mLoader::uncompress()
{
	unsigned short a=1;

	do {
		if(!ibitcount) {
			if(ibufcount == MAXBUF)
				ibufcount = 0;
			ibitbuffer = wdbuf[ibufcount];
			ibufcount++;
			ibitcount = 15;
		} else
			ibitcount--;

		if(ibitbuffer > 0x7fff)
			a = rghtc[a];
		else
			a = leftc[a];
		ibitbuffer <<= 1;
	} while(a <= MAXCHAR);

	a -= SUCCMAX;
	updatemodel(a);
	return a;
}

void Ca2mLoader::decode()
{
	unsigned short i,j,k,t,c,count=0,dist,len,index;

	inittree();
	c = uncompress();

	while(c != TERMINATE) {
		if(c < 256) {
			obuf[obufcount] = (unsigned char)c;
			obufcount++;
			if(obufcount == MAXBUF) {
				output_size = MAXBUF;
				obufcount = 0;
			}

			buf[count] = (unsigned char)c;
			count++;
			if(count == MAXSIZE)
				count = 0;
		} else {
			t = c - FIRSTCODE;
			index = t / CODESPERRANGE;
			len = t + MINCOPY - index * CODESPERRANGE;
			dist = inputcode(copybits[index]) + len + copymin[index];

			j = count;
			k = count - dist;
			if(count < dist)
				k += MAXSIZE;

			for(i=0;i<=len-1;i++) {
				obuf[obufcount] = buf[k];
				obufcount++;
				if(obufcount == MAXBUF) {
					output_size = MAXBUF;
					obufcount = 0;
				}

				buf[j] = buf[k];
				j++; k++;
				if(j == MAXSIZE) j = 0;
				if(k == MAXSIZE) k = 0;
			}

			count += len;
			if(count >= MAXSIZE)
				count -= MAXSIZE;
		}
		c = uncompress();
	}
	output_size = obufcount;
}

unsigned short Ca2mLoader::sixdepak(unsigned short *source, unsigned char *dest,
				    unsigned short size)
{
	if((unsigned int)size + 4096 > MAXBUF)
		return 0;

	buf = new unsigned char [MAXSIZE];
	input_size = size;
	ibitcount = 0; ibitbuffer = 0;
	obufcount = 0; ibufcount = 0;
	wdbuf = source; obuf = dest;

	decode();
	delete [] buf;
	return output_size;
}
