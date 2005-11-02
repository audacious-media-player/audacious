
// Namco 106 sound chip emulator

// Nes_Snd_Emu 0.1.6. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef NES_NAMCO_H
#define NES_NAMCO_H

#include "Nes_Apu.h"

class Tagged_Data;

class Nes_Namco {
public:
	Nes_Namco();
	~Nes_Namco();
	
	// See Nes_Apu.h for reference.
	
	void volume( double );
	void treble_eq( const blip_eq_t& );
	void output( Blip_Buffer* );
	enum { osc_count = 8 };
	void osc_output( int index, Blip_Buffer* );
	void reset();
	
	// Read/write data register is at $4800
	enum { data_reg_addr = 0x4800 };
	void write_data( nes_time_t, int );
	int read_data();
	
	// Write-only address register is at $F800
	enum { addr_reg_addr = 0xF800 };
	void write_addr( int );
	
	void end_frame( nes_time_t );
	void reflect_state( Tagged_Data& );
	
// End of public interface

private:
	// noncopyable
	Nes_Namco( const Nes_Namco& );
	Nes_Namco& operator = ( const Nes_Namco& );
	
	struct Namco_Osc {
		long delay;
		Blip_Buffer* output;
		short last_amp;
		short wave_pos;
	};
	
	Namco_Osc oscs [osc_count];
	
	nes_time_t last_time;
	int addr_reg;
	
	enum { reg_count = 0x80 };
	BOOST::uint8_t reg [reg_count];
	Blip_Synth<blip_good_quality,15> synth;
	
	BOOST::uint8_t& access();
	void run_until( nes_time_t );
};

	inline void Nes_Namco::volume( double v ) {
		synth.volume( 0.10 / osc_count * v );
	}

	inline void Nes_Namco::treble_eq( const blip_eq_t& eq ) {
		synth.treble_eq( eq );
	}

	inline void Nes_Namco::osc_output( int i, Blip_Buffer* buf ) {
		assert( (unsigned) i < osc_count );
		oscs [i].output = buf;
	}

	inline void Nes_Namco::write_addr( int v ) {
		addr_reg = v;
	}

	inline int Nes_Namco::read_data() {
		return access();
	}

	inline void Nes_Namco::write_data( nes_time_t time, int data ) {
		run_until( time );
		access() = data;
	}

	inline void Nes_Namco::end_frame( nes_time_t time ) {
		run_until( time );
		last_time -= time;
		assert( last_time >= 0 );
	}

#endif

