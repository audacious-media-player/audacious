
// Game_Music_Emu 0.2.4. http://www.slack.net/~ant/libs/

#include "Nes_Cpu.h"

#include <limits.h>

#include "blargg_endian.h"

/* Copyright (C) 2003-2005 Shay Green. This module is free software; you
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

// Common instructions:
// 7.40%  D0    BNE
// 6.05%  F0    BEQ
// 5.46%  BD    LDA abs,X
// 4.44%  C8    INY
// 4.32%  85    STA zp
// 4.29%  C9    CMP imm
// 3.74%  20    JSR
// 3.66%  60    RTS
// 3.23%  8D    STA abs
// 3.02%  AD    LDA abs

#ifndef NES_CPU_FIXED_CYCLES
	#define NES_CPU_FIXED_CYCLES 0
#endif

static void write_unmapped( Nsf_Emu*, nes_addr_t, int )
{
	check( false );
}

static int read_unmapped( Nsf_Emu*, nes_addr_t )
{
	check( false );
	return 0;
}

Nes_Cpu::Nes_Cpu()
{
	callback_data = NULL;
	data_writer [page_count] = write_unmapped;
	data_reader [page_count] = read_unmapped;
	reset();
}

void Nes_Cpu::reset( const void* unmapped_code_page )
{
	r.status = 0;
	r.sp = 0;
	r.pc = 0;
	r.a = 0;
	r.x = 0;
	r.y = 0;
	
	cycle_count = 0;
	base_time = 0;
	#if NES_CPU_IRQ_SUPPORT
		cycle_limit = 0;
	#endif
	
	for ( int i = 0; i < page_count + 1; i++ )
		code_map [i] = (uint8_t*) unmapped_code_page;
	
	map_memory( 0, 0x10000, read_unmapped, write_unmapped );
}

void Nes_Cpu::map_code( nes_addr_t start, unsigned long size, const void* data )
{
	// start end end must fall on page bounadries
	require( start % page_size == 0 && size % page_size == 0 );
	
	unsigned first_page = start / page_size;
	for ( unsigned i = size / page_size; i--; )
		code_map [first_page + i] = (uint8_t*) data + i * page_size;
}

void Nes_Cpu::map_memory( nes_addr_t start, unsigned long size, reader_t read, writer_t write )
{
	// start end end must fall on page bounadries
	require( start % page_size == 0 && size % page_size == 0 );
	
	if ( !read )
		read = read_unmapped;
	if ( !write )
		write = write_unmapped;
	unsigned first_page = start / page_size;
	for ( unsigned i = size / page_size; i--; ) {
		data_reader [first_page + i] = read;
		data_writer [first_page + i] = write;
	}
}

// Note: 'addr' is evaulated more than once in some of the macros, so it
// must not contain side-effects.

#define LOW_MEM( a )        (low_mem [int (a)])

#define READ( addr )        (data_reader [(addr) >> page_bits]( callback_data, addr ))
#define WRITE( addr, data ) (data_writer [(addr) >> page_bits]( callback_data, addr, data ))

#define READ_PROG( addr )   (code_map [(addr) >> page_bits] [(addr) & (page_size - 1)])
#define READ_PROG16( addr ) GET_LE16( &READ_PROG( addr ) )

#define PUSH( v )   (*--sp = (v))
#define POP()       (*sp++)

int Nes_Cpu::read( nes_addr_t addr ) {
	return READ( addr );
}
	
void Nes_Cpu::write( nes_addr_t addr, int value ) {
	WRITE( addr, value );
}

#ifndef NES_CPU_GLUE_ONLY

static const unsigned char cycle_table [256] = {                             
	7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
	2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
	6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
	2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
	6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
	2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
	6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
	2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
	2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
	2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
	2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
	2,5,2,5,4,4,4,4,2,4,2,4,4,4,4,4,
	2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
	2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
	2,6,3,8,3,3,5,5,2,2,2,2,4,4,6,6,
	2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7
};

#include BLARGG_ENABLE_OPTIMIZER

// on machines with relatively few registers:
// use local temporaries (if no function calls) over all other variables
// these are most likely to be in registers, so use them over others if possible:
// pc
// nz (so calculate with nz first, then assign to other variable if necessary)
// data
// this
Nes_Cpu::result_t Nes_Cpu::run( nes_time_t end )
{
	#if NES_CPU_IRQ_SUPPORT
		end_time( end );
	#else
		cycle_count -= (end - base_time);
		base_time = end;
	#endif
	
	result_t result = result_cycles;
	
#if BLARGG_CPU_POWERPC
	// cache commonly-used values in registers
	long cycle_count = this->cycle_count;
	writer_t* const data_writer = this->data_writer;
	reader_t* const data_reader = this->data_reader;
	uint8_t* const low_mem = this->low_mem;
#endif
	
	// stack pointer is kept one greater than usual 6502 stack pointer
	#define SET_SP( v )     (sp = low_mem + 0x101 + (v))
	#define GET_SP()        (sp - 0x101 - low_mem)
	uint8_t* sp;
	SET_SP( r.sp );
	
	// registers
	unsigned pc = r.pc;
	int a = r.a;
	int x = r.x;
	int y = r.y;
	
	// status flags
	
	#define IS_NEG (int ((nz + 0x800) | (nz << (CHAR_BIT * sizeof (int) - 8))) < 0)
	
	const int st_n = 0x80;
	const int st_v = 0x40;
	const int st_r = 0x20;
	const int st_b = 0x10;
	const int st_d = 0x08;
	const int st_i = 0x04;
	const int st_z = 0x02;
	const int st_c = 0x01;
	
	#define CALC_STATUS( out ) do {                 \
		out = status & ~(st_n | st_z | st_c);       \
		out |= (c >> 8) & st_c;                     \
		if ( IS_NEG ) out |= st_n;                  \
		if ( (nz & 0xFF) == 0 ) out |= st_z;        \
	} while ( false )       

	#define SET_STATUS( in ) do {                           \
		status = in & ~(st_n | st_z | st_c | st_b | st_r);  \
		c = in << 8;                                        \
		nz = in << 4;                                       \
		nz &= 0x820;                                        \
		nz ^= ~0xDF;                                        \
	} while ( false )
	
	uint8_t status;
	int c;  // store C as 'c' & 0x100.
	int nz; // store Z as 'nz' & 0xFF == 0. see above for encoding of N.
	{
		int temp = r.status;
		SET_STATUS( temp );
	}

	goto loop;
	
	unsigned data;
	
branch_taken: {
	unsigned old_pc = pc;
	pc += (BOOST::int8_t) data;
	if ( !NES_CPU_FIXED_CYCLES )
		cycle_count += 1 + (((old_pc ^ (pc + 1)) >> 8) & 1);
}
inc_pc_loop:            
	pc++;
loop:

	check( (unsigned) pc < 0x10000 );
	check( (unsigned) GET_SP() < 0x100 );
	check( (unsigned) a < 0x100 );
	check( (unsigned) x < 0x100 );
	check( (unsigned) y < 0x100 );
	
	// Read opcode and first operand. Optimize if processor's byte order is known
	// and non-portable constructs are allowed.
#if BLARGG_NONPORTABLE && BLARGG_BIG_ENDIAN
	data = *(BOOST::uint16_t*) &READ_PROG( pc );
	pc++;
	unsigned opcode = data >> 8;
	data = (uint8_t) data;

#elif BLARGG_NONPORTABLE && BLARGG_LITTLE_ENDIAN
	data = *(BOOST::uint16_t*) &READ_PROG( pc );
	pc++;
	unsigned opcode = (uint8_t) data;
	data >>= 8;

#else
	unsigned opcode = READ_PROG( pc );
	pc++;
	data = READ_PROG( pc );
	
#endif

	if ( cycle_count >= cycle_limit )
		goto stop;
	
	cycle_count += NES_CPU_FIXED_CYCLES ? NES_CPU_FIXED_CYCLES : cycle_table [opcode];
	
	#if BLARGG_CPU_POWERPC
		this->cycle_count = cycle_count;
	#endif
	
	switch ( opcode )
	{
#define ADD_PAGE        (pc++, data += 0x100 * READ_PROG( pc ));
#define GET_ADDR()      READ_PROG16( pc )

#if NES_CPU_FIXED_CYCLES
	#define HANDLE_PAGE_CROSSING( lsb )
#else
	#define HANDLE_PAGE_CROSSING( lsb ) cycle_count += (lsb) >> 8;
#endif

#define INC_DEC_XY( reg, n )        \
	reg = uint8_t (nz = reg + n);   \
	goto loop;

#define IND_Y {                                                 \
		int temp = LOW_MEM( data ) + y;                         \
		data = temp + 0x100 * LOW_MEM( uint8_t (data + 1) );    \
		HANDLE_PAGE_CROSSING( temp );                           \
	}
	
#define IND_X {                                                 \
		int temp = data + x;                                    \
		data = LOW_MEM( uint8_t (temp) ) + 0x100 * LOW_MEM( uint8_t (temp + 1) ); \
	}
	
#define ARITH_ADDR_MODES( op )          \
case op - 0x04: /* (ind,x) */           \
	IND_X                               \
	goto ptr##op;                       \
