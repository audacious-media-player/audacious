
// Nintendo Game Boy PAPU sound chip emulator

// Gb_Snd_Emu 0.1.3. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef GB_APU_H
#define GB_APU_H

typedef long gb_time_t;     // clock cycle count
typedef unsigned gb_addr_t; // 16-bit address

#include "Gb_Oscs.h"

class Gb_Apu {
public:
	Gb_Apu();
	~Gb_Apu();
	
	// Overall volume of all oscillators, where 1.0 is full volume.
	void volume( double );
	
	// Treble equalization (see notes.txt).
	void treble_eq( const blip_eq_t& );
	
	// Reset oscillators and internal state.
	void reset();
	
	// Assign all oscillator outputs to specified buffer(s). If buffer
	// is NULL, silence all oscillators.
	void output( Blip_Buffer* mono );
	void output( Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right );
	
	// Assign oscillator output to buffer(s). Valid indicies are 0 to
	// osc_count - 1, which refer to Square 1, Square 2, Wave, and
	// Noise, respectively. If buffer is NULL, silence oscillator.
	enum { osc_count = 4 };
	void osc_output( int index, Blip_Buffer* mono );
	void osc_output( int index, Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right );
	
	// Reads and writes at addr must satisfy start_addr <= addr <= end_addr
	enum { start_addr = 0xff10 };
	enum { end_addr   = 0xff3f };
	enum { register_count = end_addr - start_addr + 1 };
	
	// Write 'data' to address at specified time. Previous writes and reads
	// within the current frame must not have specified a later time.
	void write_register( gb_time_t, gb_addr_t, int data );
	
	// Read from address at specified time. Previous writes and reads within
	// the current frame must not have specified a later time.
	int read_register( gb_time_t, gb_addr_t );
	
	// Run all oscillators up to specified time, end current time frame, then
	// start a new frame at time 0. Return true if any oscillators added
	// sound to one of the left/right buffers, false if they only added
	// to the center buffer.
	bool end_frame( gb_time_t );
	
	static void begin_debug_log();
private:
	// noncopyable
	Gb_Apu( const Gb_Apu& );
	Gb_Apu& operator = ( const Gb_Apu& );
	
	Gb_Osc*     oscs [osc_count];
	gb_time_t   next_frame_time;
	gb_time_t   last_time;
	int         frame_count;
	bool        stereo_found;
	
	Gb_Square   square1;
	Gb_Square   square2;
	Gb_Wave     wave;
	Gb_Noise    noise;
	Gb_Square::Synth square_synth; // shared between squares
	BOOST::uint8_t regs [register_count];
	
	void run_until( gb_time_t );
	friend class Gb_Apu_Reflector;
};

	inline void Gb_Apu::output( Blip_Buffer* mono ) {
		output( mono, NULL, NULL );
	}
	
	inline void Gb_Apu::osc_output( int index, Blip_Buffer* mono ) {
		osc_output( index, mono, NULL, NULL );
	}

#endif

