
// Game_Music_Emu 0.3.0. http://www.slack.net/~ant/

#include "Gbs_Emu.h"

#include <string.h>

#include "blargg_endian.h"

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

#ifndef RUN_GB_CPU
	#define RUN_GB_CPU( cpu, n ) cpu.run( n )
#endif

const long bank_size = 0x4000;
const gb_addr_t ram_addr = 0xa000;
const gb_addr_t halt_addr = 0x9EFE;
static BOOST::uint8_t unmapped_code [Gb_Cpu::page_size];

Gbs_Emu::equalizer_t const Gbs_Emu::handheld_eq   = { -47.0, 2000 };
Gbs_Emu::equalizer_t const Gbs_Emu::headphones_eq = {   0.0, 300 };

// RAM

int Gbs_Emu::read_ram( Gbs_Emu* emu, gb_addr_t addr )
{
	return emu->ram [addr - ram_addr];
}

void Gbs_Emu::write_ram( Gbs_Emu* emu, gb_addr_t addr, int data )
{
	emu->ram [addr - ram_addr] = data;
}

// Unmapped

int Gbs_Emu::read_unmapped( Gbs_Emu*, gb_addr_t addr )
{
	dprintf( "Read from unmapped memory $%.4x\n", (unsigned) addr );
	return 0xFF; // open bus value
}

void Gbs_Emu::write_unmapped( Gbs_Emu*, gb_addr_t addr, int )
{
	dprintf( "Wrote to unmapped memory $%.4x\n", (unsigned) addr );
}

// ROM

int Gbs_Emu::read_rom( Gbs_Emu* emu, gb_addr_t addr )
{
	return emu->rom [addr];
}

int Gbs_Emu::read_bank( Gbs_Emu* emu, gb_addr_t addr )
{
	return emu->rom_bank [addr & (bank_size - 1)];
}

void Gbs_Emu::set_bank( int n )
{
	if ( n >= bank_count )
	{
		n = 0;
		dprintf( "Set to non-existent bank %d\n", (int) n );
	}
	if ( n == 0 && bank_count > 1 )
	{
		// to do: what is the correct behavior? Current Wario Land 3 and
		// Tetris DX GBS rips require that this have no effect or set to bank 1.
		//return;
		//dprintf( "Selected ROM bank 0\n" );
	}
	rom_bank = &rom [n * bank_size];
	cpu.map_code( bank_size, bank_size, rom_bank );
}

void Gbs_Emu::write_rom( Gbs_Emu* emu, gb_addr_t addr, int data )
{
	if ( unsigned (addr - 0x2000) < 0x2000 )
		emu->set_bank( data & 0x1F );
}

// I/O: Timer, APU

void Gbs_Emu::set_timer( int modulo, int rate )
{
	if ( timer_mode )
	{
		static byte const rates [4] = { 10, 4, 6, 8 };
		play_period = (gb_time_t) (256 - modulo) << (rates [rate & 3] - double_speed);
	}
}

inline gb_time_t Gbs_Emu::clock() const
{
	return cpu_time - cpu.remain();
}
	
int Gbs_Emu::read_io( Gbs_Emu* emu, gb_addr_t addr )
{
	// hi_page is accessed most
	if ( addr >= 0xFF80 )
		return emu->hi_page [addr & 0xFF];
	
	if ( unsigned (addr - Gb_Apu::start_addr) <= Gb_Apu::register_count )
		return emu->apu.read_register( emu->clock(), addr );
	
	if ( addr == 0xFF00 )
		return 0; // joypad
	
	dprintf( "Unhandled I/O read 0x%4X\n", (unsigned) addr );
	
	return 0xFF;
}

