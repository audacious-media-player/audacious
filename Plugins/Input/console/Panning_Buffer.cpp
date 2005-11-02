
// Game_Music_Emu 0.2.4. http://www.slack.net/~ant/libs/

#include "Panning_Buffer.h"

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

typedef long fixed_t;

#define TO_FIXED( f )   fixed_t ((f) * (1L << 15) + 0.5)
#define FMUL( x, y )    (((x) * (y)) >> 15)

Panning_Buffer::Panning_Buffer()
{
	bufs = NULL;
	buf_count = 0;
	bass_freq_ = -1;
	clock_rate_ = -1;
}

Panning_Buffer::~Panning_Buffer()
{
	delete [] bufs;
}
	
blargg_err_t Panning_Buffer::sample_rate( long rate, int msec )
{
	for ( int i = 0; i < buf_count; i++ )
		BLARGG_RETURN_ERR( bufs [i].sample_rate( rate, msec ) );
	sample_rate_ = rate;
	length_ = buf_count ? bufs [0].length() : msec;
	return blargg_success;
}

void Panning_Buffer::bass_freq( int freq )
{
	bass_freq_ = freq;
	for ( int i = 0; i < buf_count; i++ )
		bufs [i].bass_freq( freq );
}

void Panning_Buffer::clock_rate( long rate )
{
	clock_rate_ = rate;
	for ( int i = 0; i < buf_count; i++ )
		bufs [i].clock_rate( clock_rate_ );
}

void Panning_Buffer::clear()
{
	for ( int i = 0; i < buf_count; i++ )
		bufs [i].clear();
}

blargg_err_t Panning_Buffer::set_channel_count( int count )
{
	count += 2;
	if ( count != buf_count )
	{
		delete [] bufs;
		bufs = NULL;
		
		bufs = new buf_t [count];
		if ( !bufs )
			return "Out of memory";
		
		buf_count = count;
		
		if ( sample_rate_ )
			BLARGG_RETURN_ERR( sample_rate( sample_rate_, length_ ) );
		
		if ( clock_rate_ >= 0 )
			clock_rate( clock_rate_ );
		
		if ( bass_freq_ >= 0 )
			bass_freq( bass_freq_ );
		
		set_pan( left_chan, 1.0, 0.0 );
		set_pan( right_chan, 0.0, 1.0 );
		for ( int i = 0; i < buf_count - 2; i++ )
			set_pan( i, 1.0, 1.0 );
	}
	return blargg_success;
}

Panning_Buffer::channel_t Panning_Buffer::channel( int i )
{
	i += 2;
	require( i < buf_count );
	channel_t ch;
	ch.center = &bufs [i];
	ch.left   = &bufs [buf_count];
	ch.right  = &bufs [buf_count];
	return ch;
}


void Panning_Buffer::set_pan( int i, double left, double right )
{
	i += 2;
	require( i < buf_count );
	bufs [i].left_gain  = TO_FIXED( left  );
	bufs [i].right_gain = TO_FIXED( right );
}


void Panning_Buffer::end_frame( blip_time_t time, bool )
{
	for ( int i = 0; i < buf_count; i++ )
		bufs [i].end_frame( time );
}

long Panning_Buffer::read_samples( blip_sample_t* out, long count )
{
	require( count % 2 == 0 ); // count must be even
	
	long avail = bufs [0].samples_avail() * 2;
	if ( count > avail )
		count = avail;
	
	if ( count )
	{
		memset( out, 0, count * sizeof *out );
		
		int pair_count = count >> 1;
		
		int i;
		for ( i = 0; i < buf_count; i++ )
			add_panned( bufs [i], out, pair_count );
		
		for ( i = 0; i < buf_count; i++ )
			bufs [i].remove_samples( pair_count );
	}
	return count;
}

#include BLARGG_ENABLE_OPTIMIZER

void Panning_Buffer::add_panned( buf_t& buf, blip_sample_t* out, long count )
{
	Blip_Reader in; 
	
	fixed_t left_gain = buf.left_gain;
	fixed_t right_gain = buf.right_gain;
	int bass = in.begin( buf );
	
	while ( count-- )
	{
		long s = in.read();
		long l = out [0] + FMUL( s, left_gain );
		long r = out [1] + FMUL( s, right_gain );
		in.next();
		
		if ( (BOOST::int16_t) l != l )
			l = 0x7FFF - (l >> 24);
		
		out [0] = l;
		out [1] = r;
		out += 2;
		
		if ( (BOOST::int16_t) r != r )
			out [-1] = 0x7FFF - (r >> 24);
	}
	
	in.end( buf );
}

