
// Game_Music_Emu 0.3.0. http://www.slack.net/~ant/

#include "Nsf_Emu.h"

#include <string.h>
#include <stdio.h>

#if !NSF_EMU_APU_ONLY
	#include "Nes_Vrc6_Apu.h"
	#include "Nes_Namco_Apu.h"
	#include "Nes_Fme7_Apu.h"
#endif

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

#ifndef RUN_NES_CPU
	#define RUN_NES_CPU( cpu, count ) cpu.run( count )
#endif

#ifndef NSF_BEGIN_FRAME
	#define NSF_BEGIN_FRAME()
#endif

const unsigned low_mem_size = 0x800;
const unsigned page_size = 0x1000;
const long ram_size = 0x10000;
const nes_addr_t rom_begin = 0x8000;
const nes_addr_t bank_select_addr = 0x5ff8;
const nes_addr_t exram_addr = bank_select_addr - (bank_select_addr % Nes_Cpu::page_size);
const int master_clock_divisor = 12;

const int vrc6_flag = 0x01;
const int namco_flag = 0x10;
const int fme7_flag = 0x20;

static BOOST::uint8_t unmapped_code [Nes_Cpu::page_size];

Nes_Emu::equalizer_t const Nes_Emu::nes_eq     = {  -1.0, 80 };
Nes_Emu::equalizer_t const Nes_Emu::famicom_eq = { -15.0, 80 };

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
		if ( data < emu->total_banks )
		{
			emu->cpu.map_code( (bank + 8) * page_size, page_size,
					&emu->rom [data * page_size] );
		}
		else
		{
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
	return emu->cpu.low_mem [addr];
}

void Nsf_Emu::write_low_mem( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	emu->cpu.low_mem [addr] = data;
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
	if ( addr == Nes_Namco_Apu::data_reg_addr )
		return emu->namco->read_data();
	return addr >> 8;
}

void Nsf_Emu::write_namco( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	if ( addr == Nes_Namco_Apu::data_reg_addr )
		emu->namco->write_data( emu->cpu.time(), data );
}

void Nsf_Emu::write_namco_addr( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	if ( addr == Nes_Namco_Apu::addr_reg_addr )
		emu->namco->write_addr( data );
}

// VRC6
void Nsf_Emu::write_vrc6( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	unsigned reg = addr & (Nes_Vrc6_Apu::addr_step - 1);
	unsigned osc = unsigned (addr - Nes_Vrc6_Apu::base_addr) / Nes_Vrc6_Apu::addr_step;
	if ( osc < Nes_Vrc6_Apu::osc_count && reg < Nes_Vrc6_Apu::reg_count )
		emu->vrc6->write_osc( emu->cpu.time(), osc, reg, data );
}

// FME-7
void Nsf_Emu::write_fme7( Nsf_Emu* emu, nes_addr_t addr, int data )
{
	switch ( addr & Nes_Fme7_Apu::addr_mask )
	{
		case Nes_Fme7_Apu::latch_addr:
			emu->fme7->write_latch( data );
			break;
		
		case Nes_Fme7_Apu::data_addr:
			emu->fme7->write_data( emu->cpu.time(), data );
			break;
	}
}

#endif

// Unmapped
int Nsf_Emu::read_unmapped( Nsf_Emu*, nes_addr_t addr )
{
	dprintf( "Read unmapped $%.4X\n", (unsigned) addr );
	return (addr >> 8) & 0xff; // high byte of address stays on bus
}

void Nsf_Emu::write_unmapped( Nsf_Emu*, nes_addr_t addr, int data )
{
	#ifdef NDEBUG
		return;
	#endif
	
	// some games write to $8000 and $8001 repeatedly
	if ( addr == 0x8000 || addr == 0x8001 )
		return;
	
	// probably namco sound mistakenly turned on in mck
	if ( addr == 0x4800 || addr == 0xF800 )
		return;
	
	// memory mapper?
	if ( addr == 0xFFF8 )
		return;
	
	dprintf( "write_unmapped( 0x%04X, 0x%02X )\n", (unsigned) addr, (unsigned) data );
}

