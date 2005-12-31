/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999, 2000, 2001 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 *
 * analopl.h - Spectrum analyzing hardware OPL, by Simon Peter (dn.tlp@gmx.net)
 */

#ifndef H_ANALOPL_DEFINED
#define H_ANALOPL_DEFINED

#include "opl.h"

#define DFL_ADLIBPORT	0x388		// default adlib baseport

class CAnalopl: public Copl
{
public:
	CAnalopl(unsigned short initport = DFL_ADLIBPORT);	// initport = OPL2 hardware baseport

	bool detect();						// returns true if adlib compatible board is found, else false
	void setvolume(int volume);			// set adlib master volume (0 - 63) 0 = loudest, 63 = softest
	void setquiet(bool quiet = true);	// sets the OPL2 quiet, while still writing to the registers
	void setport(unsigned short port)	// set new OPL2 hardware baseport
	{ adlport = port; };
	void setnowrite(bool nw = true)		// set hardware write status
	{ nowrite = nw; };

	int getvolume()						// get adlib master volume
	{ return hardvol; };
	int getcarriervol(unsigned int v);			// get carrier volume of adlib voice v
	int getmodulatorvol(unsigned int v);		// get modulator volume of adlib voice v
	bool getkeyon(unsigned int v);

	// template methods
	void write(int reg, int val);
	void init();

private:
	void hardwrite(int reg, int val);	// write to OPL2 hardware registers

	unsigned short	adlport;			// adlib hardware baseport
	int				hardvol,oldvol;		// hardware master volume
	bool			bequiet;			// quiet status cache
	char			hardvols[22][2];	// volume cache
	unsigned char	keyregs[9][2];			// shadow key register
	bool			nowrite;			// don't write to hardware, if true
};

#endif
