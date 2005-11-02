
// Sega Master System SN76489 PSG sound chip emulator

// Sms_Snd_Emu 0.1.3. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef SMS_APU_H
#define SMS_APU_H

typedef long sms_time_t; // clock cycle count

#include "Sms_Oscs.h"

class Sms_Apu {
public:
	Sms_Apu();
	~Sms_Apu();
	
	// Overall volume of all oscillators, where 1.0 is full volume.
	void volume( double );
	
	// Treble equalization (see notes.txt).
	void treble_eq( const blip_eq_t& );
	
	// Assign all oscillator outputs to specified buffer(s). If buffer
	// is NULL, silence all oscillators.
	void output( Blip_Buffer* mono );
	void output( Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right );
	
	// Assign oscillator output to buffer(s). Valid indicies are 0 to
	// osc_count - 1, which refer to Square 1, Square 2, Square 3, and
	// Noise, respectively. If buffer is NULL, silence oscillator.
	enum { osc_count = 4 };
	void osc_output( int index, Blip_Buffer* mono );
	void osc_output( int index, Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right );
	
	// Reset oscillators
	void reset();
	
	// Write GameGear left/right assignment byte
	void write_ggstereo( sms_time_t, int );
	
	// Write to data port
	void write_data( sms_time_t, int );
	
	// Run all oscillators up to specified time, end current frame, then
	// start a new frame at time 0. Return true if any oscillators added
	// sound to one of the left/right buffers, false if they only added
	// to the center buffer.
	bool end_frame( sms_time_t );
	
	
	// End of public interface
private:
	// noncopyable
	Sms_Apu( const Sms_Apu& );
	Sms_Apu& operator = ( const Sms_Apu& );
	
	Sms_Osc*    oscs [osc_count];
	Sms_Square  squares [3];
	Sms_Noise   noise;
	Sms_Square::Synth square_synth; // shared between squares
	sms_time_t  last_time;
	int         latch;
	bool        stereo_found;
	
	void run_until( sms_time_t );
};

inline void Sms_Apu::output( Blip_Buffer* mono ) {
	output( mono, NULL, NULL );
}

inline void Sms_Apu::osc_output( int index, Blip_Buffer* mono ) {
	osc_output( index, mono, NULL, NULL );
}

#endif