Nes_Emu::Nes_Emu( double gain_ )
{
	cpu.set_emu( this );
	play_addr = 0;
	gain = gain_;
	apu.dmc_reader( pcm_read, this );
	vrc6 = NULL;
	namco = NULL;
	fme7 = NULL;
	Music_Emu::set_equalizer( nes_eq );
	
	// set unmapped code to illegal instruction
	memset( unmapped_code, 0x32, sizeof unmapped_code );
}

Nes_Emu::~Nes_Emu()
{
	unload();
}

void Nsf_Emu::unload()
{
	#if !NSF_EMU_APU_ONLY
		delete vrc6;
		vrc6 = NULL;
		
		delete namco;
		namco = NULL;
		
		delete fme7;
		fme7 = NULL;
		
	#endif
	
	rom.clear();
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
	static const char* dual_names [] = {
		"Square 1", "Square 2", "Triangle", "Noise", "DMC",
		"VRC6.1,N106.5&7", "VRC6.2,N106.4&6", "VRC6.3,N106.1-3"
	};
	
	static const char* fme7_names [] = {
		"Square 1", "Square 2", "Triangle", "Noise", "DMC",
		"Square 3", "Square 4", "Square 5"
	};
	
	if ( namco )
		return vrc6 ? dual_names : namco_names;
	
	if ( vrc6 )
		return vrc6_names;
	
	if ( fme7 )
		return fme7_names;
	
	return base_names;
}

blargg_err_t Nsf_Emu::init_sound()
{
	if ( exp_flags & ~(namco_flag | vrc6_flag | fme7_flag) )
		return "NSF requires unsupported expansion audio hardware";
	
	// map memory
	cpu.reset( unmapped_code, read_unmapped, write_unmapped );
	cpu.map_memory( 0, low_mem_size, read_low_mem, write_low_mem );
	cpu.map_code( 0, low_mem_size, cpu.low_mem );
	cpu.map_memory( 0x4000, Nes_Cpu::page_size, read_snd, write_snd );
	cpu.map_memory( exram_addr, Nes_Cpu::page_size, read_unmapped, write_exram );
	cpu.map_memory( 0x6000, sram_size, read_sram, write_sram );
	cpu.map_code  ( 0x6000, sram_size, sram );
	cpu.map_memory( rom_begin, ram_size - rom_begin, read_code, write_unmapped );
	
	set_voice_count( Nes_Apu::osc_count );
	
	double adjusted_gain = gain;
	
	#if NSF_EMU_APU_ONLY
		if ( exp_flags )
			return "NSF requires expansion audio hardware";
	#else
	
	if ( exp_flags )
		set_voice_count( Nes_Apu::osc_count + 3 );
	
	// namco
	if ( exp_flags & namco_flag )
	{
		namco = BLARGG_NEW Nes_Namco_Apu;
		BLARGG_CHECK_ALLOC( namco );
		
		adjusted_gain *= 0.75;
		cpu.map_memory( Nes_Namco_Apu::data_reg_addr, Nes_Cpu::page_size,
				read_namco, write_namco );
		cpu.map_memory( Nes_Namco_Apu::addr_reg_addr, Nes_Cpu::page_size,
				 read_code, write_namco_addr );
	}
	
	// vrc6
	if ( exp_flags & vrc6_flag )
	{
		vrc6 = BLARGG_NEW Nes_Vrc6_Apu;
		BLARGG_CHECK_ALLOC( vrc6 );
		
		adjusted_gain *= 0.75;
		for ( int i = 0; i < Nes_Vrc6_Apu::osc_count; i++ )
			cpu.map_memory( Nes_Vrc6_Apu::base_addr + i * Nes_Vrc6_Apu::addr_step,
					Nes_Cpu::page_size, read_code, write_vrc6 );
	}
	
	// fme7
	if ( exp_flags & fme7_flag )
	{
		fme7 = BLARGG_NEW Nes_Fme7_Apu;
		BLARGG_CHECK_ALLOC( fme7 );
		
		adjusted_gain *= 0.75;
		cpu.map_memory( fme7->latch_addr, ram_size - fme7->latch_addr,
				read_code, write_fme7 );
	}
	// to do: is gain adjustment even needed? other sound chip volumes should work
	// naturally with the apu without change.
	
	if ( namco )
		namco->volume( adjusted_gain );
	
	if ( vrc6 )
		vrc6->volume( adjusted_gain );
	
	if ( fme7 )
		fme7->volume( adjusted_gain );
	
#endif
	
	apu.volume( adjusted_gain );
	
	return blargg_success;
}

