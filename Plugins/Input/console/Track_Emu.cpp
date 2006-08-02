
// Game_Music_Emu 0.3.0. http://www.slack.net/~ant/

#include "Track_Emu.h"

#include <string.h>
#include <math.h>

/* Copyright (C) 2005-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include BLARGG_SOURCE_BEGIN

int const stereo = 2; // channels for a stereo signal
int const fade_block_size = 512;
int const fade_length = 8000; // msec
int const silence_max = 6; // seconds
int const silence_threshold = 0x10;

long Track_Emu::msec_to_samples( long msec ) const
{
	long rate = emu->sample_rate() * stereo;
	return (msec / 1000L) * rate + (msec % 1000L) * rate / 1000;
}

void Track_Emu::sync( long time )
{
	buf_count = 0;
	silence_count = 0;
	emu_time = time;
	out_time = time;
	silence_time = time;
	track_ended = time > fade_time + fade_length * stereo * emu->sample_rate();
}

void Track_Emu::restart_track()
{
	emu->start_track( track );
	sync ( 0 );
	
	// skip initial silence
	for ( int n = 40 * stereo * emu->sample_rate() / buf_size; n--; )
	{
		fill_buf( true );
		if ( buf_count || track_ended )
			break;
	}
	sync( 0 );
}

void Track_Emu::seek( long time )
{
	long pos = msec_to_samples( time ) & ~1;
	if ( pos < out_time )
		restart_track();
	emu->skip( pos - emu_time );
	sync( pos );
}

long Track_Emu::tell() const
{
	long rate = emu->sample_rate() * stereo;
	return (out_time / rate * 1000) + (out_time % rate * 1000 / rate);
}

void Track_Emu::start_track( Music_Emu* e, int t, long length, bool ds )
{
// to do: remove
//length = 50 * 1000;
//ds = true;
//t = 23;

	emu = e;
	track = t;
	detect_silence = ds;
	fade_factor = pow( 0.005, 1.0 / msec_to_samples( fade_length ) );
	fade_time = msec_to_samples( length );
	restart_track();
}

static bool is_silence( const Music_Emu::sample_t* p, int count )
{
	while ( count-- )
	{
		if ( (unsigned) (*p++ + silence_threshold / 2) > (unsigned) silence_threshold )
			return false;
	}
	return true;
}

void Track_Emu::fill_buf( bool check_silence )
{
	emu->play( buf_size, buf );
	emu_time += buf_size;
	if ( (check_silence || emu_time > fade_time) && is_silence( buf, buf_size ) )
	{
		silence_count += buf_size;
	}
	else
	{
		silence_time = emu_time;
		buf_count = buf_size;
	}
	if ( emu->track_ended() || emu->error_count() )
		track_ended = true;
}

inline void Track_Emu::end_track()
{
	silence_count = 0;
	buf_count = 0;
	track_ended = true;
}

bool Track_Emu::play( int out_count, Music_Emu::sample_t* out )
{
	assert( out_count % 2 == 0 );
	assert( emu );
	
	int pos = 0;
	while ( pos < out_count )
	{
		// fill with any remaining silence
		int count = min( silence_count, out_count - pos );
		if ( count )
		{
			silence_count -= count;
			memset( &out [pos], 0, count * sizeof *out );
		}
		else
		{
			// empty internal buffer
			count = min( buf_count, out_count - pos );
			if ( !count && track_ended )
			{
				memset( &out [pos], 0, (out_count - pos) * sizeof *out );
				return true;
			}
			
			memcpy( &out [pos], &buf [buf_size - buf_count], count * sizeof *out );
			buf_count -= count;
		}
		pos += count;
		
		// keep internal buffer full and possibly run ahead
		for ( int n = 6; n--; )
		{
			if ( buf_count || track_ended ||
					emu_time - out_time > silence_max * stereo * emu->sample_rate() )
				break;
			fill_buf( detect_silence );
		}
	}
	out_time += out_count;
	
	if ( detect_silence && 
		( emu_time - silence_time > silence_max * stereo * emu->sample_rate() && silence_time ) )
		end_track();
	
	// fade if track is ending
	if ( out_time > fade_time )
	{
		for ( int i = 0; i < out_count; i += fade_block_size )
		{
			double gain = pow( fade_factor, (double) (out_time + i - fade_time) );
			if ( gain < 0.005 )
				end_track();
			
			int count = min( fade_block_size, out_count - i );
			int igain = (unsigned int)((double)gain * (1 << 15));;
			for ( int j = 0; j < count; j++ )
				out [i + j] = (out [i + j] * igain) >> 15;
		}
	}
	
	return !silence_count && !buf_count && track_ended;
}