case op + 0x0C: /* (ind),y */           \
	IND_Y                               \
	goto ptr##op;                       \
case op + 0x10: /* zp,X */              \
	data = uint8_t (data + x);          \
case op + 0x00: /* zp */                \
	data = LOW_MEM( data );             \
	goto imm##op;                       \
case op + 0x14: /* abs,Y */             \
	data += y;                          \
	goto ind##op;                       \
case op + 0x18: /* abs,X */             \
	data += x;                          \
ind##op:                                \
	HANDLE_PAGE_CROSSING( data );       \
case op + 0x08: /* abs */               \
	ADD_PAGE                            \
ptr##op:                                \
	data = READ( data );                \
case op + 0x04: /* imm */               \
imm##op:                                \

// Often-Used

	case 0xD0: // BNE
		if ( (uint8_t) nz )
			goto branch_taken;
		goto inc_pc_loop;
	
	case 0xF0: // BEQ
		if ( !(uint8_t) nz )
			goto branch_taken;
		goto inc_pc_loop;
	
	case 0xBD: // LDA abs,X
		data += x;
	lda_ind_common:
		HANDLE_PAGE_CROSSING( data );
	case 0xAD: // LDA abs
		ADD_PAGE
	lda_ptr:
		nz = a = READ( data );
		goto inc_pc_loop;

	case 0xC8: INC_DEC_XY( y, 1 )  // INY

	case 0x95: // STA zp,x
		data = uint8_t (data + x);
	case 0x85: // STA zp
		LOW_MEM( data ) = a;
		goto inc_pc_loop;
	
	ARITH_ADDR_MODES( 0xC5 ) // CMP
		nz = a - data;
	compare_common:
		c = ~nz;
		goto inc_pc_loop;
	
	case 0x4C: // JMP abs
		pc++;
		goto jmp_common;
	
	case 0x20: // JSR
		pc++;
		PUSH( pc >> 8 );
		PUSH( pc );
	jmp_common:
		pc = data + 0x100 * READ_PROG( pc );
		goto loop;
	
	case 0x60: // RTS
		// often-used instruction
		pc = POP();
		pc += 0x100 * POP();
		goto inc_pc_loop;

	case 0x9D: // STA abs,X
		data += x;
	sta_ind_common:
		HANDLE_PAGE_CROSSING( data );
	case 0x8D: // STA abs
		ADD_PAGE
	sta_ptr:
		WRITE( data, a );
		goto inc_pc_loop;
	
	case 0xB5: // LDA zp,x
		data = uint8_t (data + x);
	case 0xA5: // LDA zp
		data = LOW_MEM( data );
	case 0xA9: // LDA #imm
		a = nz = data;
		goto inc_pc_loop;

	case 0xA8: // TAY
		y = a;
	case 0x98: // TYA
		a = nz = y;
		goto loop;
	