void Nsf_Emu::update_eq( blip_eq_t const& eq )
{
	apu.treble_eq( eq );
	
	#if !NSF_EMU_APU_ONLY
		if ( vrc6 )
			vrc6->treble_eq( eq );
		
		if ( namco )
			namco->treble_eq( eq );
		
		if ( fme7 )
			fme7->treble_eq( eq );
	#endif
}

blargg_err_t Nsf_Emu::load( Data_Reader& in )
{
	header_t h;
	BLARGG_RETURN_ERR( in.read( &h, sizeof h ) );
	return load( h, in );
}

blargg_err_t Nsf_Emu::load( const header_t& h, Data_Reader& in )
{
	header_ = h;
	unload();
	
	// check compatibility
	if ( 0 != memcmp( header_.tag, "NESM\x1A", 5 ) )
		return "Not an NSF file";
	if ( header_.vers != 1 )
		return "Unsupported NSF format";
	
	// sound and memory
	exp_flags = header_.chip_flags;
	blargg_err_t err = init_sound();
	if ( err )
		return err;
	
	// set up data
	nes_addr_t load_addr = get_le16( header_.load_addr );
	init_addr = get_le16( header_.init_addr );
	play_addr = get_le16( header_.play_addr );
	if ( !load_addr ) load_addr = rom_begin;
	if ( !init_addr ) init_addr = rom_begin;
	if ( !play_addr ) play_addr = rom_begin;
	if ( load_addr < rom_begin || init_addr < rom_begin )
		return "Invalid address in NSF";
	
	// set up rom
	total_banks = (in.remain() + load_addr % page_size + page_size - 1) / page_size;
	BLARGG_RETURN_ERR( rom.resize( total_banks * page_size ) );
	memset( rom.begin(), 0, rom.size() );
	err = in.read( &rom [load_addr % page_size], in.remain() );
	if ( err )
	{
		unload();
		return err;
	}
	
	// bank switching
	int first_bank = (load_addr - rom_begin) / page_size;
	for ( int i = 0; i < bank_count; i++ )
	{
		unsigned bank = i - first_bank;
		initial_banks [i] = (bank < (unsigned) total_banks) ? bank : 0;
		
		if ( header_.banks [i] )
		{
			// bank-switched
			memcpy( initial_banks, header_.banks, sizeof initial_banks );
			break;
		}
	}
	
	// playback rate
	unsigned playback_rate = get_le16( header_.ntsc_speed );
	unsigned standard_rate = 0x411A;
	double clock_rate = 1789772.72727;
	play_period = 262 * 341L * 4 + 2;
	pal_only = false;
	
	// use pal speed if there is no ntsc speed
	if ( (header_.speed_flags & 3) == 1 )
	{
		pal_only = true;
		play_period = 33247 * master_clock_divisor;
		clock_rate = 1662607.125;
		standard_rate = 0x4E20;
		playback_rate = get_le16( header_.pal_speed );
	}
	
	// use custom playback rate if not the standard rate
	if ( playback_rate && playback_rate != standard_rate )
		play_period = long (clock_rate * playback_rate * master_clock_divisor /
				1000000.0);
	
	// extra flags
	int extra_flags = header_.speed_flags;
	#if !NSF_EMU_EXTRA_FLAGS
		extra_flags = 0;
	#endif
	needs_long_frames = (extra_flags & 0x10) != 0;
	initial_pcm_dac = (extra_flags & 0x20) ? 0x3F : 0;

	set_track_count( header_.track_count );
	
	return setup_buffer( (long) (clock_rate + 0.5) );
}

