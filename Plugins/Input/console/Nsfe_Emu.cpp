
// Game_Music_Emu 0.3.0. http://www.slack.net/~ant/

#include "Nsfe_Emu.h"

#include "blargg_endian.h"
#include <string.h>

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

#define NSFE_TAG( a, b, c, d ) (d*0x1000000L + c*0x10000L + b*0x100L + a)

Nsfe_Info::Nsfe_Info()
{
	playlist_enabled = false;
}

Nsfe_Info::~Nsfe_Info() { }

void Nsfe_Info::enable_playlist( bool b )
{
	playlist_enabled = b;
	info_.track_count = (b && playlist_size()) ? playlist_size() : track_count_;
}

int Nsfe_Info::remap_track( int i ) const
{
	if ( !playlist_enabled || !playlist_size() )
		return i;
	
	return playlist_entry( i );
}

void Nsfe_Emu::start_track( int i )
{
	Nsf_Emu::start_track( remap_track( i ) );
}

const char* Nsfe_Info::track_name( int i ) const
{
	i = remap_track( i );
	if ( i < track_names.size() )
		return track_names [i];
	
	return "";
}

long Nsfe_Info::track_time( int i ) const
{
	i = remap_track( i );
	if ( i < track_times.size() )
		return track_times [i];
	
	return 0;
}

// Read little-endian 32-bit int
static blargg_err_t read_le32( Emu_Reader& in, long* out )
{
	unsigned char buf [4];
	BLARGG_RETURN_ERR( in.read( buf, sizeof buf ) );
	*out = get_le32( buf );
	return blargg_success;
}

// Read multiple strings and separate into individual strings
static blargg_err_t read_strs( Emu_Reader& in, long size, std::vector<char>& chars,
		std::vector<const char*>& strs )
{
	chars.resize( size + 1 );
	chars [size] = 0; // in case last string doesn't have terminator
	BLARGG_RETURN_ERR( in.read( &chars [0], size ) );
	
	for ( int i = 0; i < size; i++ )
	{
		strs.push_back( &chars [i] );
		while ( i < size && chars [i] )
			i++;
	}
	
	return blargg_success;
}

// Copy in to out, where out has out_max characters allocated. Truncate to
// out_max - 1 characters.
static void copy_str( const char* in, char* out, int out_max )
{
	out [out_max - 1] = 0;
	strncpy( out, in, out_max - 1 );
}

struct nsfe_info_t {
	unsigned char load_addr [2];
	unsigned char init_addr [2];
	unsigned char play_addr [2];
	unsigned char speed_flags;
	unsigned char chip_flags;
	unsigned char track_count;
	unsigned char first_track;
};
BOOST_STATIC_ASSERT( sizeof (nsfe_info_t) == 10 );

blargg_err_t Nsfe_Info::load( const header_t& nsfe_tag, Emu_Reader& in, Nsf_Emu* nsf_emu )
{
	// check header
	if ( memcmp( nsfe_tag.tag, "NSFE", 4 ) )
		return "Not an NSFE file";
	
	// free previous info
	track_name_data.clear();
	track_names.clear();
	playlist.clear();
	track_times.clear();
	
	// default nsf header
	static const Nsf_Emu::header_t base_header =
	{
		{'N','E','S','M','\x1A'},// tag
		1,                  // version
		1, 1,               // track count, first track
		{0,0},{0,0},{0,0},  // addresses
		"","","",           // strings
		{0x1A, 0x41},       // NTSC rate
		{0,0,0,0,0,0,0,0},  // banks
		{0x20, 0x4E},       // PAL rate
		0, 0,               // flags
		{0,0,0,0}           // unused
	};
	Nsf_Emu::header_t& header = info_;
	header = base_header;
	
	// parse tags
	int phase = 0;
	while ( phase != 3 )
	{
		// read size and tag
		long size = 0;
		long tag = 0;
		BLARGG_RETURN_ERR( read_le32( in, &size ) );
		BLARGG_RETURN_ERR( read_le32( in, &tag ) );
		
		switch ( tag )
		{
			case NSFE_TAG('I','N','F','O'): {
				check( phase == 0 );
				if ( size < 8 )
					return "Bad NSFE file";
				
				nsfe_info_t info;
				info.track_count = 1;
				info.first_track = 0;
				
				int s = size;
				if ( s > (int) sizeof info )
					s = sizeof info;
				BLARGG_RETURN_ERR( in.read( &info, s ) );
				BLARGG_RETURN_ERR( in.skip( size - s ) );
				phase = 1;
				info_.speed_flags = info.speed_flags;
				info_.chip_flags = info.chip_flags;
				info_.track_count = info.track_count;
				this->track_count_ = info.track_count;
				info_.first_track = info.first_track;
				std::memcpy( info_.load_addr, info.load_addr, 2 * 3 );
				break;
			}
			
			case NSFE_TAG('B','A','N','K'):
				if ( size > (int) sizeof info_.banks )
					return "Bad NSFE file";
				BLARGG_RETURN_ERR( in.read( info_.banks, size ) );
				break;
			
			case NSFE_TAG('a','u','t','h'): {
				std::vector<char> chars;
				std::vector<const char*> strs;
				BLARGG_RETURN_ERR( read_strs( in, size, chars, strs ) );
				int n = strs.size();
				
				if ( n > 3 )
					copy_str( strs [3], info_.ripper, sizeof info_.ripper );
				
				if ( n > 2 )
					copy_str( strs [2], info_.copyright, sizeof info_.copyright );
				
				if ( n > 1 )
					copy_str( strs [1], info_.author, sizeof info_.author );
				
				if ( n > 0 )
					copy_str( strs [0], info_.game, sizeof info_.game );
				
				break;
			}
			
			case NSFE_TAG('t','i','m','e'): {
				track_times.resize( size / 4 );
				for ( int i = 0; i < track_times.size(); i++ )
					BLARGG_RETURN_ERR( read_le32( in, &track_times [i] ) );
				break;
			}
			
			case NSFE_TAG('t','l','b','l'):
				BLARGG_RETURN_ERR( read_strs( in, size, track_name_data, track_names ) );
				break;
			
			case NSFE_TAG('p','l','s','t'):
				playlist.resize( size );
				BLARGG_RETURN_ERR( in.read( &playlist [0], size ) );
				break;
			
			case NSFE_TAG('D','A','T','A'): {
				check( phase == 1 );
				phase = 2;
				if ( !nsf_emu )
				{
					in.skip( size );
				}
				else
				{
					Subset_Reader sub( &in, size ); // limit emu to nsf data
					BLARGG_RETURN_ERR( nsf_emu->load( info_, sub ) );
					check( sub.remain() == 0 );
				}
				break;
			}
			
			case NSFE_TAG('N','E','N','D'):
				check( phase == 2 );
				phase = 3;
				break;
			
			default:
				// tags that can be skipped start with a lowercase character
				check( std::islower( (tag >> 24) & 0xff ) );
				BLARGG_RETURN_ERR( in.skip( size ) );
				break;
		}
	}
	
	enable_playlist( playlist_enabled );
	
	return blargg_success;
}

blargg_err_t Nsfe_Info::load( Emu_Reader& in, Nsf_Emu* nsf_emu )
{
	header_t h;
	BLARGG_RETURN_ERR( in.read( &h, sizeof h ) );
	return load( h, in, nsf_emu );
}

blargg_err_t Nsfe_Info::load_file( const char* path, Nsf_Emu* emu )
{
	Std_File_Reader in;
	BLARGG_RETURN_ERR( in.open( path ) );
	return load( in, emu );
}