// Branch

#define BRANCH( cond )      \
	if ( cond )             \
		goto branch_taken;  \
	goto inc_pc_loop;

	case 0x50: // BVC
		BRANCH( !(status & st_v) )
	
	case 0x70: // BVS
		BRANCH( status & st_v )
	
	case 0xB0: // BCS
		BRANCH( c & 0x100 )
	
	case 0x90: // BCC
		BRANCH( !(c & 0x100) )
	
	case 0x10: // BPL
		BRANCH( !IS_NEG )
	
	case 0x30: // BMI
		BRANCH( IS_NEG )
	
// Load/store
	
	case 0x94: // STY zp,x
		data = uint8_t (data + x);
	case 0x84: // STY zp
		LOW_MEM( data ) = y;
		goto inc_pc_loop;
	
	case 0x96: // STX zp,y
		data = uint8_t (data + y);
	case 0x86: // STX zp
		LOW_MEM( data ) = x;
		goto inc_pc_loop;
	
	case 0xB6: // LDX zp,y
		data = uint8_t (data + y);
	case 0xA6: // LDX zp
		data = LOW_MEM( data );
	case 0xA2: // LDX imm
		x = data;
		nz = data;
		goto inc_pc_loop;
	
	case 0xB4: // LDY zp,x
		data = uint8_t (data + x);
	case 0xA4: // LDY zp
		data = LOW_MEM( data );
	case 0xA0: // LDY imm
		y = data;
		nz = data;
		goto inc_pc_loop;
	
	case 0xB1: // LDA (ind),Y
		IND_Y
		goto lda_ptr;
	
	case 0xA1: // LDA (ind,X)
		IND_X
		goto lda_ptr;
	
	case 0xB9: // LDA abs,Y
		data += y;
		goto lda_ind_common;
	
	case 0x91: // STA (ind),Y
		IND_Y
		goto sta_ptr;
	
	case 0x81: // STA (ind,X)
		IND_X
		goto sta_ptr;
	
	case 0x99: // STA abs,Y
		data += y;
		goto sta_ind_common;
	
	case 0xBC: // LDY abs,X
		data += x;
		HANDLE_PAGE_CROSSING( data );
	case 0xAC:{// LDY abs
		pc++;
		unsigned addr = data + 0x100 * READ_PROG( pc );
		y = nz = READ( addr );
		goto inc_pc_loop;
	}
	
	case 0xBE: // LDX abs,y
		data += y;
		HANDLE_PAGE_CROSSING( data );
	case 0xAE:{// LDX abs
		pc++;
		unsigned addr = data + 0x100 * READ_PROG( pc );
		x = nz = READ( addr );
		goto inc_pc_loop;
	}
	
	{
		int temp;
	case 0x8C: // STY abs
		temp = y;
		goto store_abs;
	
	case 0x8E: // STX abs
		temp = x;
	store_abs:
		int addr = GET_ADDR();
		WRITE( addr, temp );
		pc += 2;
		goto loop;
	}

