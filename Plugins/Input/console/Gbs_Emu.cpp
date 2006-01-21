
// Game_Music_Emu 0.2.4. http://www.slack.net/~ant/libs/

#include "Gbs_Emu.h"

#include <string.h>

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

const long clock_rate = 4194304;
const long bank_size = 0x4000;
const gb_addr_t ram_addr = 0xa000;
const gb_addr_t halt_addr = 0x9eff;
static BOOST::uint8_t unmapped_code [Gb_Cpu::page_size];

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
	return 0xff; // open bus value (probably due to pull-up resistors)
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
	if ( n >= bank_count ) {
		n = 0;
		dprintf( "Set to non-existent bank %d\n", (int) n );
	}
	if ( n == 0 && bank_count > 1 )
		dprintf( "Selected ROM bank 0\n" );
	rom_bank = &rom [n * bank_size];
	cpu.map_code( bank_size, bank_size, rom_bank );
}

void Gbs_Emu::write_rom( Gbs_Emu* emu, gb_addr_t addr, int data )
{
	if ( unsigned (addr - 0x2000) < 0x2000 )
		emu->set_bank( data & 0x1f );
}

// I/O: Timer, APU

void Gbs_Emu::set_timer( int modulo, int rate )
{
	if ( timer_mode )
		play_period = gb_time_t (256 - modulo) << (((rate - 1) & 3) * 2 + 4 - double_speed);
}

inline gb_time_t Gbs_Emu::clock() const
{
	return cpu_time - cpu.remain();
}
	
int Gbs_Emu::read_io( Gbs_Emu* emu, gb_addr_t addr )
{
	// hi_page is accessed most
	if ( addr >= 0xff80 )
		return emu->hi_page [addr & 0xff];
	
	if ( unsigned (addr - Gb_Apu::start_addr) <= Gb_Apu::register_count )
		return emu->apu.read_register( emu->clock(), addr );
	
	if ( addr == 0xff00 )
		return 0; // joypad
	
	dprintf( "Unhandled I/O read 0x%4x\n", (unsigned) addr );
	
	return 0xff;
}

void Gbs_Emu::write_io( Gbs_Emu* emu, gb_addr_t addr, int data )
{
	// apu is accessed most
	if ( unsigned (addr - Gb_Apu::start_addr) < Gb_Apu::register_count ) {
		emu->apu.write_register( emu->clock(), addr, data );
	}
	else {
		emu->hi_page [addr & 0xff] = data;
		
		if ( addr == 0xff06 || addr == 0xff07 )
			emu->set_timer( emu->hi_page [6], emu->hi_page [7] );
		
		if ( addr == 0xffff )
			dprintf( "Wrote interrupt mask\n" );
	}
}

Gbs_Emu::Gbs_Emu( double gain )
{
	rom = NULL;
	
	apu.volume( gain );
	
	// to do: decide on equalization parameters
	set_equalizer( equalizer_t( -32, 8000, 90 ) );
	
 	// unmapped code is all HALT instructions
	memset( unmapped_code, 0x76, sizeof unmapped_code );
	
	// cpu
	cpu.callback_data = this;
	cpu.reset( unmapped_code );
	cpu.map_memory( 0x0000, 0x4000, read_rom, write_rom );
	cpu.map_memory( 0x4000, 0x4000, read_bank, write_rom );
	cpu.map_memory( 0x8000, 0x8000, read_unmapped, write_unmapped );
	cpu.map_memory( ram_addr, 0x4000, read_ram, write_ram );
	cpu.map_code(   ram_addr, 0x4000, ram );
	cpu.map_code(   0xff00, 0x0100, hi_page );
	cpu.map_memory( 0xff00, 0x0100, read_io, write_io );
}

Gbs_Emu::~Gbs_Emu()
{
	unload();
}

void Gbs_Emu::unload()
{
	delete [] rom;
	rom = NULL;
	cpu.r.pc = halt_addr;
}

void Gbs_Emu::set_voice( int i, Blip_Buffer* c, Blip_Buffer* l, Blip_Buffer* r )
{
	apu.osc_output( i, c, l, r );
}

void Gbs_Emu::update_eq( blip_eq_t const& eq )
{
	apu.treble_eq( eq );
}