void Nsf_Emu::set_voice( int i, Blip_Buffer* buf, Blip_Buffer*, Blip_Buffer* )
{
	if ( i < Nes_Apu::osc_count )
	{
		apu.osc_output( i, buf );
		return;
	}
	
	#if !NSF_EMU_APU_ONLY
		if ( vrc6 )
			vrc6->osc_output( i - Nes_Apu::osc_count, buf );
		
		if ( fme7 )
			fme7->osc_output( i - Nes_Apu::osc_count, buf );
		
		if ( namco )
		{
			if ( i < 7 )
			{
				i &= 1;
				namco->osc_output( i + 4, buf );
				namco->osc_output( i + 6, buf );
			}
			else
			{
				for ( int n = 0; n < namco->osc_count / 2; n++ )
					namco->osc_output( n, buf );
			}
		}
	#endif
}

void Nsf_Emu::start_track( int track )
{
	require( rom.size() ); // file must be loaded
	
	Classic_Emu::start_track( track );
	
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
		if ( namco )
			namco->reset();
		
		if ( vrc6 )
			vrc6->reset();
		
		if ( fme7 )
			fme7->reset();
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
}

void Nsf_Emu::cpu_jsr( nes_addr_t pc, int adj )
{
	unsigned addr = cpu.r.pc + adj;
	cpu.r.pc = pc;
	cpu.push_byte( addr >> 8 );
	cpu.push_byte( addr );
}

void Nsf_Emu::call_play()
{
	cpu_jsr( play_addr, -1 );
}

blip_time_t Nsf_Emu::run_clocks( blip_time_t duration, bool* )
{
	// run cpu
	cpu.set_time( 0 );
	bool first_illegal = true; // avoid swamping output with illegal instruction errors
	while ( cpu.time() < duration )
	{
		// check for idle cpu
		if ( cpu.r.pc == exram_addr )
		{
			if ( next_play > duration )
			{
				cpu.set_time( duration );
				break;
			}
			
			if ( next_play > cpu.time() )
				cpu.set_time( next_play );
			
			nes_time_t period = (play_period + play_extra) / master_clock_divisor;
			play_extra = play_period - period * master_clock_divisor;
			next_play += period;
			call_play();
		}
		
		Nes_Cpu::result_t result = RUN_NES_CPU( cpu, duration );
		if ( result == Nes_Cpu::result_badop && cpu.r.pc != exram_addr )
		{
			if ( cpu.r.pc > 0xffff )
			{
				cpu.r.pc &= 0xffff;
				dprintf( "PC wrapped around\n" );
			}
			else
			{
				cpu.r.pc = (cpu.r.pc + 1) & 0xffff;
				cpu.set_time( cpu.time() + 4 );
				log_error();
				if ( first_illegal )
				{
					first_illegal = false;
					dprintf( "Bad opcode $%.2x at $%.4x\n",
							(int) cpu.read( cpu.r.pc ), (int) cpu.r.pc );
				}
			}
		}
	}
	
	// end time frame
	duration = cpu.time();
	next_play -= duration;
	if ( next_play < 0 ) // could go negative if routine is taking too long to return
		next_play = 0;
	apu.end_frame( duration );
	
	#if !NSF_EMU_APU_ONLY
		if ( namco )
			namco->end_frame( duration );
		
		if ( vrc6 )
			vrc6->end_frame( duration );
		
		if ( fme7 )
			fme7->end_frame( duration );
		
	#endif
	
	return duration;
}

