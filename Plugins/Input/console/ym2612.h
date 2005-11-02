
// Sega Genesis YM2612 FM Sound Chip Emulator

// Game_Music_Emu 0.2.4. Copyright (C) 2004-2005 Shay Green. GNU LGPL license.
// Copyright (C) 2002 Stéphane Dallongeville

#ifndef YM2612_H
#define YM2612_H

#include "blargg_common.h"

struct YM2612_Impl;

class YM2612_Emu {
public:
	YM2612_Emu();
	~YM2612_Emu();
	
	blargg_err_t set_rate( long sample_rate, long clock_rate );
	
	void reset();
	
	enum { channel_count = 6 };
	void mute_voices( int mask );
	
	void write( int addr, int data );
	
	void run_timer( int );
	
	typedef BOOST::int16_t sample_t;
	void run( sample_t*, int count );
	
private:
	YM2612_Impl* impl;
};

#endif