void Gbs_Emu::write_io( Gbs_Emu* emu, gb_addr_t addr, int data )
{
	// apu is accessed most
	if ( unsigned (addr - Gb_Apu::start_addr) < Gb_Apu::register_count )
	{
		emu->apu.write_register( emu->clock(), addr, data );
	}
	else
	{
		emu->hi_page [addr & 0xFF] = data;
		
		if ( addr == 0xFF06 || addr == 0xFF07 )
			emu->set_timer( emu->hi_page [6], emu->hi_page [7] );
		
		//if ( addr == 0xFFFF )
		//  dprintf( "Wrote interrupt mask\n" );
	}
}

Gbs_Emu::Gbs_Emu( double gain ) : cpu( this )
{
	apu.volume( gain );
	
	static equalizer_t const eq = { -1.0, 120 };
	set_equalizer( eq );
	
 	// unmapped code is all HALT instructions
	memset( unmapped_code, 0x76, sizeof unmapped_code );
	
	// cpu
	cpu.reset( unmapped_code, read_unmapped, write_unmapped );
	cpu.map_memory( 0x0000, 0x4000, read_rom, write_rom );
	cpu.map_memory( 0x4000, 0x4000, read_bank, write_rom );
	cpu.map_memory( ram_addr, 0x4000, read_ram, write_ram );
	cpu.map_code(   ram_addr, 0x4000, ram );
	cpu.map_code(   0xFF00, 0x0100, hi_page );
	cpu.map_memory( 0xFF00, 0x0100, read_io, write_io );
}

Gbs_Emu::~Gbs_Emu()
{
}

void Gbs_Emu::unload()
{
	cpu.r.pc = halt_addr;
	rom.clear();
}

void Gbs_Emu::set_voice( int i, Blip_Buffer* c, Blip_Buffer* l, Blip_Buffer* r )
{
	apu.osc_output( i, c, l, r );
}

void Gbs_Emu::update_eq( blip_eq_t const& eq )
{
	apu.treble_eq( eq );
}

blargg_err_t Gbs_Emu::load( Data_Reader& in )
{
	header_t h;
	BLARGG_RETURN_ERR( in.read( &h, sizeof h ) );
	return load( h, in );
}

blargg_err_t Gbs_Emu::load( const header_t& h, Data_Reader& in )
{
	header_ = h;
	unload();
	
	// check compatibility
	if ( 0 != memcmp( header_.tag, "GBS", 3 ) )
		return "Not a GBS file";
	if ( header_.vers != 1 )
		return "Unsupported GBS format";
	
	// gather relevant fields
	load_addr = get_le16( header_.load_addr );
	init_addr = get_le16( header_.init_addr );
	play_addr = get_le16( header_.play_addr );
	stack_ptr = get_le16( header_.stack_ptr );
	double_speed = (header_.timer_mode & 0x80) != 0;
	timer_modulo_init = header_.timer_modulo;
	timer_mode = header_.timer_mode;
	if ( !(timer_mode & 0x04) )
		timer_mode = 0; // using vbl
	
	#ifndef NDEBUG
	{
		if ( header_.timer_mode & 0x78 )
			dprintf( "TAC field has extra bits set: 0x%02x\n", (unsigned) header_.timer_mode );
		
		if ( load_addr < 0x400 || load_addr >= 0x8000 ||
				init_addr < 0x400 || init_addr >= 0x8000 ||
				play_addr < 0x400 || play_addr >= 0x8000 )
			dprintf( "Load/init/play address violates GBS spec.\n" );
	}
	#endif
	
	// rom
	bank_count = (load_addr + in.remain() + bank_size - 1) / bank_size;
	BLARGG_RETURN_ERR( rom.resize( bank_count * bank_size ) );
	memset( rom.begin(), 0, rom.size() );
	blargg_err_t err = in.read( &rom [load_addr], in.remain() );
	if ( err )
	{
		unload();
		return err;
	}
	
	// cpu
	cpu.rst_base = load_addr;
	cpu.map_code( 0x0000, 0x4000, rom.begin() );
	
	set_voice_count( Gb_Apu::osc_count );
	set_track_count( header_.track_count );
	
	return setup_buffer( 4194304 );
}

