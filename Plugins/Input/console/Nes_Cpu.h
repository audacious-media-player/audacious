
// Nintendo Entertainment System (NES) 6502 CPU emulator

// Game_Music_Emu 0.2.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef NES_CPU_H
#define NES_CPU_H

#include "blargg_common.h"

typedef long nes_time_t;     // clock cycle count
typedef unsigned nes_addr_t; // 16-bit address

class Nsf_Emu;

#ifndef NES_CPU_IRQ_SUPPORT
	#define NES_CPU_IRQ_SUPPORT 0
#endif

class Nes_Cpu {
	typedef BOOST::uint8_t uint8_t;
	enum { page_bits = 11 };
	enum { page_count = 0x10000 >> page_bits };
	const uint8_t* code_map [page_count + 1];
public:
	Nes_Cpu();
	
	// Clear registers, unmap memory, and map code pages to unmapped_page.
	void reset( const void* unmapped_page = NULL );
	
	// Memory read/write function types. Reader must return value from 0 to 255.
	Nsf_Emu* callback_data;
	typedef int  (*reader_t)( Nsf_Emu* callback_data, nes_addr_t );
	typedef void (*writer_t)( Nsf_Emu* callback_data, nes_addr_t, int value );
	
	// Memory mapping functions take a block of memory of specified 'start' address
	// and 'size' in bytes. Both start address and size must be a multiple of page_size.
	enum { page_size = 1L << page_bits };
	
	// Map code memory to 'code' (memory accessed via the program counter)
	void map_code( nes_addr_t start, unsigned long size, const void* code );
	
	// Map data memory to read and write functions
	void map_memory( nes_addr_t start, unsigned long size, reader_t, writer_t );
	
	// Access memory as the emulated CPU does.
	int  read( nes_addr_t );
	void write( nes_addr_t, int value );
	uint8_t* get_code( nes_addr_t );
	
	// NES 6502 registers. *Not* kept updated during a call to run().
	struct registers_t {
		BOOST::uint16_t pc;
		uint8_t a;
		uint8_t x;
		uint8_t y;
		uint8_t status;
		uint8_t sp;
	} r;
	
	// Reasons that run() returns
	enum result_t {
		result_cycles,  // Requested number of cycles (or more) were executed
		result_cli,     // I flag cleared
		result_badop    // unimplemented/illegal instruction
	};
	
	// Run CPU to or after end_time, or until a stop reason from above
	// is encountered. Return the reason for stopping.
	result_t run( nes_time_t end_time );
	
	nes_time_t time() const             { return base_time + cycle_count; }
	void time( nes_time_t t );
	nes_time_t end_time() const         { return base_time + cycle_limit; }
	void end_frame( nes_time_t );
#if NES_CPU_IRQ_SUPPORT
	void end_time( nes_time_t t )       { cycle_limit = t - base_time; }
#endif
	
private:
	// noncopyable
	Nes_Cpu( const Nes_Cpu& );
	Nes_Cpu& operator = ( const Nes_Cpu& );
	
	nes_time_t cycle_count;
	nes_time_t base_time;
#if NES_CPU_IRQ_SUPPORT
	nes_time_t cycle_limit;
#else
	enum { cycle_limit = 0 };
#endif
	
	reader_t data_reader [page_count + 1]; // extra entry to catch address overflow
	writer_t data_writer [page_count + 1];

public:
	// low_mem is a full page size so it can be mapped with code_map
	uint8_t low_mem [page_size > 0x800 ? page_size : 0x800];
};

	inline BOOST::uint8_t* Nes_Cpu::get_code( nes_addr_t addr ) {
		return (uint8_t*) &code_map [(addr) >> page_bits] [(addr) & (page_size - 1)];
	}
	
#if NES_CPU_IRQ_SUPPORT
	inline void Nes_Cpu::time( nes_time_t t ) {
		t -= time();
		cycle_limit -= t;
		base_time += t;
	}
#else
	inline void Nes_Cpu::time( nes_time_t t ) {
		cycle_count = t - base_time;
	}
#endif
	
	inline void Nes_Cpu::end_frame( nes_time_t end_time ) {
		base_time -= end_time;
		assert( time() >= 0 );
	}

#endif

