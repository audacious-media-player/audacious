
// Game_Music_Emu 0.2.4. http://www.slack.net/~ant/libs/

#include "Spc_Emu.h"

#include <string.h>
#include "abstract_file.h"

/* Copyright (C) 2004-2005 Shay Green. This module is free software; you
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

Spc_Emu::Spc_Emu()
{
	resample_ratio = 1.0;
	use_resampler = false;
	track_count_ = 0;
}

Spc_Emu::~Spc_Emu()
{
}

const char** Spc_Emu::voice_names() const
{
	static const char* names [] = {
		"DSP 1", "DSP 2", "DSP 3", "DSP 4", "DSP 5", "DSP 6", "DSP 7", "DSP 8"
	};
	return names;
}

blargg_err_t Spc_Emu::init( long sample_rate, double gain )
{
	apu.set_gain( gain );
	use_resampler = false;
	resample_ratio = (double) native_sample_rate / sample_rate;
	if ( sample_rate != native_sample_rate )
	{
		BLARGG_RETURN_ERR( resampler.buffer_size( native_sample_rate / 20 * 2 ) );
		resampler.time_ratio( resample_ratio, 0.9965 );
		use_resampler = true;
	}
	
	return blargg_success;
}

blargg_err_t Spc_Emu::load( const header_t& h, Emu_Reader& in )
{
	if ( in.remain() < sizeof file.data )
		return "Not an SPC file";
	
	if ( strncmp( h.tag, "SNES-SPC700 Sound File Data", 27 ) != 0 )
		return "Not an SPC file";
	
	track_count_ = 1;
	voice_count_ = Snes_Spc::voice_count;
	
	memcpy( &file.header, &h, sizeof file.header );
	return in.read( file.data, sizeof file.data );
}

blargg_err_t Spc_Emu::start_track( int )
{
	resampler.clear();
	return apu.load_spc( &file, sizeof file );
}

blargg_err_t Spc_Emu::skip( long count )
{
	count = long (count * resample_ratio) & ~1;
	
	count -= resampler.skip_input( count );
	if ( count > 0 )
		BLARGG_RETURN_ERR( apu.skip( count ) );
	
	// eliminate pop due to resampler
	const int resampler_latency = 64;
	sample_t buf [resampler_latency];
	return play( resampler_latency, buf );
}

blargg_err_t Spc_Emu::play( long count, sample_t* out )
{
	require( track_count_ ); // file must be loaded
	
	if ( !use_resampler )
		return apu.play( count, out );
	
	long remain = count;
	while ( remain > 0 )
	{
		remain -= resampler.read( &out [count - remain], remain );
		if ( remain > 0 )
		{
			long n = resampler.max_write();
			BLARGG_RETURN_ERR( apu.play( n, resampler.buffer() ) );
			resampler.write( n );
		}
	}
	
	assert( remain == 0 );
	
	return blargg_success;
}

Spc_Reader::Spc_Reader() : file( NULL ) {
}

Spc_Reader::~Spc_Reader() {
	close();
}

blargg_err_t Spc_Reader::read_head(Spc_Emu::header_t *header) {
	vfs_fread(&header->tag,     1,35,file);
	vfs_fread(&header->format,  1, 1,file);
	vfs_fread(&header->version, 1, 1,file);
	vfs_fread(&header->pc,      1, 2,file);
	vfs_fread(&header->a,       1, 1,file);
	vfs_fread(&header->x,       1, 1,file);
	vfs_fread(&header->y,       1, 1,file);
	vfs_fread(&header->psw,     1, 1,file);
	vfs_fread(&header->sp,      1, 1,file);
	vfs_fread(&header->unused,  1, 2,file);
	vfs_fread(&header->song,    1,32,file);
	vfs_fread(&header->game,    1,32,file);
	vfs_fread(&header->dumper,  1,16,file);
	vfs_fread(&header->comment, 1,32,file);
	vfs_fread(&header->date,    1,11,file);
	vfs_fread(&header->len_secs,1, 3,file);
	vfs_fread(&header->fade_msec,1,5,file);
	vfs_fread(&header->author,  1,32,file);
	vfs_fread(&header->mute_mask,1,1,file);
	vfs_fread(&header->emulator,1, 1,file);
	vfs_fread(&header->unused2, 1,45,file);
}
