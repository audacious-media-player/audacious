
// Nintendo Game Boy CPU emulator

// Game_Music_Emu 0.3.0

#ifndef GB_CPU_H
#define GB_CPU_H

#include "blargg_common.h"

typedef unsigned gb_addr_t; // 16-bit CPU address

class Gbs_Emu;

// Game Boy CPU emulator. Currently treats every instruction as taking 4 cycles.
class Gb_Cpu {
	typedef BOOST::uint8_t uint8_t;
	enum { page_bits = 8 };
	enum { page_count = 0x10000 >> page_bits };
	uint8_t const* code_map [page_count + 1];
	long remain_;
	Gbs_Emu* callback_data;
public:
	
	Gb_Cpu( Gbs_Emu* );
	
	// Memory read/write function types. Reader must return value from 0 to 255.
	typedef int (*reader_t)( Gbs_Emu*, gb_addr_t );
	typedef void (*writer_t)( Gbs_Emu*, gb_addr_t, int data );
	
	// Clear registers, unmap memory, and map code pages to unmapped_page.
	void reset( const void* unmapped_page = NULL, reader_t read = NULL, writer_t write = NULL );
	
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
	uint8_t* get_code( gb_addr_t ); // non-const to allow debugger to modify code
	
	// Push a byte on the stack
	void push_byte( int );
	
	// Game Boy Z80 registers. *Not* kept updated during a call to run().
	struct registers_t {
		long pc; // more than 16 bits to allow overflow detection
		BOOST::uint16_t sp;
		uint8_t flags;
		uint8_t a;
		uint8_t b;
		uint8_t c;
		uint8_t d;
		uint8_t e;
		uint8_t h;
		uint8_t l;
	};
	registers_t r;
	
	// Interrupt enable flag set by EI and cleared by DI
	bool interrupts_enabled;
	
	// Base address for RST vectors (normally 0)
	gb_addr_t rst_base;
	
	// Reasons that run() returns
	enum result_t {
		result_cycles,      // Requested number of cycles (or more) were executed
		result_halt,        // PC is at HALT instruction
		result_badop        // PC is at bad (unimplemented) instruction
	};
	
	// Run CPU for at least 'count' cycles, or until one of the above conditions
	// arises. Returns reason for stopping.
	result_t run( long count );
	
	// Number of clock cycles remaining for most recent run() call
	long remain() const;
	
private:
	// noncopyable
	Gb_Cpu( const Gb_Cpu& );
	Gb_Cpu& operator = ( const Gb_Cpu& );
	
	reader_t data_reader [page_count + 1]; // extra entry catches address overflow
	writer_t data_writer [page_count + 1];
	void set_code_page( int, uint8_t const* );
};

inline long Gb_Cpu::remain() const
{
	return remain_;
}

#endif