// Compare

	case 0xEC:{// CPX abs
		unsigned addr = GET_ADDR();
		pc++;
		data = READ( addr );
		goto cpx_data;
	}
	
	case 0xE4: // CPX zp
		data = LOW_MEM( data );
	case 0xE0: // CPX imm
	cpx_data:
		nz = x - data;
		goto compare_common;
	
	case 0xCC:{// CPY abs
		unsigned addr = GET_ADDR();
		pc++;
		data = READ( addr );
		goto cpy_data;
	}
	
	case 0xC4: // CPY zp
		data = LOW_MEM( data );
	case 0xC0: // CPY imm
	cpy_data:
		nz = y - data;
		goto compare_common;
	
// Logical

	ARITH_ADDR_MODES( 0x25 ) // AND
		nz = (a &= data);
		goto inc_pc_loop;
	
	ARITH_ADDR_MODES( 0x45 ) // EOR
		nz = (a ^= data);
		goto inc_pc_loop;
	
	ARITH_ADDR_MODES( 0x05 ) // ORA
		nz = (a |= data);
		goto inc_pc_loop;
	
// Add/Subtract

	ARITH_ADDR_MODES( 0x65 ) // ADC
	adc_imm: {
		int carry = (c >> 8) & 1;
		int ov = (a ^ 0x80) + carry + (BOOST::int8_t) data; // sign-extend
		c = nz = a + data + carry;
		a = (uint8_t) nz;
		status = (status & ~st_v) | ((ov >> 2) & 0x40);
		goto inc_pc_loop;
	}
	
	ARITH_ADDR_MODES( 0xE5 ) // SBC
	case 0xEB: // unofficial equivalent
		data ^= 0xFF;
		goto adc_imm;
	
	case 0x2C:{// BIT abs
		unsigned addr = GET_ADDR();
		pc++;
		nz = READ( addr );
		goto bit_common;
	}
	
	case 0x24: // BIT zp
		nz = LOW_MEM( data );
	bit_common:
		status = (status & ~st_v) | (nz & st_v);
		if ( !(a & nz) ) // use special encoding since N and Z might both be set
			nz = ((nz & 0x80) << 4) ^ ~0xFF;
		goto inc_pc_loop;
		
