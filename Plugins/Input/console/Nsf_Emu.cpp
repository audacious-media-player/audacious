
// Game_Music_Emu 0.2.4. http://www.slack.net/~ant/libs/

#include "Nsf_Emu.h"

#include <string.h>
#include <stdio.h>

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

const unsigned low_mem_size = 0x800;
const unsigned page_size = 0x1000;
const long ram_size = 0x10000;
const nes_addr_t rom_begin = 0x8000;
const nes_addr_t bank_select_addr = 0x5ff8;
const nes_addr_t exram_addr = bank_select_addr - (bank_select_addr % Nes_Cpu::page_size);
const int master_clock_divisor = 12;

const int vrc6_flag = 0x01;
const int namco_flag = 0x10;

// ROM

int Nsf_Emu::read_code( Nsf_Emu* emu, nes_addr_t addr )
{
	return *emu->cpu.get_code( addr );
}

void Nsf_Emu::write_exram( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	unsigned bank = addr - bank_select_addr;
	if ( bank < bank_count )
	{
		if ( data < emu->total_banks ) {
			emu->cpu.map_code( (bank + 8) * page_size, page_size,
					&emu->rom [data * page_size] );
		}
		else {
			dprintf( "Bank %d out of range (%d banks total)\n",
					data, (int) emu->total_banks );
		}
	}
}

// APU

int Nsf_Emu::read_snd( Nsf_Emu* emu, nes_addr_t addr )
{
	if ( addr == Nes_Apu::status_addr )
		return emu->apu.read_status( emu->cpu.time() );
	return addr >> 8; // high byte of address stays on bus
}

void Nsf_Emu::write_snd( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	if ( unsigned (addr - Nes_Apu::start_addr) <= Nes_Apu::end_addr - Nes_Apu::start_addr )
		 emu->apu.write_register( emu->cpu.time(), addr, data );
}

int Nsf_Emu::pcm_read( void* emu, nes_addr_t addr )
{
	return ((Nsf_Emu*) emu)->cpu.read( addr );
}

// Low Mem

int Nsf_Emu::read_low_mem( Nsf_Emu* emu, nes_addr_t addr )
{
	return emu->cpu.low_mem [addr & (low_mem_size - 1)];
}

void Nsf_Emu::write_low_mem( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	emu->cpu.low_mem [addr & (low_mem_size - 1)] = data;
}

// SRAM

int Nsf_Emu::read_sram( Nsf_Emu* emu, nes_addr_t addr )
{
	return emu->sram [addr & (sram_size - 1)];
}

void Nsf_Emu::write_sram( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	emu->sram [addr & (sram_size - 1)] = data;
}

#if !NSF_EMU_APU_ONLY

// Namco
int Nsf_Emu::read_namco( Nsf_Emu* emu, nes_addr_t addr )
{
	if ( addr == Nes_Namco::data_reg_addr )
		return emu->namco.read_data();
	return addr >> 8;
}

void Nsf_Emu::write_namco( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	if ( addr == Nes_Namco::data_reg_addr )
		emu->namco.write_data( emu->cpu.time(), data );
}

void Nsf_Emu::write_namco_addr( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	if ( addr == Nes_Namco::addr_reg_addr )
		emu->namco.write_addr( data );
}

// VRC6
void Nsf_Emu::write_vrc6( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	unsigned reg = addr & (Nes_Vrc6::addr_step - 1);
	unsigned osc = unsigned (addr - Nes_Vrc6::base_addr) / Nes_Vrc6::addr_step;
	if ( osc < Nes_Vrc6::osc_count && reg < Nes_Vrc6::reg_count )
		emu->vrc6.write_osc( emu->cpu.time(), osc, reg, data );
}

#endif

// Unmapped
int Nsf_Emu::read_unmapped( Nsf_Emu*, nes_addr_t addr )
{
	dprintf( "Read unmapped $%.4X\n", (unsigned) addr );
	return addr >> 8; // high byte of address stays on bus
}

void Nsf_Emu::write_unmapped( Nsf_Emu*, nes_addr_t addr, int )
{
	if (// some games write to $8000 and $8001 repeatedly
		addr != 0x8000 && addr != 0x8001 &&
		
		// probably namco sound mistakenly turned on in mck
		addr != 0x4800 && addr != 0xF800 &&
		
		// memory mapper?
		addr != 0xFFF8 )
	{
		dprintf( "Write unmapped $%.4X\n", (unsigned) addr );
	}
}

static BOOST::uint8_t unmapped_code [Nes_Cpu::page_size];

Nsf_Emu::Nsf_Emu( double gain_ )
{
	rom = NULL;
	play_addr = 0;
	clocks_per_msec = 0;
	gain = gain_;
	cpu.callback_data = this;
	set_equalizer( equalizer_t( -8.87, 8800, 110 ) );
	apu.dmc_reader( pcm_read, this );
	// set unmapped code to illegal instruction
	memset( unmapped_code, 0x32, sizeof unmapped_code );
}

Nsf_Emu::~Nsf_Emu()
{
	unload();
}

