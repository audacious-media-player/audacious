
// Game_Music_Emu 0.3.0. http://www.slack.net/~ant/

#include "Dual_Resampler.h"

#include <stdlib.h>
#include <string.h>

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

int const resampler_extra = 256;

Dual_Resampler::Dual_Resampler()
{
}

Dual_Resampler::~Dual_Resampler()
{
}

blargg_err_t Dual_Resampler::resize( int pairs )
{
	BLARGG_RETURN_ERR( sample_buf.resize( pairs * 2 ) );
	buf_pos = sample_buf.size();
	oversamples_per_frame = int (pairs * resampler.ratio()) * 2 + 2;
	return resampler.buffer_size( oversamples_per_frame + resampler_extra );
}

void Dual_Resampler::play_frame_( Blip_Buffer& blip_buf, sample_t* out )
{
	long pair_count = sample_buf.size() >> 1;
	blip_time_t blip_time = blip_buf.count_clocks( pair_count );
	int sample_count = oversamples_per_frame - resampler.written();
	
	int new_count = play_frame( blip_time, sample_count, resampler.buffer() );
	assert( unsigned (new_count - sample_count) < resampler_extra );
	
	blip_buf.end_frame( blip_time );
	assert( blip_buf.samples_avail() == pair_count );
	
	resampler.write( new_count );
	
	long count = resampler.read( sample_buf.begin(), sample_buf.size() );
	assert( count == (long) sample_buf.size() );
	
	mix_samples( blip_buf, out );
	blip_buf.remove_samples( pair_count );
}

void Dual_Resampler::play( long count, sample_t* out, Blip_Buffer& blip_buf )
{
	// empty extra buffer
	long remain = sample_buf.size() - buf_pos;
	if ( remain )
	{
		if ( remain > count )
			remain = count;
		count -= remain;
		memcpy( out, &sample_buf [buf_pos], remain * sizeof *out );
		out += remain;
		buf_pos += remain;
	}
	
	// entire frames
	while ( count >= (long) sample_buf.size() )
	{
		play_frame_( blip_buf, out );
		out += sample_buf.size();
		count -= sample_buf.size();
	}
	
	// extra
	if ( count )
	{
		play_frame_( blip_buf, sample_buf.begin() );
		buf_pos = count;
		memcpy( out, sample_buf.begin(), count * sizeof *out );
		out += count;
	}
}

#include BLARGG_ENABLE_OPTIMIZER

void Dual_Resampler::mix_samples( Blip_Buffer& blip_buf, sample_t* out )
{
	Blip_Reader sn;
	int bass = sn.begin( blip_buf );
	const sample_t* in = sample_buf.begin();
	
	for ( int n = sample_buf.size() >> 1; n--; )
	{
		int s = sn.read();
		long l = (long) in [0] * 2 + s;
		sn.next( bass );
		long r = in [1];
		if ( (BOOST::int16_t) l != l )
			l = 0x7FFF - (l >> 24);
		r = r * 2 + s;
		in += 2;
		out [0] = l;
		out [1] = r;
		out += 2;
		if ( (BOOST::int16_t) r != r )
			out [-1] = 0x7FFF - (r >> 24);
	}
	
	sn.end( blip_buf );
}

