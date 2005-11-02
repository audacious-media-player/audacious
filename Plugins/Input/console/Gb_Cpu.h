
// Nintendo Game Boy CPU emulator

// Game_Music_Emu 0.2.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef GB_CPU_H
#define GB_CPU_H

#include "blargg_common.h"

typedef unsigned gb_addr_t; // 16-bit address

class Gbs_Emu;

// Game Boy CPU emulator. Currently treats every instruction as taking 4 cycles.
class Gb_Cpu {
	typedef BOOST::uint8_t uint8_t;
	enum { page_bits = 8 };
	enum { page_count = 0x10000 >> page_bits };
	const uint8_t* code_map [page_count + 1];
	long remain_;
public:
	Gb_Cpu();
	
	// Set all registers to 0, unmap all memory, and map all code pages
	// to unmapped_page.
	void reset( const void* unmapped_page = NULL );
	
	// Memory read/write function types. Memory reader return value must be 0 to 255.
	Gbs_Emu* callback_data; // passed to memory read/write functions
	typedef int (*reader_t)( Gbs_Emu* callback_data, gb_addr_t );
	typedef void (*writer_t)( Gbs_Emu* callback_data, gb_addr_t, int );
	
	// Memory mapping functions take a block of memory of specified 'start' address
	// and 'size' in bytes. Both start address and size must be a multiple of page_size.
	enum { page_size = 1L << page_bits };
	
	// Map code memory to 'code' (memory accessed via the program counter)
	void map_code( gb_addr_t start, unsigned long size, const void* code );
	
	// Map data memory to read and write functions
	void map_memory( gb_addr_t start, unsigned long size, reader_t, writer_t );
	
	// Access memory as the emulated CPU does.
	int  read( gb_addr_t );
	void write( gb_addr_t, int data );
	uint8_t* get_code( gb_addr_t ); // for use in a debugger
	
	// Game Boy Z80 registers. *Not* kept updated during a call to run().
	struct registers_t {
		BOOST::uint16_t pc;
		BOOST::uint16_t sp;
		uint8_t flags;
		uint8_t a;
		uint8_t b;
		uint8_t c;
		uint8_t d;
		uint8_t e;
		uint8_t h;
		uint8_t l;
	} r;
	
	// Interrupt enable flag set by EI and cleared by DI.
	bool interrupts_enabled;
	
	// Base address for RST vectors (normally 0).
	gb_addr_t rst_base;
	
	// Reasons that run() returns
	enum result_t {
		result_cycles,      // Requested number of cycles (or more) were executed
		result_halt,        // PC is at HALT instruction
		result_badop        // PC is at bad (unimplemented) instruction
	};
	
	// Run CPU for at least 'count' cycles, or until one of the above conditions
	// arises. Return reason for stopping.
	result_t run( long count );
	
	// Number of clock cycles remaining for current run() call.
	long remain() const;
	
private:
	// noncopyable
	Gb_Cpu( const Gb_Cpu& );
	Gb_Cpu& operator = ( const Gb_Cpu& );
	
	reader_t data_reader [page_count + 1]; // extra entry to catch overflow addresses
	writer_t data_writer [page_count + 1];
};

	inline long Gb_Cpu::remain() const {
		return remain_;
	}

#endif