blargg_err_t Gbs_Emu::load( const header_t& h, Emu_Reader& in )
{
	unload();
	
	// check compatibility
	if ( 0 != memcmp( h.tag, "GBS", 3 ) )
		return "Not a GBS file";
	if ( h.vers != 1 )
		return "Unsupported GBS format";
	
	// gather relevant fields
	load_addr = get_le16( h.load_addr );
	init_addr = get_le16( h.init_addr );
	play_addr = get_le16( h.play_addr );
	stack_ptr = get_le16( h.stack_ptr );
	double_speed = (h.timer_mode & 0x80) != 0;
	timer_modulo_init = h.timer_modulo;
	timer_mode = h.timer_mode;
	if ( !(timer_mode & 0x04) )
		timer_mode = 0; // using vbl
	
	#ifndef NDEBUG
	{
		if ( h.timer_mode & 0x78 )
			dprintf( "TAC field has extra bits set: 0x%02x\n", (unsigned) h.timer_mode );
		
		if ( load_addr < 0x400 || load_addr >= 0x8000 ||
				init_addr < 0x400 || init_addr >= 0x8000 ||
				play_addr < 0x400 || play_addr >= 0x8000 )
			dprintf( "Load/init/play address violates GBS spec.\n" );
	}
	#endif
	
	// rom
	bank_count = (load_addr + in.remain() + bank_size - 1) / bank_size;
	long rom_size = bank_count * bank_size;
	rom = new BOOST::uint8_t [rom_size];
	if ( !rom )
		return "Out of memory";
	memset( rom, 0, rom_size );
	blargg_err_t err = in.read( &rom [load_addr], in.remain() );
	if ( err ) {
		unload();
		return err;
	}
	
	// cpu
	cpu.rst_base = load_addr;
	cpu.map_code( 0x0000, 0x4000, rom );
	
	voice_count_ = Gb_Apu::osc_count;
	track_count_ = h.track_count;
	
	return setup_buffer( clock_rate );
}

const char** Gbs_Emu::voice_names() const
{
	static const char* names [] = { "Square 1", "Square 2", "Wave", "Noise" };
	return names;
}

// Emulation

static const BOOST::uint8_t sound_data [Gb_Apu::register_count] = {
	0x80, 0xbf, 0x00, 0x00, 0xbf, // square 1
	0x00, 0x3f, 0x00, 0x00, 0xbf, // square 2
	0x7f, 0xff, 0x9f, 0x00, 0xbf, // wave
	0x00, 0xff, 0x00, 0x00, 0xbf, // noise
	
	0x77, 0xf3, 0xf1, // vin/volume, status, power mode
	
	0, 0, 0, 0, 0, 0, 0, 0, 0, // unused
	
	0xac, 0xdd, 0xda, 0x48, 0x36, 0x02, 0xcf, 0x16, // waveform data
	0x2c, 0x04, 0xe5, 0x2c, 0xac, 0xdd, 0xda, 0x48
};

void Gbs_Emu::cpu_jsr( gb_addr_t addr )
{
	cpu.write( --cpu.r.sp, cpu.r.pc >> 8 );
	cpu.write( --cpu.r.sp, cpu.r.pc&0xff );
	cpu.r.pc = addr;
}

blargg_err_t Gbs_Emu::start_track( int track_index )
{
	require( rom ); // file must be loaded
	require( (unsigned) track_index < track_count() );
	
	starting_track();
	
	apu.reset();
	
	memset( ram, 0, sizeof ram );
	memset( hi_page, 0, sizeof hi_page );
	
	// configure hardware
	set_bank( bank_count > 1 );
	for ( int i = 0; i < sizeof sound_data; i++ )
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
	
	return blargg_success;
}

blip_time_t Gbs_Emu::run( int msec, bool* added_stereo )
{
	require( rom ); // file must be loaded
	
	gb_time_t duration = (gb_time_t) (clock_rate * (1.0 / 1000.0) * msec);
	cpu_time = 0;
	while ( cpu_time < duration )
	{
		// check for idle cpu
		if ( cpu.r.pc == halt_addr )
		{
			if ( next_play > duration ) {
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
		Gb_Cpu::result_t result = cpu.run( count );
		cpu_time -= cpu.remain();
		
		if ( (result == Gb_Cpu::result_halt && cpu.r.pc != halt_addr) ||
				result == Gb_Cpu::result_badop )
		{
			if ( result == Gb_Cpu::result_halt && cpu.r.pc < cpu.page_size )
			{
				dprintf( "PC wrapped around\n" );
			}
			else
			{
				dprintf( "Bad opcode $%.2x at $%.4x\n",
						(int) cpu.read( cpu.r.pc ), (int) cpu.r.pc );
				return 0; // error
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

Gbs_Reader::Gbs_Reader() : file( NULL ) {
}

Gbs_Reader::~Gbs_Reader() {
	close();
}

blargg_err_t Gbs_Reader::read_head(Gbs_Emu::header_t *header) {
	vfs_fread(&header->tag,        1, 3,file);
	vfs_fread(&header->vers,       1, 1,file);
	vfs_fread(&header->track_count,1, 1,file);
	vfs_fread(&header->first_track,1, 1,file);
	vfs_fread(&header->load_addr,  1, 2,file);
	vfs_fread(&header->init_addr,  1, 2,file);
	vfs_fread(&header->play_addr,  1, 2,file);
	vfs_fread(&header->stack_ptr,  1, 2,file);
	vfs_fread(&header->timer_modulo,1, 1,file);
	vfs_fread(&header->timer_mode, 1, 2,file);
	vfs_fread(&header->game,       1,32,file);
	vfs_fread(&header->author,     1,32,file);
	vfs_fread(&header->copyright,  1,32,file);
}