// Shift/Rotate

	case 0x4A: // LSR A
		c = 0;
	case 0x6A: // ROR A
		nz = (c >> 1) & 0x80; // could use bit insert macro here
		c = a << 8;
		nz |= a >> 1;
		a = nz;
		goto loop;

	case 0x0A: // ASL A
		nz = a << 1;
		c = nz;
		a = (uint8_t) nz;
		goto loop;

	case 0x2A: { // ROL A
		nz = a << 1;
		int temp = (c >> 8) & 1;
		c = nz;
		nz |= temp;
		a = (uint8_t) nz;
		goto loop;
	}
	
	case 0x3E: // ROL abs,X
		data += x;
		goto rol_abs;
	
	case 0x1E: // ASL abs,X
		data += x;
	case 0x0E: // ASL abs
		c = 0;
	case 0x2E: // ROL abs
	rol_abs:
		HANDLE_PAGE_CROSSING( data );
		ADD_PAGE
		nz = (c >> 8) & 1;
		nz |= (c = READ( data ) << 1);
	rotate_common:
		WRITE( data, (uint8_t) nz );
		goto inc_pc_loop;
	
	case 0x7E: // ROR abs,X
		data += x;
		goto ror_abs;
	
	case 0x5E: // LSR abs,X
		data += x;
	case 0x4E: // LSR abs
		c = 0;
	case 0x6E: // ROR abs
	ror_abs: {
		HANDLE_PAGE_CROSSING( data );
		ADD_PAGE
		int temp = READ( data );
		nz = ((c >> 1) & 0x80) | (temp >> 1);
		c = temp << 8;
		goto rotate_common;
	}
	
	case 0x76: // ROR zp,x
		data = uint8_t (data + x);
		goto ror_zp;
	
	case 0x56: // LSR zp,x
		data = uint8_t (data + x);
	case 0x46: // LSR zp
		c = 0;
	case 0x66: // ROR zp
	ror_zp: {
		int temp = LOW_MEM( data );
		nz = ((c >> 1) & 0x80) | (temp >> 1);
		c = temp << 8;
		goto write_nz_zp;
	}
	
	case 0x36: // ROL zp,x
		data = uint8_t (data + x);
		goto rol_zp;
	
	case 0x16: // ASL zp,x
		data = uint8_t (data + x);
	case 0x06: // ASL zp
		c = 0;
	case 0x26: // ROL zp
	rol_zp:
		nz = (c >> 8) & 1;
		nz |= (c = LOW_MEM( data ) << 1);
		goto write_nz_zp;
	
// Increment/Decrement

	case 0xE8: INC_DEC_XY( x, 1 )  // INX
	case 0xCA: INC_DEC_XY( x, -1 ) // DEX
	case 0x88: INC_DEC_XY( y, -1 ) // DEY
	
	case 0xF6: // INC zp,x
		data = uint8_t (data + x);
	case 0xE6: // INC zp
		nz = 1;
		goto add_nz_zp;
	
	case 0xD6: // DEC zp,x
		data = uint8_t (data + x);
	case 0xC6: // DEC zp
		nz = -1;
	add_nz_zp:
		nz += LOW_MEM( data );
	write_nz_zp:
		LOW_MEM( data ) = nz;
		goto inc_pc_loop;
	
	case 0xFE: // INC abs,x
		HANDLE_PAGE_CROSSING( data + x );
		data = x + GET_ADDR();
		goto inc_ptr;
	
	case 0xEE: // INC abs
		data = GET_ADDR();
	inc_ptr:
		nz = 1;
		goto inc_common;
	
	case 0xDE: // DEC abs,x
		HANDLE_PAGE_CROSSING( data + x );
		data = x + GET_ADDR();
		goto dec_ptr;
	
	case 0xCE: // DEC abs
		data = GET_ADDR();
	dec_ptr:
		nz = -1;
	inc_common:
		nz += READ( data );
		pc += 2;
		WRITE( data, (uint8_t) nz );
		goto loop;
		
// Transfer

	case 0xAA: // TAX
		x = a;
	case 0x8A: // TXA
		a = nz = x;
		goto loop;

	case 0x9A: // TXS
		SET_SP( x ); // verified (no flag change)
		goto loop;
	
	case 0xBA: // TSX
		x = nz = GET_SP();
		goto loop;
	
