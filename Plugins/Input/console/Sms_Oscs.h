
// Private oscillators used by Sms_Apu

// Sms_Snd_Emu 0.1.3. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef SMS_OSCS_H
#define SMS_OSCS_H

#include "Blip_Buffer.h"

struct Sms_Osc
{
	Blip_Buffer* outputs [4]; // NULL, right, left, center
	Blip_Buffer* output;
	int output_select;
	
	int delay;
	int last_amp;
	int volume;
	
	Sms_Osc();
	void reset();
	virtual void run( sms_time_t start, sms_time_t end ) = 0;
};

struct Sms_Square : Sms_Osc
{
	int period;
	int phase;
	
	typedef Blip_Synth<blip_good_quality,64 * 2> Synth;
	const Synth* synth;
	
	Sms_Square();
	void reset();
	void run( sms_time_t, sms_time_t );
};

struct Sms_Noise : Sms_Osc
{
	const int* period;
	unsigned shifter;
	unsigned tap;
	
	typedef Blip_Synth<blip_med_quality,64 * 2> Synth;
	Synth synth;
	
	Sms_Noise();
	void reset();
	void run( sms_time_t, sms_time_t );
};

#endif

