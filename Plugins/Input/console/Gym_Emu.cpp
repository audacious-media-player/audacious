
// Game_Music_Emu 0.2.4. http://www.slack.net/~ant/libs/

#include "Gym_Emu.h"

#include "ym2612.h"

#include <string.h>

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

const long base_clock = 53700300;
const long clock_rate = base_clock / 15;

Gym_Emu::Gym_Emu()
{
	data = NULL;
	pos = NULL;
	mem = NULL;
	pairs_per_frame = 0;
}

Gym_Emu::~Gym_Emu()
{
	unload();
}

void Gym_Emu::unload()
{
	delete [] mem;
	mem = NULL;
	data = NULL;
	pos = NULL;
	track_ended_ = false;
}

blargg_err_t Gym_Emu::init( long sample_rate, double gain, double oversample_ )
{
	require( oversample_ <= 4.0 ); 
	
	blip_eq_t eq( -32, 8000, sample_rate );
	apu.treble_eq( eq );
	apu.volume( 0.27 * gain );
	dac_synth.treble_eq( eq );
	dac_synth.volume( 0.25 * gain );
	oversample = resampler.time_ratio( oversample_, 0.990, gain );

	pairs_per_frame = sample_rate / 60;
	oversamples_per_frame = int (pairs_per_frame * oversample) * 2 + 2;
	clocks_per_sample = (double) clock_rate / sample_rate;
	
	BLARGG_RETURN_ERR( resampler.buffer_size( oversamples_per_frame + 256 ) );
	
	BLARGG_RETURN_ERR( blip_buf.sample_rate( sample_rate, 1000 / 30 ) );
	
	BLARGG_RETURN_ERR( fm.set_rate( sample_rate * oversample, base_clock / 7 ) );
	
	blip_buf.clock_rate( clock_rate );
	
	return blargg_success;
}

void Gym_Emu::mute_voices( int mask )
{
	fm.mute_voices( mask );
	dac_disabled = mask & 0x40;
	apu.output( (mask & 0x80) ? NULL : &blip_buf );
}

const char** Gym_Emu::voice_names() const
{
	static const char* names [] = {
		"FM 1", "FM 2", "FM 3", "FM 4", "FM 5", "FM 6", "PCM", "SN76489"
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
	else if ( h.tag [0] != 0 && h.tag [0] != 1 ) {
		// not a headerless GYM
		// to do: more thorough check, or just require a damn header
		return "Not a GYM file";
	}
	
	return blargg_success;
}

blargg_err_t Gym_Emu::load_( const void* file, long data_offset, long file_size )
{
	require( pairs_per_frame );
	
	data = (const byte*) file + data_offset;
	data_end = (const byte*) file + file_size;
	
	loop_begin = NULL;
	loop_offset = 0;
	if ( data_offset )
	{
		const header_t& h = *(header_t*) file;
		loop_offset =
				h.loop [3] * 0x1000000L +
				h.loop [2] * 0x10000L +
				h.loop [1] * 0x100L +
				h.loop [0];
	}
	
	track_count_ = 1;
	voice_count_ = 8;
	mute_voices( 0 );
	
	return blargg_success;
}

blargg_err_t Gym_Emu::load( const void* file, long file_size )
{
	unload();
	
	if ( file_size < sizeof (header_t) )
		return "Not a GYM file";
	
	int data_offset = 0;
	BLARGG_RETURN_ERR( check_header( *(header_t*) file, &data_offset ) );
	
	return load_( file, data_offset, file_size );
}

blargg_err_t Gym_Emu::load( const header_t& h, Emu_Reader& in )
{
	unload();
	
	int data_offset = 0;
	BLARGG_RETURN_ERR( check_header( h, &data_offset ) );
	
	long file_size = sizeof h + in.remain();
	mem = new byte [file_size];
	if ( !mem )
		return "Out of memory";
	memcpy( mem, &h, sizeof h );
	BLARGG_RETURN_ERR( in.read( mem + sizeof h, file_size - sizeof h ) );
	
	return load_( mem, data_offset, file_size );
}

int Gym_Emu::track_length() const
{
	if ( loop_offset || loop_begin )
		return 0;
	
	long time = 0; // 1/60 sec frames
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
				++p;
			case 3:
				++p;
				break;
			
			default:
				dprintf( "Bad command: %02X\n", (int) p [-1] );
				break;
		}
	}
	
	return (time + 30 + 59) / 60;
}

blargg_err_t Gym_Emu::start_track( int )
{
	require( data );
	
	pos = &data [0];
	extra_pos = 0;
	loop_remain = loop_offset;
	
	prev_dac_count = 0;
	dac_enabled = false;
	last_dac = -1;
	
	fm.reset();
	apu.reset();
	blip_buf.clear( false );
	resampler.clear();
	
	track_ended_ = false;
	
	return blargg_success;
}

void Gym_Emu::play_frame( sample_t* out )
{
	parse_frame();
	
	// run SMS APU and buffer
	blip_time_t clock_count = (pairs_per_frame + 1 - blip_buf.samples_avail()) *
			clocks_per_sample;
	apu.end_frame( clock_count );
	blip_buf.end_frame( clock_count );
	assert( unsigned (blip_buf.samples_avail() - pairs_per_frame) <= 4 );
	
	// run fm
	const int sample_count = oversamples_per_frame - resampler.written();
	sample_t* buf = resampler.buffer();
	memset( buf, 0, sample_count * sizeof *buf );
	fm.run( buf, sample_count );
	resampler.write( sample_count );
	int count = resampler.read( sample_buf, pairs_per_frame * 2 );
	assert( count <= sample_buf_size );
	assert( unsigned (count - pairs_per_frame * 2) < 32 );
	
	// mix outputs
	mix_samples( out );
	blip_buf.remove_samples( pairs_per_frame );
}

