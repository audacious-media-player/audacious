
// Game_Music_Emu 0.3.0. http://www.slack.net/~ant/

#include "Classic_Emu.h"

#include "Multi_Buffer.h"

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

Classic_Emu::Classic_Emu()
{
	buf = NULL;
	stereo_buffer = NULL;
}

Classic_Emu::~Classic_Emu()
{
	delete stereo_buffer;
}

void Classic_Emu::set_equalizer( equalizer_t const& eq )
{
	Music_Emu::set_equalizer( eq );
	update_eq( eq.treble );
	if ( buf )
		buf->bass_freq( equalizer().bass );
}
	
blargg_err_t Classic_Emu::set_sample_rate( long sample_rate )
{
	if ( !buf )
	{
		if ( !stereo_buffer )
			BLARGG_CHECK_ALLOC( stereo_buffer = BLARGG_NEW Stereo_Buffer );
		buf = stereo_buffer;
	}
	
	BLARGG_RETURN_ERR( buf->set_sample_rate( sample_rate, 1000 / 20 ) );
	return Music_Emu::set_sample_rate( sample_rate );
}

void Classic_Emu::mute_voices( int mask )
{
	require( buf ); // set_sample_rate() must have been called
	
	Music_Emu::mute_voices( mask );
	for ( int i = voice_count(); i--; )
	{
		if ( mask & (1 << i) )
		{
			set_voice( i, NULL, NULL, NULL );
		}
		else
		{
			Multi_Buffer::channel_t ch = buf->channel( i );
			set_voice( i, ch.center, ch.left, ch.right );
		}
	}
}

blargg_err_t Classic_Emu::setup_buffer( long new_clock_rate )
{
	require( sample_rate() ); // fails if set_sample_rate() hasn't been called yet
	
	clock_rate = new_clock_rate;
	buf->clock_rate( clock_rate );
	BLARGG_RETURN_ERR( buf->set_channel_count( voice_count() ) );
	set_equalizer( equalizer() );
	remute_voices();
	return blargg_success;
}

void Classic_Emu::start_track( int track )
{
	Music_Emu::start_track( track );
	buf->clear();
}

blip_time_t Classic_Emu::run_clocks( blip_time_t t, bool* )
{
	assert( false );
	return t;
}

blip_time_t Classic_Emu::run( int msec, bool* added_stereo )
{
	return run_clocks( (long) msec * clock_rate / 1000, added_stereo );
}

void Classic_Emu::play( long count, sample_t* out )
{
	require( sample_rate() ); // fails if set_sample_rate() hasn't been called yet
	
	long remain = count;
	while ( remain )
	{
		remain -= buf->read_samples( &out [count - remain], remain );
		if ( remain )
		{
			bool added_stereo = false;
			blip_time_t clocks_emulated = run( buf->length(), &added_stereo );
			buf->end_frame( clocks_emulated, added_stereo );
		}
	}
}