void Nsf_Emu::unload()
{
	delete [] rom;
	rom = NULL;
}

const char** Nsf_Emu::voice_names() const
{
	static const char* base_names [] = {
		"Square 1", "Square 2", "Triangle", "Noise", "DMC"
	};
	static const char* namco_names [] = {
		"Square 1", "Square 2", "Triangle", "Noise", "DMC",
		"Namco 5&7", "Namco 4&6", "Namco 1-3"
	};
	static const char* vrc6_names [] = {
		"Square 1", "Square 2", "Triangle", "Noise", "DMC",
		"VRC6 Square 1", "VRC6 Square 2", "VRC6 Saw"
	};
	if ( exp_flags & namco_flag )
		return namco_names;
	if ( exp_flags & vrc6_flag )
		return vrc6_names;
	return base_names;
}

blargg_err_t Nsf_Emu::init_sound()
{
	if ( exp_flags & ~(namco_flag | vrc6_flag) )
		return "NSF requires unsupported expansion audio hardware";
	
	// map memory
	cpu.reset( unmapped_code );
	cpu.map_memory( 0, ram_size, read_unmapped, write_unmapped ); // unmapped
	cpu.map_memory( 0, low_mem_size, read_low_mem, write_low_mem ); // low mem
	cpu.map_code( 0, low_mem_size, cpu.low_mem );
	cpu.map_memory( 0x4000, Nes_Cpu::page_size, read_snd, write_snd ); // apu
	cpu.map_memory( exram_addr, Nes_Cpu::page_size, read_unmapped, write_exram ); // exram
	cpu.map_memory( 0x6000, sram_size, read_sram, write_sram ); // sram
	cpu.map_code  ( 0x6000, sram_size, sram );
	cpu.map_memory( rom_begin, ram_size - rom_begin, read_code, write_unmapped ); // rom
	
	voice_count_ = Nes_Apu::osc_count;
	
	double adjusted_gain = gain;
	
#if NSF_EMU_APU_ONLY
	if ( exp_flags )
		return "NSF requires expansion audio hardware";
#else
	// namco
	if ( exp_flags & namco_flag ) {
		adjusted_gain *= 0.75;
		voice_count_ += 3;
		cpu.map_memory( Nes_Namco::data_reg_addr, Nes_Cpu::page_size,
				read_namco, write_namco );
		cpu.map_memory( Nes_Namco::addr_reg_addr, Nes_Cpu::page_size,
				 read_code, write_namco_addr );
	}
	
	// vrc6
	if ( exp_flags & vrc6_flag ) {
		adjusted_gain *= 0.75;
		voice_count_ += 3;
		for ( int i = 0; i < Nes_Vrc6::osc_count; i++ )
			cpu.map_memory( Nes_Vrc6::base_addr + i * Nes_Vrc6::addr_step,
					Nes_Cpu::page_size, read_code, write_vrc6 );
	}
	
	namco.volume( adjusted_gain );
	vrc6.volume( adjusted_gain );
#endif
	
	apu.volume( adjusted_gain );
	
	return blargg_success;
}

void Nsf_Emu::update_eq( blip_eq_t const& eq )
{
#if !NSF_EMU_APU_ONLY
	vrc6.treble_eq( eq );
	namco.treble_eq( eq );
#endif
	apu.treble_eq( eq );
}

blargg_err_t Nsf_Emu::load( const header_t& h, Emu_Reader& in )
{
	unload();
	
	// check compatibility
	if ( 0 != memcmp( h.tag, "NESM\x1A", 5 ) )
		return "Not an NSF file";
	if ( h.vers != 1 )
		return "Unsupported NSF format";
	
	// sound and memory
	exp_flags = h.chip_flags;
	blargg_err_t err = init_sound();
	if ( err )
		return err;
	
	// set up data
	nes_addr_t load_addr = get_le16( h.load_addr );
	init_addr = get_le16( h.init_addr );
	play_addr = get_le16( h.play_addr );
	if ( !load_addr ) load_addr = rom_begin;
	if ( !init_addr ) init_addr = rom_begin;
	if ( !play_addr ) play_addr = rom_begin;
	if ( load_addr < rom_begin || init_addr < rom_begin )
		return "Invalid address in NSF";
	
	// set up rom
	total_banks = (in.remain() + load_addr % page_size + page_size - 1) / page_size;
	long rom_size = total_banks * page_size;
	rom = new byte [rom_size];
	if ( !rom )
		return "Out of memory";
	memset( rom, 0, rom_size );
	err = in.read( &rom [load_addr % page_size], in.remain() );
	if ( err ) {
		unload();
		return err;
	}
	
	// bank switching
	int first_bank = (load_addr - rom_begin) / page_size;
	for ( int i = 0; i < bank_count; i++ )
	{
		unsigned bank = i - first_bank;
		initial_banks [i] = (bank < total_banks) ? bank : 0;
		
		if ( h.banks [i] ) {
			// bank-switched
			memcpy( initial_banks, h.banks, sizeof initial_banks );
			break;
		}
	}
	
	// playback rate
	unsigned playback_rate = get_le16( h.ntsc_speed );
	unsigned standard_rate = 0x411A;
	double clock_rate = 1789772.72727;
	play_period = 262 * 341L * 4 + 2;
	pal_only = false;
	
	// use pal speed if there is no ntsc speed
	if ( (h.speed_flags & 3) == 1 ) {
		pal_only = true;
		play_period = 33247 * master_clock_divisor;
		clock_rate = 1662607.125;
		standard_rate = 0x4E20;
		playback_rate = get_le16( h.pal_speed );
	}
	
	clocks_per_msec = clock_rate * (1.0 / 1000.0);
	
	// use custom playback rate if not the standard rate
	if ( playback_rate && playback_rate != standard_rate )
		play_period = long (clock_rate * playback_rate * master_clock_divisor /
				1000000.0);
	
	// extra flags
	int extra_flags = h.speed_flags;
	#if !NSF_EMU_EXTRA_FLAGS
		extra_flags = 0;
	#endif
	needs_long_frames = (extra_flags & 0x10) != 0;
	initial_pcm_dac = (extra_flags & 0x20) ? 0x3F : 0;

	track_count_ = h.track_count;
	
	return setup_buffer( clock_rate + 0.5 );
}