blargg_err_t Gym_Emu::play( long count, sample_t* out )
{
	require( pos );
	
	const int samples_per_frame = pairs_per_frame * 2;
	
	// empty extra buffer
	if ( extra_pos ) {
		int n = samples_per_frame - extra_pos;
		if ( n > count )
			n = count;
		memcpy( out, sample_buf + extra_pos, n * sizeof *out );
		out += n;
		count -= n;
		extra_pos = (extra_pos + n) % samples_per_frame;
	}
	
	// entire frames
	while ( count >= samples_per_frame ) {
		play_frame( out );
		out += samples_per_frame;
		count -= samples_per_frame;
	}
	
	// extra
	if ( count ) {
		play_frame( sample_buf );
		extra_pos = count;
		memcpy( out, sample_buf, count * sizeof *out );
		out += count;
	}
	
	return blargg_success;
}

blargg_err_t Gym_Emu::skip( long count )
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
		BLARGG_RETURN_ERR( play( n, buf ) );
	}
	
	return blargg_success;
}

void Gym_Emu::run_dac( int dac_count )
{
	if ( !dac_disabled )
	{
		// Guess beginning and end of sample and adjust rate and buffer position accordingly.
		
		// count dac samples in next frame
		int next_dac_count = 0;
		const byte* p = this->pos;
		int cmd;
		while ( (cmd = *p++) != 0 ) {
			int data = *p++;
			if ( cmd <= 2 )
				++p;
			if ( cmd == 1 && data == 0x2A )
				next_dac_count++;
		}
		
		// adjust
		int rate_count = dac_count;
		int start = 0;
		if ( !prev_dac_count && next_dac_count && dac_count < next_dac_count ) {
			rate_count = next_dac_count;
			start = next_dac_count - dac_count;
		}
		else if ( prev_dac_count && !next_dac_count && dac_count < prev_dac_count ) {
			rate_count = prev_dac_count;
		}
		
		// Evenly space samples within buffer section being used
		Blip_Buffer::resampled_time_t period =
				blip_buf.resampled_duration( clock_rate / 60 ) / rate_count;
		
		Blip_Buffer::resampled_time_t time = blip_buf.resampled_time( 0 ) +
				period * start + (period >> 1);
		
		int last_dac = this->last_dac;
		if ( last_dac < 0 )
			last_dac = dac_buf [0];
		
		for ( int i = 0; i < dac_count; i++ )
		{
			int diff = dac_buf [i] - last_dac;
			last_dac += diff;
			dac_synth.offset_resampled( time, diff, &blip_buf );
			time += period;
		}
		this->last_dac = last_dac;
	}
	
	int const step = 6 * oversample;
	int remain = pairs_per_frame * oversample;
	while ( remain ) {
		int n = step;
		if ( n > remain )
			n = remain;
		remain -= n;
		fm.run_timer( n );
	}
}

void Gym_Emu::parse_frame()
{
	if ( track_ended_ )
		return;
	
	int dac_count = 0;
	
	const byte* pos = this->pos;
	if ( loop_remain && !--loop_remain )
		loop_begin = pos; // find loop on first time through sequence
	int cmd;
	while ( (cmd = *pos++) != 0 )
	{
		int data = *pos++;
		if ( cmd == 1 ) {
			int data2 = *pos++;
			if ( data == 0x2A ) {
				if ( dac_count < sizeof dac_buf ) {
					dac_buf [dac_count] = data2;
					dac_count += dac_enabled;
				}
			}
			else {
				if ( data == 0x2B )
					dac_enabled = (data2 & 0x80) != 0;
				
				fm.write( 0, data );
				fm.write( 1, data2 );
			}
		}
		else if ( cmd == 2 ) {
			fm.write( 2, data );
			fm.write( 3, *pos++ );
		}
		else if ( cmd == 3 ) {
			apu.write_data( 0, data );
		}
		else {
			dprintf( "Bad command: %02X\n", (int) cmd );
			--pos; // put data back
		}
	}
	// loop
	if ( pos >= data_end ) {
		if ( loop_begin )
			pos = loop_begin;
		else
			track_ended_ = true;
	}
	this->pos = pos;
	
	// dac
	if ( dac_count )
		run_dac( dac_count );
	prev_dac_count = dac_count;
}

#include BLARGG_ENABLE_OPTIMIZER

void Gym_Emu::mix_samples( sample_t* out )
{
	// Mix one frame of Blip_Buffer (SMS APU and PCM) and resampled YM audio
	Blip_Reader sn;
	int bass = sn.begin( blip_buf );
	const sample_t* ym = sample_buf;
	
	for ( int n = pairs_per_frame; n--; )
	{
		int s = sn.read();
		long l = ym [0] * 2 + s;
		sn.next( bass );
		if ( (BOOST::int16_t) l != l )
			l = 0x7FFF - (l >> 24);
		long r = ym [1] * 2 + s;
		ym += 2;
		out [0] = l;
		out [1] = r;
		out += 2;
		if ( (BOOST::int16_t) r != r )
			out [-1] = 0x7FFF - (r >> 24);
	}
	
	sn.end( blip_buf );
}