// Stack
	
	case 0x48: // PHA
		PUSH( a ); // verified
		goto loop;
		
	case 0x68: // PLA
		a = nz = POP();
		goto loop;
		
	{
		int temp;
	case 0x40: // RTI
		temp = POP();
		pc = POP();
		pc |= POP() << 8;
		goto set_status;
	case 0x28: // PLP
		temp = POP();
	set_status:
		#if !NES_CPU_IRQ_SUPPORT
			SET_STATUS( temp );
			goto loop;
		#endif
		data = status & st_i;
		SET_STATUS( temp );
		if ( !(data & ~status) )
			goto loop;
		result = result_cli; // I flag was just cleared
		goto end;
	}
	
	case 0x08: { // PHP
		int temp;
		CALC_STATUS( temp );
		temp |= st_b | st_r;
		PUSH( temp );
		goto loop;
	}
	
	case 0x6C: // JMP (ind)
		data = GET_ADDR();
		pc = READ( data );
		data++;
		pc |= READ( data ) << 8;
		goto loop;
	
	case 0x00: { // BRK
		pc += 2; // verified
		PUSH( pc >> 8 );
		PUSH( pc );
		status |= st_i;
		int temp;
		CALC_STATUS( temp );
		PUSH( temp | st_b | st_r );
		pc = READ_PROG16( 0xFFFE );
		goto loop;
	}
	
// Flags

	case 0x38: // SEC
		c = ~0;
		goto loop;
	
	case 0x18: // CLC
		c = 0;
		goto loop;
		
	case 0xB8: // CLV
		status &= ~st_v;
		goto loop;
	
	case 0xD8: // CLD
		status &= ~st_d;
		goto loop;
	
	case 0xF8: // SED
		status |= st_d;
		goto loop;
	
	case 0x58: // CLI
		#if !NES_CPU_IRQ_SUPPORT
			status &= ~st_i;
			goto loop;
		#endif
		if ( !(status & st_i) )
			goto loop;
		status &= ~st_i;
		result = result_cli;
		goto end;
		
	case 0x78: // SEI
		status |= st_i;
		goto loop;

// undocumented

	case 0x0C: case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: // SKW
		pc++;
	case 0x74: case 0x04: case 0x14: case 0x34: case 0x44: case 0x54: case 0x64: // SKB
	case 0x80: case 0x82: case 0x89: case 0xC2: case 0xD4: case 0xE2: case 0xF4:
		goto inc_pc_loop;

	case 0xEA: case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: // NOP
		goto loop;

// unimplemented
	
	case 0x9B: // TAS
	case 0x9C: // SAY
	case 0x9E: // XAS
	case 0x93: // AXA
	case 0x9F: // AXA
	case 0x0B: // ANC
	case 0x2B: // ANC
	case 0xBB: // LAS
	case 0x4B: // ALR
	case 0x6B: // AAR
	case 0x8B: // XAA
	case 0xAB: // OAL
	case 0xCB: // SAX
	case 0x02: case 0x12: case 0x22: case 0x32: // ?
	case 0x42: case 0x52: case 0x62: case 0x72:
	case 0x92: case 0xB2: case 0xD2: case 0xF2:
	case 0x83: case 0x87: case 0x8F: case 0x97: // AXS
	case 0xA3: case 0xA7: case 0xAF: case 0xB3: case 0xB7: case 0xBF: // LAX
	case 0xE3: case 0xE7: case 0xEF: case 0xF3: case 0xF7: case 0xFB: case 0xFF: // INS
	case 0xC3: case 0xC7: case 0xCF: case 0xD3: case 0xD7: case 0xDB: case 0xDF: // DCM
	case 0x63: case 0x67: case 0x6F: case 0x73: case 0x77: case 0x7B: case 0x7F: // RRA
	case 0x43: case 0x47: case 0x4F: case 0x53: case 0x57: case 0x5B: case 0x5F: // LSE
	case 0x23: case 0x27: case 0x2F: case 0x33: case 0x37: case 0x3B: case 0x3F: // RLA
	case 0x03: case 0x07: case 0x0F: case 0x13: case 0x17: case 0x1B: case 0x1F: // ASO
		result = result_badop;
		goto stop;
	}
	assert( false );
	
stop:
	pc--;
end:
	
	{
		int temp;
		CALC_STATUS( temp );
		r.status = temp;
	}
	
	base_time += cycle_count;
	#if NES_CPU_IRQ_SUPPORT
		this->cycle_limit -= cycle_count;
	#endif
	this->cycle_count = 0;
	r.pc = pc;
	r.sp = GET_SP();
	r.a = a;
	r.x = x;
	r.y = y;
	
	return result;
}

#endif

