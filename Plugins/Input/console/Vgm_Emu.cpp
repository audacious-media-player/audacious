
// Game_Music_Emu 0.2.4. http://www.slack.net/~ant/libs/

#include "Vgm_Emu.h"

#include <string.h>
#include <math.h>

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

const long vgm_sample_rate = 44100;

Vgm_Emu::Vgm_Emu( double gain )
{
	data = NULL;
	pos = NULL;
	apu.volume( gain );
	
	// to do: decide on equalization parameters
	set_equalizer( equalizer_t( -32, 8000, 66 ) );
}

Vgm_Emu::~Vgm_Emu()
{
	unload();
}

void Vgm_Emu::unload()
{
	delete [] data;
	data = NULL;
	pos = NULL;
	track_ended_ = false;
}

const char** Vgm_Emu::voice_names() const
{
	static const char* names [] = { "Square 1", "Square 2", "Square 3", "Noise" };
	return names;
}

void Vgm_Emu::set_voice( int i, Blip_Buffer* c, Blip_Buffer* l, Blip_Buffer* r )
{
	apu.osc_output( i, c, l, r );
}

void Vgm_Emu::update_eq( blip_eq_t const& eq )
{
	apu.treble_eq( eq );
}

const int time_bits = 12;

inline sms_time_t Vgm_Emu::clocks_from_samples( int samples ) const
{
	const long round_up = 1L << (time_bits - 1);
	return (samples * time_factor + round_up) >> time_bits;
}

static long get_le32( const BOOST::uint8_t b [4] )
{
	return b [3] * 0x1000000L + b [2] * 0x10000L + b [1] * 0x100L + b [0];
}

blargg_err_t Vgm_Emu::load( const header_t& h, Emu_Reader& in )
{
	unload();
	
	// compatibility
	if ( 0 != memcmp( h.tag, "Vgm ", 4 ) )
		return "Not a VGM file";
	if ( get_le32( h.vers ) > 0x0101 )
		return "Unsupported VGM format";
	
	// clock rate
	long clock_rate = get_le32( h.psg_rate );
	if ( !clock_rate )
		return "Only PSG sound chip is supported";
	time_factor = (long) floor( clock_rate * ((1L << time_bits) /
			(double) vgm_sample_rate) + 0.5 );
	
	// data
	long data_size = in.remain();
	data = new byte [data_size + 3]; // allow pointer to go past end
	if ( !data )
		return "Out of memory";
	end = data + data_size;
	data [data_size] = 0;
	data [data_size + 1] = 0;
	data [data_size + 2] = 0;
	blargg_err_t err = in.read( data, data_size );
	if ( err ) {
		unload();
		return err;
	}
	long loop_offset = get_le32( h.loop_offset );
	loop_begin = end;
	loop_duration = 0;
	if ( loop_offset )
	{
		loop_duration = get_le32( h.loop_duration );
		if ( loop_duration )
			loop_begin = &data [loop_offset + 0x1c - sizeof (header_t)];
	}
	
	voice_count_ = Sms_Apu::osc_count;
	track_count_ = 1;
	
	return setup_buffer( clock_rate );
}

int Vgm_Emu::track_length( const byte** end_out, int* remain_out ) const
{
	require( data ); // file must have been loaded
	
	long time = 0;
	
	if ( end_out || !loop_duration )
	{
		const byte* p = data;
		while ( p < end )
		{
			int cmd = *p++;
			switch ( cmd )
			{
				case 0x4f:
				case 0x50:
					p += 1;
					break;
				
				case 0x61:
					if ( p + 1 < end ) {
						time += p [1] * 0x100L + p [0];
						p += 2;
					}
					break;
				
				case 0x62:
					time += 735; // ntsc frame
					break;
				
				case 0x63:
					time += 882; // pal frame
					break;
				
				default:
					if ( (p [-1] & 0xf0) == 0x50 ) {
						p += 2;
						break;
					}
					dprintf( "Bad command in VGM stream: %02X\n", (int) cmd );
					break;
				
				case 0x66:
					if ( end_out )
						*end_out = p;
					if ( remain_out )
						*remain_out = end - p;
					p = end;
					break;
			}
		}
	}
	
	// i.e. ceil( exact_length + 0.5 )
	return loop_duration ? 0 :
			(time + (vgm_sample_rate >> 1) + vgm_sample_rate - 1) / vgm_sample_rate;
}

blargg_err_t Vgm_Emu::start_track( int )
{
	require( data ); // file must have been loaded
	
	pos = data;
	loop_remain = 0;
	delay = 0;
	track_ended_ = false;
	apu.reset();
	starting_track();
	return blargg_success;
}

blip_time_t Vgm_Emu::run( int msec, bool* added_stereo )
{
	require( pos ); // track must have been started
	
	const int duration = vgm_sample_rate / 100 * msec / 10;
	int time = delay;
	while ( time < duration && pos < end )
	{
		if ( !loop_remain && pos >= loop_begin )
			loop_remain = loop_duration;
		
		int cmd = *pos++;
		int delay = 0;
		switch ( cmd )
		{
			case 0x66:
				pos = end; // end
				if ( loop_duration ) {
					pos = loop_begin;
					loop_remain = loop_duration;
				} 
				break;
			
			case 0x62:
				delay = 735; // ntsc frame
				break;
			
			case 0x63:
				delay = 882; // pal frame
				break;
			
			case 0x4f:
				if ( pos == end ) {
					check( false ); // missing data
					break;
				}
				apu.write_ggstereo( clocks_from_samples( time ), *pos++ );
				break;
			
			case 0x50:
				if ( pos == end ) {
					check( false ); // missing data
					break;
				}
				apu.write_data( clocks_from_samples( time ), *pos++ );
				break;
			
			case 0x61:
				if ( end - pos < 1 ) {
					check( false ); // missing data
					break;
				}
				delay = pos [1] * 0x100L + pos [0];
				pos += 2;
				break;
			
			default:
				if ( (cmd & 0xf0) == 0x50 )
				{
					if ( end - pos < 1 ) {
						check( false ); // missing data
						break;
					}
					pos += 2;
					break;
				}
				dprintf( "Bad command in VGM stream: %02X\n", (int) cmd );
				break;
			
		}
		time += delay;
		if ( loop_remain && (loop_remain -= delay) <= 0 )
		{
			pos = loop_begin;
			loop_remain = 0;
		}
	}
	
	blip_time_t end_time = clocks_from_samples( duration );
	if ( pos < end )
	{
		delay = time - duration;
		if ( apu.end_frame( end_time ) && added_stereo )
			*added_stereo = true;
	}
	else {
		delay = 0;
		track_ended_ = true;
	}
	
	return end_time;
}

Vgm_Reader::Vgm_Reader() : file( NULL ) {
}

Vgm_Reader::~Vgm_Reader() {
	close();
}

blargg_err_t Vgm_Reader::read_head(Vgm_Emu::header_t *header) {
	vfs_fread(&header->tag,         1, 4,file);
	vfs_fread(&header->data_size,   1, 4,file);
	vfs_fread(&header->vers,        1, 4,file);
	vfs_fread(&header->psg_rate,    1, 4,file);
	vfs_fread(&header->fm_rate,     1, 4,file);
	vfs_fread(&header->g3d_offset,  1, 4,file);
	vfs_fread(&header->sample_count,1, 4,file);
	vfs_fread(&header->loop_offset, 1, 4,file);
	vfs_fread(&header->loop_duration,1,4,file);
	vfs_fread(&header->frame_rate,  1, 4,file);
	vfs_fread(&header->unused,      1,0x18,file);
}