const char** Gbs_Emu::voice_names() const
{
	static const char* names [] = { "Square 1", "Square 2", "Wave", "Noise" };
	return names;
}

// Emulation

static const BOOST::uint8_t sound_data [Gb_Apu::register_count] = {
	0x80, 0xBF, 0x00, 0x00, 0xBF, // square 1
	0x00, 0x3F, 0x00, 0x00, 0xBF, // square 2
	0x7F, 0xFF, 0x9F, 0x00, 0xBF, // wave
	0x00, 0xFF, 0x00, 0x00, 0xBF, // noise
	0x77, 0xF3, 0xF1, // vin/volume, status, power mode
	0, 0, 0, 0, 0, 0, 0, 0, 0, // unused
	0xAC, 0xDD, 0xDA, 0x48, 0x36, 0x02, 0xCF, 0x16, // waveform data
	0x2C, 0x04, 0xE5, 0x2C, 0xAC, 0xDD, 0xDA, 0x48
};

void Gbs_Emu::cpu_jsr( gb_addr_t addr )
{
	cpu.write( --cpu.r.sp, cpu.r.pc >> 8 );
	cpu.write( --cpu.r.sp, cpu.r.pc&0xFF );
	cpu.r.pc = addr;
}

void Gbs_Emu::start_track( int track_index )
{
	require( rom.size() ); // file must be loaded
	
	Classic_Emu::start_track( track_index );
	
	apu.reset();
	
	memset( ram, 0, sizeof ram );
	memset( hi_page, 0, sizeof hi_page );
	
	// configure hardware
	set_bank( bank_count > 1 );
	for ( int i = 0; i < (int) sizeof sound_data; i++ )
		apu.write_register( 0, i + apu.start_addr, sound_data [i] );
	play_period = 70224; // 59.73 Hz
	set_timer( timer_modulo_init, timer_mode ); // ignored if using vbl
	next_play = play_period;
	
	// set up init call
	cpu.r.a = track_index;
	cpu.r.b = 0;
	cpu.r.c = 0;
	cpu.r.d = 0;
	cpu.r.e = 0;
	cpu.r.h = 0;
	cpu.r.l = 0;
	cpu.r.flags = 0;
	cpu.r.pc = halt_addr;
	cpu.r.sp = stack_ptr;
	cpu_jsr( init_addr );
}

blip_time_t Gbs_Emu::run_clocks( blip_time_t duration, bool* added_stereo )
{
	require( rom.size() ); // file must be loaded
	
	cpu_time = 0;
	while ( cpu_time < duration )
	{
		// check for idle cpu
		if ( cpu.r.pc == halt_addr )
		{
			if ( next_play > duration )
			{
				cpu_time = duration;
				break;
			}
			
			if ( cpu_time < next_play )
				cpu_time = next_play;
			next_play += play_period;
			cpu_jsr( play_addr );
		}
		
		long count = duration - cpu_time;
		cpu_time = duration;
		Gb_Cpu::result_t result = RUN_GB_CPU( cpu, count );
		cpu_time -= cpu.remain();
		
		if ( (result == Gb_Cpu::result_halt && cpu.r.pc != halt_addr) ||
				result == Gb_Cpu::result_badop )
		{
			if ( cpu.r.pc > 0xFFFF )
			{
				dprintf( "PC wrapped around\n" );
				cpu.r.pc &= 0xFFFF;
			}
			else
			{
				log_error();
				dprintf( "Bad opcode $%.2x at $%.4x\n",
						(int) cpu.read( cpu.r.pc ), (int) cpu.r.pc );
				cpu.r.pc = (cpu.r.pc + 1) & 0xFFFF;
				cpu_time += 6;
			}
		}
	}
	
	// end time frame
	
	next_play -= cpu_time;
	if ( next_play < 0 ) // could go negative if routine is taking too long to return
		next_play = 0;
	
	if ( apu.end_frame( cpu_time ) && added_stereo )
		*added_stereo = true;
	
	return cpu_time;
}

