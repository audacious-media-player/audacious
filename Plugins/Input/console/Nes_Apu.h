
// NES 2A03 APU sound chip emulator

// Nes_Snd_Emu 0.1.6. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef NES_APU_H
#define NES_APU_H

typedef long nes_time_t;     // CPU clock cycle count
typedef unsigned nes_addr_t; // 16-bit memory address

#include "Nes_Oscs.h"

class Tagged_Data;

class Nes_Apu {
public:
	Nes_Apu();
	~Nes_Apu();
	
// Initialization

	// Reset internal frame counter, registers, and all oscillators.
	// Use PAL timing if pal_timing is true, otherwise use NTSC timing.
	// Set the DMC oscillator's initial DAC value to initial_dmc_dac without
	// any audible click.
	void reset( bool pal_timing = false, int initial_dmc_dac = 0 );
	
	// Set memory reader callback used by DMC oscillator to fetch samples.
	//  When callback is invoked,  'user_data' is passed unchanged as the
	// first parameter.
	void dmc_reader( int (*callback)( void* user_data, nes_addr_t ), void* user_data = NULL );
	
	// Set IRQ time callback that is invoked when the time of earliest IRQ
	// may have changed, or NULL to disable. When callback is invoked,
	// 'user_data' is passed unchanged as the first parameter.
	void irq_notifier( void (*callback)( void* user_data ), void* user_data = NULL );
	
	// Reflect complete state between tagged data container.
	void reflect_state( Tagged_Data& );
	
// Sound output
	
	// Set overall volume (default is 1.0). Set nonlinear to true only when
	// using special Nes_Nonlinearizer on output.
	void volume( double, bool nonlinear = false );
	
	// Set treble equalization (see notes.txt).
	void treble_eq( const blip_eq_t& );
	
	// Set sound output of all oscillators to buffer. If buffer is NULL, no
	// sound is generated and emulation accuracy is reduced.
	void output( Blip_Buffer* );
	
	// Set sound output of specific oscillator to buffer. If buffer is NULL,
	// the specified oscillator is muted and emulation accuracy is reduced.
	// The oscillators are indexed as follows: 0) Square 1, 1) Square 2,
	// 2) Triangle, 3) Noise, 4) DMC.
	enum { osc_count = 5 };
	void osc_output( int index, Blip_Buffer* buffer );
	
// Emulation

	// All time values are the number of CPU clock cycles relative to the
	// beginning of the current time frame.

	// Emulate memory write at specified time. Address must be in the range
	// start_addr to end_addr.
	enum { start_addr = 0x4000 };
	enum { end_addr   = 0x4017 };
	void write_register( nes_time_t, nes_addr_t, int data );
	
	// Emulate memory read of the status register at specified time.
	enum { status_addr = 0x4015 };
	int read_status( nes_time_t );
	
	// Get time that APU-generated IRQ will occur if no further register reads
	// or writes occur. If IRQ is already pending, returns irq_waiting. If no
	// IRQ will occur, returns no_irq.
	enum { no_irq = LONG_MAX / 2 + 1 };
	enum { irq_waiting = 0 };
	nes_time_t earliest_irq() const;
	
	// Run APU until specified time, so that any DMC memory reads can be
	// accounted for (i.e. inserting CPU wait states).
	void run_until( nes_time_t );
	
	// Count number of DMC reads that would occur if 'run_until( t )' were executed.
	// If last_read is not NULL, set *last_read to the earliest time that
	// 'count_dmc_reads( time )' would result in the same result.
	int count_dmc_reads( nes_time_t t, nes_time_t* last_read = NULL ) const;
	
	// Run all oscillators up to specified time, end current time frame, then
	// start a new time frame at time 0. Time frames have no effect on emulation
	// and each can be whatever length is convenient.
	void end_frame( nes_time_t );
	
// End of public interface.

private:
	// noncopyable
	Nes_Apu( const Nes_Apu& );
	Nes_Apu& operator = ( const Nes_Apu& );
	
	Nes_Osc*            oscs [osc_count];
	Nes_Square          square1;
	Nes_Square          square2;
	Nes_Noise           noise;
	Nes_Triangle        triangle;
	Nes_Dmc             dmc;
	
	nes_time_t last_time; // has been run until this time in current frame
	nes_time_t earliest_irq_;
	nes_time_t next_irq;
	int frame_period;
	int frame_delay; // cycles until frame counter runs next
	int frame; // current frame (0-3)
	int osc_enables;
	int frame_mode;
	bool irq_flag;
	void (*irq_notifier_)( void* user_data );
	void* irq_data;
	Nes_Square::Synth square_synth; // shared by squares
	
	void irq_changed();
	void state_restored();
	
	friend struct Nes_Dmc;
};

	inline void Nes_Apu::osc_output( int osc, Blip_Buffer* buf ) {
		assert(( "Nes_Apu::osc_output(): Index out of range", 0 <= osc && osc < osc_count ));
		oscs [osc]->output = buf;
	}

	inline nes_time_t Nes_Apu::earliest_irq() const {
		return earliest_irq_;
	}

	inline void Nes_Apu::dmc_reader( int (*func)( void*, nes_addr_t ), void* user_data ) {
		dmc.rom_reader_data = user_data;
		dmc.rom_reader = func;
	}

	inline void Nes_Apu::irq_notifier( void (*func)( void* user_data ), void* user_data ) {
		irq_notifier_ = func;
		irq_data = user_data;
	}
	
	inline int Nes_Apu::count_dmc_reads( nes_time_t time, nes_time_t* last_read ) const {
		return dmc.count_reads( time, last_read );
	}
	
#endif

