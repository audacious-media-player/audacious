
// Game_Music_Emu 0.3.0. http://www.slack.net/~ant/

#include "Gym_Emu.h"

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

double const gain = 3.0;
double const oversample_factor = 5 / 3.0;

const long base_clock = 53700300;
const long clock_rate = base_clock / 15;

Gym_Emu::Gym_Emu()
{
	data = NULL;
	pos = NULL;
}

Gym_Emu::~Gym_Emu()
{
	unload();
}

void Gym_Emu::unload()
{
	data = NULL;
	pos = NULL;
	set_track_ended( false );
	mem.clear();
}

blargg_err_t Gym_Emu::set_sample_rate( long sample_rate )
{
	blip_eq_t eq( -32, 8000, sample_rate );
	apu.treble_eq( eq );
	apu.volume( 0.135 * gain );
	dac_synth.treble_eq( eq );
	dac_synth.volume( 0.125 / 256 * gain );
	
	BLARGG_RETURN_ERR( blip_buf.set_sample_rate( sample_rate, 1000 / 60 ) );
	blip_buf.clock_rate( clock_rate );
	
	double factor = Dual_Resampler::setup( oversample_factor, 0.990, gain );
	double fm_sample_rate = sample_rate * factor;
	BLARGG_RETURN_ERR( fm.set_rate( fm_sample_rate, base_clock / 7.0 ) );
	BLARGG_RETURN_ERR( Dual_Resampler::resize( sample_rate / 60 ) );
	
	return Music_Emu::set_sample_rate( sample_rate );
}

void Gym_Emu::mute_voices( int mask )
{
	Music_Emu::mute_voices( mask );
	fm.mute_voices( mask );
	dac_muted = mask & 0x40;
	apu.output( (mask & 0x80) ? NULL : &blip_buf );
}

const char** Gym_Emu::voice_names() const
{
	static const char* names [] = {
		"FM 1", "FM 2", "FM 3", "FM 4", "FM 5", "FM 6", "PCM", "PSG"
	};
	return names;
}

static blargg_err_t check_header( const Gym_Emu::header_t& h, int* data_offset = NULL )
{
	if ( memcmp( h.tag, "GYMX", 4 ) == 0 )
	{
		if ( memcmp( h.packed, "\0\0\0\0", 4 ) != 0 )
			return "Packed GYM file not supported";
		
		if ( data_offset )
			*data_offset = sizeof h;
	}
	else if ( h.tag [0] != 0 && h.tag [0] != 1 )
	{
		// not a headerless GYM
		// to do: more thorough check, or just require a damn header
		return "Not a GYM file";
	}
	
	return blargg_success;
}

blargg_err_t Gym_Emu::load_( const void* file, long data_offset, long file_size )
{
	require( blip_buf.length() );
	
	data = (const byte*) file + data_offset;
	data_end = (const byte*) file + file_size;
	
	loop_begin = NULL;
	if ( data_offset )
		header_ = *(header_t*) file;
	else
		memset( &header_, 0, sizeof header_ );
	
	set_voice_count( 8 );
	set_track_count( 1 );
	remute_voices();
	
	return blargg_success;
}

blargg_err_t Gym_Emu::load( const void* file, long file_size )
{
	unload();
	
	if ( file_size < (int) sizeof (header_t) )
		return "Not a GYM file";
	
	int data_offset = 0;
	BLARGG_RETURN_ERR( check_header( *(header_t*) file, &data_offset ) );
	
	return load_( file, data_offset, file_size );
}

blargg_err_t Gym_Emu::load( Data_Reader& in )
{
	header_t h;
	BLARGG_RETURN_ERR( in.read( &h, sizeof h ) );
	return load( h, in );
}

blargg_err_t Gym_Emu::load( const header_t& h, Data_Reader& in )
{
	unload();
	
	int data_offset = 0;
	BLARGG_RETURN_ERR( check_header( h, &data_offset ) );
	
	BLARGG_RETURN_ERR( mem.resize( sizeof h + in.remain() ) );
	memcpy( mem.begin(), &h, sizeof h );
	BLARGG_RETURN_ERR( in.read( &mem [sizeof h], mem.size() - sizeof h ) );
	
	return load_( mem.begin(), data_offset, mem.size() );
}

