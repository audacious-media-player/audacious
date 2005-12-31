/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2004 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * dro.c - DOSBox Raw OPL Player by Sjoerd van der Berg <harekiet@zophar.net>
 *
 * NOTES:
 * OPL3 and second opl2 writes are ignored
  */

#include "dro.h"

/*** public methods *************************************/

CPlayer *CdroPlayer::factory(Copl *newopl)
{
  return new CdroPlayer(newopl);
}

bool CdroPlayer::load(const std::string &filename, const CFileProvider &fp)
{
	binistream *f = fp.open(filename); if(!f) return false;
	char id[8];unsigned long i;

	// file validation section
	f->readString(id, 8);
	if(strncmp(id,"DBRAWOPL",8)) { fp.close (f); return false; }

	// load section
	mstotal = f->readInt(4);	// Total milliseconds in file
	length = f->readInt(4);		// Total data bytes in file
	mode = (OplMode)f->readInt(1);		// Type of opl data this can contain
	data = new unsigned char [length];
	for (i=0;i<length;i++) 
		data[i]=f->readInt(1);
	fp.close(f);
	rewind(0);
	return true;
}

bool CdroPlayer::update()
{
	if (delay>500) {
		delay-=500;
		return true;
	} else delay=0;
	while (pos < length) 
	{	
		unsigned char cmd = data[pos++];
		switch(cmd) {
		case 0: 
			delay = 1 + data[pos++];
			return true;
		case 1: 
			delay = 1 + data[pos] + (data[pos+1]<<8);
			pos+=2;
			return true;
		case 2:
			index = 0;
			break;
		case 3:
			index = 0;
			break;
		default:
		  if(!index)
		    opl->write(cmd,data[pos++]);
		  break;
		}
	}
	return pos<length;
}

void CdroPlayer::rewind(int subsong)
{
	delay=1;
	pos = index = 0; 
	opl->init(); 
	opl->write(1,32);	// go to OPL2 mode
}

float CdroPlayer::getrefresh()
{
	if (delay > 500) return 1000 / 500;
	else return 1000 / (double)delay;
}