void Nsf_Emu::set_voice( int i, Blip_Buffer* buf, Blip_Buffer*, Blip_Buffer* )
{
#if !NSF_EMU_APU_ONLY
	if ( i >= Nes_Apu::osc_count ) {
		vrc6.osc_output( i - Nes_Apu::osc_count, buf );
		if ( i < 7 ) {
			i &= 1;
			namco.osc_output( i + 4, buf );
			namco.osc_output( i + 6, buf );
		}
		else {
			for ( int n = 0; n < namco.osc_count / 2; n++ )
				namco.osc_output( n, buf );
		}
		return;
	}
#endif
	apu.osc_output( i, buf );
}

blargg_err_t Nsf_Emu::start_track( int track )
{
	require( rom ); // file must be loaded
	
	starting_track();
	
	// clear memory
	memset( cpu.low_mem, 0, sizeof cpu.low_mem );
	memset( sram, 0, sizeof sram );
	
	// initial rom banks
	for ( int i = 0; i < bank_count; ++i )
		cpu.write( bank_select_addr + i, initial_banks [i] );
	
	// reset sound
	apu.reset( pal_only, initial_pcm_dac );
	apu.write_register( 0, 0x4015, 0x0F );
	apu.write_register( 0, 0x4017, needs_long_frames ? 0x80 : 0 );
	
#if !NSF_EMU_APU_ONLY
	if ( exp_flags ) {
		namco.reset();
		vrc6.reset();
	}
#endif
	
	// reset cpu
	cpu.r.pc = exram_addr;
	cpu.r.a = track;
	cpu.r.x = pal_only;
	cpu.r.y = 0;
	cpu.r.sp = 0xFF;
	cpu.r.status = 0x04; // i flag
	
	// first call
	cpu_jsr( init_addr, -1 );
	next_play = 0;
	play_extra = 0;
	
	return blargg_success;
}

void Nsf_Emu::cpu_jsr( nes_addr_t pc, int adj )
{
	unsigned ret_addr = cpu.r.pc + adj;
	cpu.r.pc = pc;
	cpu.low_mem [cpu.r.sp-- + 0x100] = ret_addr >> 8;
	cpu.low_mem [cpu.r.sp-- + 0x100] = ret_addr;
}

blip_time_t Nsf_Emu::run( int msec, bool* )
{
	// run cpu
	blip_time_t duration = clocks_per_msec * msec;
	cpu.time( 0 );
	while ( cpu.time() < duration )
	{
		// check for idle cpu
		if ( cpu.r.pc == exram_addr )
		{
			if ( next_play > duration ) {
				cpu.time( duration );
				break;
			}
			
			if ( next_play > cpu.time() )
				cpu.time( next_play );
			
			nes_time_t period = (play_period + play_extra) / master_clock_divisor;
			play_extra = play_period - period * master_clock_divisor;
			next_play += period;
			cpu_jsr( play_addr, -1 );
		}
		
		Nes_Cpu::result_t result = cpu.run( duration );
		if ( result == Nes_Cpu::result_badop && cpu.r.pc != exram_addr )
		{
			dprintf( "Bad opcode $%.2x at $%.4x\n",
					(int) cpu.read( cpu.r.pc ), (int) cpu.r.pc );
			
			return 0; // error
		}
	}
	
	// end time frame
	duration = cpu.time();
	next_play -= duration;
	if ( next_play < 0 ) // could go negative if routine is taking too long to return
		next_play = 0;
	apu.end_frame( duration );
	
#if !NSF_EMU_APU_ONLY
	if ( exp_flags & namco_flag )
		namco.end_frame( duration );
	if ( exp_flags & vrc6_flag )
		vrc6.end_frame( duration );
#endif
	
	return duration;
}