long Gym_Emu::track_length() const
{
	long time = 0;
	const byte* p = data;
	while ( p < data_end )
	{
		switch ( *p++ )
		{
			case 0:
				time++;
				break;
			
			case 1:
			case 2:
				p += 2;
				break;
			
			case 3:
				p += 1;
				break;
		}
	}
	return time;
}

void Gym_Emu::start_track( int track )
{
	require( data );
	
	Music_Emu::start_track( track );
	
	pos = &data [0];
	loop_remain = get_le32( header_.loop_start );
	
	prev_dac_count = 0;
	dac_enabled = false;
	dac_amp = -1;
	
	fm.reset();
	apu.reset();
	blip_buf.clear();
	Dual_Resampler::clear();
}

void Gym_Emu::run_dac( int dac_count )
{
	// Guess beginning and end of sample and adjust rate and buffer position accordingly.
	
	// count dac samples in next frame
	int next_dac_count = 0;
	const byte* p = this->pos;
	int cmd;
	while ( (cmd = *p++) != 0 )
	{
		int data = *p++;
		if ( cmd <= 2 )
			++p;
		if ( cmd == 1 && data == 0x2A )
			next_dac_count++;
	}
	
	// detect beginning and end of sample
	int rate_count = dac_count;
	int start = 0;
	if ( !prev_dac_count && next_dac_count && dac_count < next_dac_count )
	{
		rate_count = next_dac_count;
		start = next_dac_count - dac_count;
	}
	else if ( prev_dac_count && !next_dac_count && dac_count < prev_dac_count )
	{
		rate_count = prev_dac_count;
	}
	
	// Evenly space samples within buffer section being used
	blip_resampled_time_t period =
			blip_buf.resampled_duration( clock_rate / 60 ) / rate_count;
	
	blip_resampled_time_t time = blip_buf.resampled_time( 0 ) +
			period * start + (period >> 1);
	
	int dac_amp = this->dac_amp;
	if ( dac_amp < 0 )
		dac_amp = dac_buf [0];
	
	for ( int i = 0; i < dac_count; i++ )
	{
		int delta = dac_buf [i] - dac_amp;
		dac_amp += delta;
		dac_synth.offset_resampled( time, delta, &blip_buf );
		time += period;
	}
	this->dac_amp = dac_amp;
}

void Gym_Emu::parse_frame()
{
	int dac_count = 0;
	const byte* pos = this->pos;
	
	if ( loop_remain && !--loop_remain )
		loop_begin = pos; // find loop on first time through sequence
	
	int cmd;
	while ( (cmd = *pos++) != 0 )
	{
		int data = *pos++;
		if ( cmd == 1 )
		{
			int data2 = *pos++;
			if ( data != 0x2A )
			{
				if ( data == 0x2B )
					dac_enabled = (data2 & 0x80) != 0;
				
				fm.write0( data, data2 );
			}
			else if ( dac_count < (int) sizeof dac_buf )
			{
				dac_buf [dac_count] = data2;
				dac_count += dac_enabled;
			}
		}
		else if ( cmd == 2 )
		{
			fm.write1( data, *pos++ );
		}
		else if ( cmd == 3 )
		{
			apu.write_data( 0, data );
		}
		else
		{
			// to do: many GYM streams are full of errors, and error count should
			// reflect cases where music is really having problems
			//log_error(); 
			--pos; // put data back
		}
	}
	
	// loop
	if ( pos >= data_end )
	{
		if ( pos > data_end )
			log_error();
		
		if ( loop_begin )
			pos = loop_begin;
		else
			set_track_ended();
	}
	this->pos = pos;
	
	// dac
	if ( dac_count && !dac_muted )
		run_dac( dac_count );
	prev_dac_count = dac_count;
}

int Gym_Emu::play_frame( blip_time_t blip_time, int sample_count, sample_t* buf )
{
	if ( !track_ended() )
		parse_frame();
	
	apu.end_frame( blip_time );
	
	memset( buf, 0, sample_count * sizeof *buf );
	fm.run( sample_count >> 1, buf );
	
	return sample_count;
}

void Gym_Emu::play( long count, sample_t* out )
{
	require( pos );
	
	Dual_Resampler::play( count, out, blip_buf );
}

void Gym_Emu::skip( long count )
{
	// to do: figure out why total muting generated access violation on MorphOS
	const int buf_size = 1024;
	sample_t buf [buf_size];
	
	while ( count )
	{
		int n = buf_size;
		if ( n > count )
			n = count;
		count -= n;
		play( n, buf );
	}
}

