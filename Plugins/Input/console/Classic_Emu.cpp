
// Game_Music_Emu 0.2.4. http://www.slack.net/~ant/libs/

#include "Classic_Emu.h"

#include "Multi_Buffer.h"

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

Classic_Emu::Classic_Emu()
{
	buf = NULL;
	std_buf = NULL;
	set_equalizer( equalizer_t( -8.87, 8800 ) );
}

Classic_Emu::~Classic_Emu()
{
	delete std_buf;
}

void Classic_Emu::update_eq_()
{
	update_eq( blip_eq_t( equalizer_.treble, equalizer_.cutoff, buf->sample_rate() ) );
	buf->bass_freq( equalizer_.bass );
}

void Classic_Emu::set_equalizer( const equalizer_t& eq )
{
	equalizer_ = eq;
	if ( buf )
		update_eq_();
}
	
blargg_err_t Classic_Emu::init( long sample_rate )
{
	buf = NULL;
	delete std_buf;
	std_buf = NULL;
	
	Stereo_Buffer* sb = new Stereo_Buffer;
	if ( !sb )
		return "Out of memory";
	std_buf = sb;
	
	BLARGG_RETURN_ERR( sb->sample_rate( sample_rate, 1000 / 20 ) );

	buf = std_buf;
	return blargg_success;
}

void Classic_Emu::mute_voices( int mask )
{
	require( buf ); // init() must have been called
	
	mute_mask_ = mask;
	for ( int i = voice_count(); i--; )
	{
		if ( mask & (1 << i) ) {
			set_voice( i, NULL );
		}
		else {
			Multi_Buffer::channel_t ch = buf->channel( i );
			set_voice( i, ch.center, ch.left, ch.right );
		}
	}
}

blargg_err_t Classic_Emu::setup_buffer( long clock_rate )
{
	require( buf ); // init() must have been called
	
	buf->clock_rate( clock_rate );
	update_eq_();
	return buf->set_channel_count( voice_count() );
}

void Classic_Emu::starting_track()
{
	require( buf ); // init() must have been called
	
	mute_voices( 0 );
	buf->clear();
}

blargg_err_t Classic_Emu::play( long count, sample_t* out )
{
	require( buf ); // init() must have been called
	
	long remain = count;
	while ( remain )
	{
		remain -= buf->read_samples( &out [count - remain], remain );
		if ( remain )
		{
			bool added_stereo = false;
			blip_time_t cyc = run( buf->length(), &added_stereo );
			if ( !cyc )
				return "Emulation error";
			buf->end_frame( cyc, added_stereo );
		}
	}
	return blargg_success;
}

