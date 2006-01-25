
// Game_Music_Emu 0.3.0. http://www.slack.net/~ant/

#include "Music_Emu.h"

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

Music_Emu::equalizer_t const Music_Emu::tv_eq = { -8.0, 180 };

Music_Emu::Music_Emu()
{
	equalizer_.treble = -1.0;
	equalizer_.bass = 60;
	sample_rate_ = 0;
	voice_count_ = 0;
	mute_mask_ = 0;
	track_count_ = 0;
	error_count_ = 0;
	track_ended_ = false;
}

Music_Emu::~Music_Emu()
{
}

blargg_err_t Music_Emu::load_file( const char* path )
{
	Std_File_Reader in;
	BLARGG_RETURN_ERR( in.open( path ) );
	return load( in );
}

void Music_Emu::skip( long count )
{
	const int buf_size = 1024;
	sample_t buf [buf_size];
	
	const long threshold = 30000;
	if ( count > threshold )
	{
		int saved_mute = mute_mask_;
		mute_voices( ~0 );
		
		while ( count > threshold / 2 )
		{
			play( buf_size, buf );
			count -= buf_size;
		}
		
		mute_voices( saved_mute );
	}
	
	while ( count )
	{
		int n = buf_size;
		if ( n > count )
			n = count;
		count -= n;
		play( n, buf );
	}
}

const char** Music_Emu::voice_names() const
{
	static const char* names [] = {
		"Voice 1", "Voice 2", "Voice 3", "Voice 4",
		"Voice 5", "Voice 6", "Voice 7", "Voice 8"
	};
	return names;
}

