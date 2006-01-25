
// Sms_Snd_Emu 0.1.3. http://www.slack.net/~ant/

#include "Sms_Apu.h"

/* Copyright (C) 2003-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include BLARGG_SOURCE_BEGIN

// Sms_Osc

Sms_Osc::Sms_Osc()
{
	output = NULL;
	outputs [0] = NULL; // always stays NULL
	outputs [1] = NULL;
	outputs [2] = NULL;
	outputs [3] = NULL;
}

void Sms_Osc::reset()
{
	delay = 0;
	last_amp = 0;
	volume = 0;
	output_select = 3;
	output = outputs [3];
}

// Sms_Square

inline void Sms_Square::reset()
{
	period = 0;
	phase = 0;
	Sms_Osc::reset();
}

void Sms_Square::run( sms_time_t time, sms_time_t end_time )
{
	if ( !volume || period <= 128 )
	{
		// ignore 16kHz and higher
		if ( last_amp )
		{
			synth->offset( time, -last_amp, output );
			last_amp = 0;
		}
		time += delay;
		if ( !period )
		{
			time = end_time;
		}
		else if ( time < end_time )
		{
			// keep calculating phase
			int count = (end_time - time + period - 1) / period;
			phase = (phase + count) & 1;
			time += count * period;
		}
	}
	else
	{
		int amp = phase ? volume : -volume;
		int delta = amp - last_amp;
		if ( delta )
		{
			last_amp = amp;
			synth->offset( time, delta, output );
		}
		
		time += delay;
		if ( time < end_time )
		{
			Blip_Buffer* const output = this->output;
			int delta = amp * 2;
			do
			{
				delta = -delta;
				synth->offset_inline( time, delta, output );
				time += period;
				phase ^= 1;
			}
			while ( time < end_time );
			this->last_amp = phase ? volume : -volume;
		}
	}
	delay = time - end_time;
}

// Sms_Noise

static const int noise_periods [3] = { 0x100, 0x200, 0x400 };

inline void Sms_Noise::reset()
{
	period = &noise_periods [0];
	shifter = 0x8000;
	tap = 12;
	Sms_Osc::reset();
}

void Sms_Noise::run( sms_time_t time, sms_time_t end_time )
{
	int amp = volume;
	if ( shifter & 1 )
		amp = -amp;
	
	int delta = amp - last_amp;
	if ( delta )
	{
		last_amp = amp;
		synth.offset( time, delta, output );
	}
	
	time += delay;
	if ( !volume )
		time = end_time;
	
	if ( time < end_time )
	{
		Blip_Buffer* const output = this->output;
		unsigned shifter = this->shifter;
		int delta = amp * 2;
		int period = *this->period * 2;
		if ( !period )
			period = 16;
		
		do
		{
			int changed = (shifter + 1) & 2; // set if prev and next bits differ
			shifter = (((shifter << 15) ^ (shifter << tap)) & 0x8000) | (shifter >> 1);
			if ( changed )
			{
				delta = -delta;
				synth.offset_inline( time, delta, output );
			}
			time += period;
		}
		while ( time < end_time );
		
		this->shifter = shifter;
		this->last_amp = delta >> 1;
	}
	delay = time - end_time;
}

// Sms_Apu

Sms_Apu::Sms_Apu()
{
	for ( int i = 0; i < 3; i++ )
	{
		squares [i].synth = &square_synth;
		oscs [i] = &squares [i];
	}
	oscs [3] = &noise;
	
	volume( 1.0 );
	reset();
}

Sms_Apu::~Sms_Apu()
{
}

void Sms_Apu::volume( double vol )
{
	vol *= 0.85 / (osc_count * 64 * 2);
	square_synth.volume( vol );
	noise.synth.volume( vol );
}

void Sms_Apu::treble_eq( const blip_eq_t& eq )
{
	square_synth.treble_eq( eq );
	noise.synth.treble_eq( eq );
}

void Sms_Apu::osc_output( int index, Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right )
{
	require( (unsigned) index < osc_count );
	require( (center && left && right) || (!center && !left && !right) );
	Sms_Osc& osc = *oscs [index];
	osc.outputs [1] = right;
	osc.outputs [2] = left;
	osc.outputs [3] = center;
	osc.output = osc.outputs [osc.output_select];
}

void Sms_Apu::output( Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right )
{
	for ( int i = 0; i < osc_count; i++ )
		osc_output( i, center, left, right );
}

void Sms_Apu::reset()
{
	stereo_found = false;
	last_time = 0;
	latch = 0;
	
	squares [0].reset();
	squares [1].reset();
	squares [2].reset();
	noise.reset();
}

void Sms_Apu::run_until( sms_time_t end_time )
{
	require( end_time >= last_time ); // end_time must not be before previous time
	
	if ( end_time > last_time )
	{
		// run oscillators
		for ( int i = 0; i < osc_count; ++i )
		{
			Sms_Osc& osc = *oscs [i];
			if ( osc.output )
			{
				if ( osc.output != osc.outputs [3] )
					stereo_found = true; // playing on side output
				
				if ( i < 3 )
					squares [i].run( last_time, end_time );
				else
					noise.run( last_time, end_time );
			}
		}
		
		last_time = end_time;
	}
}

bool Sms_Apu::end_frame( sms_time_t end_time )
{
	if ( end_time > last_time )
		run_until( end_time );
	
	assert( last_time >= end_time );
	last_time -= end_time;
	
	bool result = stereo_found;
	stereo_found = false;
	return result;
}

void Sms_Apu::write_ggstereo( sms_time_t time, int data )
{
	require( (unsigned) data <= 0xFF );
	
	run_until( time );
	
	for ( int i = 0; i < osc_count; i++ )
	{
		Sms_Osc& osc = *oscs [i];
		int flags = data >> i;
		Blip_Buffer* old_output = osc.output;
		osc.output_select = (flags >> 3 & 2) | (flags & 1);
		osc.output = osc.outputs [osc.output_select];
		if ( osc.output != old_output && osc.last_amp )
		{
			if ( old_output )
				square_synth.offset( time, -osc.last_amp, old_output );
			osc.last_amp = 0;
		}
	}
}

static const unsigned char volumes [16] = {
	// volumes [i] = 64 * pow( 1.26, 15 - i ) / pow( 1.26, 15 )
	64, 50, 39, 31, 24, 19, 15, 12, 9, 7, 5, 4, 3, 2, 1, 0
};

void Sms_Apu::write_data( sms_time_t time, int data )
{
	require( (unsigned) data <= 0xFF );
	
	run_until( time );
	
	if ( data & 0x80 )
		latch = data;
	
	int index = (latch >> 5) & 3;
	if ( latch & 0x10 )
	{
		oscs [index]->volume = volumes [data & 15];
	}
	else if ( index < 3 )
	{
		Sms_Square& sq = squares [index];
		if ( data & 0x80 )
			sq.period = (sq.period & 0xFF00) | (data << 4 & 0x00FF);
		else
			sq.period = (sq.period & 0x00FF) | (data << 8 & 0x3F00);
	}
	else
	{
		int select = data & 3;
		if ( select < 3 )
			noise.period = &noise_periods [select];
		else
			noise.period = &squares [2].period;
		
		int const tap_disabled = 16;
		noise.tap = (data & 0x04) ? 12 : tap_disabled;
		noise.shifter = 0x8000;
	}
}

