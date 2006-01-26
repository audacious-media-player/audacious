/*
 * Audacious: Cross platform multimedia player
 * Copyright (c) 2005  Audacious Team
 *
 * Driver for Game_Music_Emu library. See details at:
 * http://www.slack.net/~ant/libs/
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "libaudacious/configdb.h"
#include "libaudacious/util.h"
#include "libaudacious/titlestring.h"
extern "C" {
#include "audacious/output.h"
}
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

// Game_Music_Emu
#include "Nsf_Emu.h"
#include "Nsfe_Emu.h"
#include "Gbs_Emu.h"
#include "Vgm_Emu.h"
#include "Gym_Emu.h"
#include "Spc_Emu.h"

#include "Track_Emu.h"
#include "Vfs_File.h"
#include "Gzip_File.h"
#include "blargg_endian.h"

//typedef Vfs_File_Reader Audacious_Reader; // will use VFS once it handles gzip transparently
typedef Gzip_File_Reader Audacious_Reader;

struct AudaciousConsoleConfig {
	gint loop_length;   // length of tracks that lack timing information
	gboolean resample;  // whether or not to resample
	gint resample_rate; // rate to resample at
	gboolean nsfe_playlist; // if true, use optional NSFE playlist
};
static AudaciousConsoleConfig audcfg = { 180, FALSE, 32000, TRUE };
static GThread* decode_thread;
static GStaticMutex playback_mutex = G_STATIC_MUTEX_INIT;
static int console_ip_is_going;
static volatile long pending_seek;
extern InputPlugin console_ip;
static Music_Emu* emu = 0;
static Track_Emu track_emu;

static void unload_file()
{
	delete emu;
	emu = NULL;
}

// Information

typedef unsigned char byte;

#define DUPE_FIELD( field ) g_strndup( field, sizeof (field) );

struct track_info_t
{
	int track;  // track to get info for
	int length; // in msec, -1 = unknown
	int loop;   // in msec, -1 = unknown, 0 = not looped
	int intro;  // in msec, -1 = unknown
	
	TitleInput* ti;
};

// NSFE

void get_nsfe_info( Nsfe_Info const& nsfe, track_info_t* out )
{
	Nsfe_Info::info_t const& h = nsfe.info();
	out->ti->performer  = DUPE_FIELD( h.author );
	out->ti->album_name = DUPE_FIELD( h.game );
	out->ti->comment    = DUPE_FIELD( h.copyright );
	out->ti->track_name = g_strdup( nsfe.track_name( out->track ) );
	int time = nsfe.track_time( out->track );
	if ( time > 0 )
		out->length = time;
	if ( nsfe.info().track_count > 1 )
		out->ti->track_number = out->track + 1;
}

inline void get_info_emu( Nsfe_Emu& emu, track_info_t* out )
{
	emu.enable_playlist( audcfg.nsfe_playlist ); // to do: kind of hacky
	get_nsfe_info( emu, out );
}

inline void get_file_info( Nsfe_Emu::header_t const& h, Data_Reader& in, track_info_t* out )
{
	Nsfe_Info nsfe;
	if ( !nsfe.load( h, in ) )
	{
		nsfe.enable_playlist( audcfg.nsfe_playlist );
		get_nsfe_info( nsfe, out );
	}
}

// NSF

static void get_nsf_info_( Nsf_Emu::header_t const& h, track_info_t* out )
{
	out->ti->performer  = DUPE_FIELD( h.author );
	out->ti->album_name = DUPE_FIELD( h.game );
	out->ti->comment    = DUPE_FIELD( h.copyright );
	if ( h.track_count > 1 )
		out->ti->track_number = out->track + 1;
}

inline void get_info_emu( Nsf_Emu& emu, track_info_t* out )
{
	get_nsf_info_( emu.header(), out );
}

inline void get_file_info( Nsf_Emu::header_t const& h, Data_Reader& in, track_info_t* out )
{
	get_nsf_info_( h, out );
}

// GBS

static void get_gbs_info_( Gbs_Emu::header_t const& h, track_info_t* out )
{
	out->ti->performer  = DUPE_FIELD( h.author );
	out->ti->album_name = DUPE_FIELD( h.game );
	out->ti->comment    = DUPE_FIELD( h.copyright );
	if ( h.track_count > 1 )
		out->ti->track_number = out->track + 1;
}

inline void get_info_emu( Gbs_Emu& emu, track_info_t* out )
{
	get_gbs_info_( emu.header(), out );
}

inline void get_file_info( Gbs_Emu::header_t const& h, Data_Reader& in, track_info_t* out )
{
	get_gbs_info_( h, out );
}

// GYM

static void get_gym_info_( Gym_Emu::header_t const& h, track_info_t* out )
{
	if ( !memcmp( h.tag, "GYMX", 4 ) )
	{
		out->ti->performer  = DUPE_FIELD( h.copyright );
		out->ti->album_name = DUPE_FIELD( h.game );
		out->ti->track_name = DUPE_FIELD( h.song );
		out->ti->comment    = DUPE_FIELD( h.comment );
	}
}

static void get_gym_timing_( Gym_Emu const& emu, track_info_t* out )
{
	out->length = emu.track_length() * 50 / 3; // 1000 / 60
	out->loop = 0;
	
	long loop = get_le32( emu.header().loop_start );
	if ( loop )
	{
		out->intro = loop * 50 / 3;
		out->loop = out->length - out->intro;
		out->length = -1;
	}
}

inline void get_info_emu( Gym_Emu& emu, track_info_t* out )
{
	get_gym_info_( emu.header(), out );
	get_gym_timing_( emu, out );
}

inline void get_file_info( Gym_Emu::header_t const& h, Data_Reader& in, track_info_t* out )
{
	get_gym_info_( h, out );
	
	// have to load and parse entire GYM file to determine length
	// to do: could make more efficient by manually parsing data (format is simple)
	// rather than loading into emulator with its FM chips and resampler
	Gym_Emu* emu = new Gym_Emu;
	if ( emu && !emu->set_sample_rate( 44100 ) && !emu->load( h, in ) )
		get_gym_timing_( *emu, out );
	delete emu;
}

// SPC

static void get_spc_xid6( byte const* begin, long size, track_info_t* out )
{
	// header
	byte const* end = begin + size;
	if ( size < 8 || memcmp( begin, "xid6", 4 ) )
		return;
	long info_size = get_le32( begin + 4 );
	byte const* in = begin + 8; 
	if ( end - in > info_size )
		end = in + info_size;
	
	while ( end - in >= 4 )
	{
		// header
		int id   = in [0];
		int data = in [3] * 0x100 + in [2];
		int type = in [1];
		int len  = type ? data : 0;
		in += 4;
		if ( len > end - in )
			break; // block goes past end of data
		
		// handle specific block types
		switch ( id )
		{
			case 0x01: out->ti->track_name = g_strndup( (char*) in, len ); break;
			case 0x02: out->ti->album_name = g_strndup( (char*) in, len ); break;
			case 0x03: out->ti->performer  = g_strndup( (char*) in, len ); break;
			case 0x07: out->ti->comment    = g_strndup( (char*) in, len ); break;
			//case 0x31: // loop length, but I haven't found any SPC files that use this
		}
		
		// skip to next block
		in += len;
		
		// blocks are supposed to be 4-byte aligned with zero-padding...
		byte const* unaligned = in;
		while ( (in - begin) & 3 && in < end )
		{
			if ( *in++ != 0 )
			{
				// ...but some files have no padding
				in = unaligned;
				break;
			}
		}
	}
}

static void get_spc_info_( Spc_Emu::header_t const& h, byte const* xid6, long xid6_size,
		track_info_t* out )
{
	// decode length (can be in text or binary format)
	char s [4] = { h.len_secs [0], h.len_secs [1], h.len_secs [2], 0 };
	int len_secs = (unsigned char) s [1] * 0x100 + s [0];
	if ( s [1] >= ' ' || (!s [1] && isdigit( s [0] )) )
		len_secs = atoi( s );
	if ( len_secs )
		out->length = len_secs * 1000;
	
	if ( xid6_size )
		get_spc_xid6( xid6, xid6_size, out );
	
	// use header to fill any remaining fields
	if ( !out->ti->performer  ) out->ti->performer  = DUPE_FIELD( h.author );
	if ( !out->ti->album_name ) out->ti->album_name = DUPE_FIELD( h.game );
	if ( !out->ti->track_name ) out->ti->track_name = DUPE_FIELD( h.song );
}

inline void get_info_emu( Spc_Emu& emu, track_info_t* out )
{
	get_spc_info_( emu.header(), emu.trailer(), emu.trailer_size(), out );
}

inline void get_file_info( Spc_Emu::header_t const& h, Data_Reader& in, track_info_t* out )
{
	// handle xid6 data at end of file
	long const xid6_skip = 0x10200 - sizeof (Spc_Emu::header_t);
	long xid6_size = in.remain() - xid6_skip;
	blargg_vector<byte> xid6;
	if ( xid6_size <= 0 || xid6.resize( xid6_size ) || in.skip( xid6_skip ) ||
			in.read( xid6.begin(), xid6.size() ) )
		xid6_size = 0;
	
	get_spc_info_( h, xid6.begin(), xid6_size, out );
}

// VGM

static void get_gd3_str( byte const* in, byte const* end, gchar** out )
{
	int len = (end - in) / 2 - 1;
	if ( len > 0 )
	{
		*out = g_strndup( "", len );
		if ( !*out )
			return;
		for ( int i = 0; i < len; i++ )
			(*out) [i] = in [i * 2]; // to do: convert to utf-8
	}
}

static byte const* skip_gd3_str( byte const* in, byte const* end )
{
	while ( end - in >= 2 )
	{
		in += 2;
		if ( !(in [-2] | in [-1]) )
			break;
	}
	return in;
}

static byte const* get_gd3_pair( byte const* in, byte const* end, gchar** out )
{
	byte const* mid = skip_gd3_str( in, end );
	if ( out )
		get_gd3_str( in, mid, out );
	return skip_gd3_str( mid, end );
}

static void get_vgm_gd3( byte const* in, long size, track_info_t* out )
{
	byte const* end = in + size;
	in = get_gd3_pair( in, end, &out->ti->track_name );
	in = get_gd3_pair( in, end, &out->ti->album_name );
	in = get_gd3_pair( in, end, 0 ); // system
	in = get_gd3_pair( in, end, &out->ti->performer );
	in = get_gd3_pair( in, end, 0 ); // copyright
	// ... other fields (release date, dumper, notes)
}

static void get_vgm_length( Vgm_Emu::header_t const& h, track_info_t* out )
{
	long length = get_le32( h.track_duration );
	if ( length > 0 )
	{
		out->length = length * 10 / 441; // 1000 / 44100 (VGM files used 44100 as timebase)
		out->loop = 0;
		
		long loop = get_le32( h.loop_duration );
		if ( loop > 0 && get_le32( h.loop_offset ) )
		{
			out->loop = loop * 10 / 441;
			out->intro = out->length - out->loop;
			out->length = -1;
		}
	}
}

inline void get_info_emu( Vgm_Emu& emu, track_info_t* out )
{
	get_vgm_length( emu.header(), out );
	
	int size;
	byte const* data = emu.gd3_data( &size );
	if ( data )
		get_vgm_gd3( data + 12, size, out );
}

inline void get_file_info( Vgm_Emu::header_t const& vgm_h, Data_Reader& in, track_info_t* out )
{
	get_vgm_length( vgm_h, out );
	
	// find gd3 header
	long gd3_offset = get_le32( vgm_h.gd3_offset ) + offsetof(Vgm_Emu::header_t,gd3_offset) -
			sizeof vgm_h;
	long gd3_max_size = in.remain() - gd3_offset;
	byte gd3_h [12];
	if ( gd3_offset <= 0 || gd3_max_size < (int) sizeof gd3_h )
		return;
	
	// read gd3 header
	if ( in.skip( gd3_offset ) || in.read( gd3_h, sizeof gd3_h ) )
		return;
	
	// check header signature and version
	if ( memcmp( gd3_h, "Gd3 ", 4 ) || get_le32( gd3_h + 4 ) >= 0x200 )
		return;
	
	// get and check size
	long gd3_size = get_le32( gd3_h + 8 );
	if ( gd3_size > gd3_max_size - 12 )
		return;
	
	// read and parse gd3 data
	blargg_vector<byte> gd3;
	if ( gd3.resize( gd3_size ) || in.read( gd3.begin(), gd3.size() ) )
		return;
	
	get_vgm_gd3( gd3.begin(), gd3.size(), out );
}

// File identification

enum { type_none = 0, type_spc, type_nsf, type_nsfe, type_vgm, type_gbs, type_gym };

int const tag_size = 4;
typedef char tag_t [tag_size];

static int identify_file( gchar* path, tag_t tag )
{
	// GYM file format doesn't require *any* header, just the ".gym" extension
	if ( g_str_has_suffix( path, ".gym" ) ) // to do: is pathname in unicode?
		return type_gym;
	// to do: trust suffix for all file types, avoiding having to look inside files?
	
	int result = type_none;
	if ( !memcmp( tag, "SNES", 4 ) ) result = type_spc;
	if ( !memcmp( tag, "NESM", 4 ) ) result = type_nsf;
	if ( !memcmp( tag, "NSFE", 4 ) ) result = type_nsfe;
	if ( !memcmp( tag, "GYMX", 4 ) ) result = type_gym;
	if ( !memcmp( tag, "GBS" , 3 ) ) result = type_gbs;
	if ( !memcmp( tag, "Vgm ", 4 ) ) result = type_vgm;
	return result;
}

static gint is_our_file( gchar* path )
{
	Audacious_Reader in;
	tag_t tag;
	return !in.open( path ) && !in.read( tag, sizeof tag ) && identify_file( path, tag );
}

// Get info

static int begin_get_info( const char* path, track_info_t* out )
{
	out->track  = 0;
	out->length = -1;
	out->loop   = -1;
	out->intro  = -1;
	TitleInput* fields = bmp_title_input_new();
	out->ti = fields;
	if ( !fields )
		return true;
	
	fields->file_name = g_path_get_basename( path );
	fields->file_path = g_path_get_dirname( path );
	return false;
}

static char* end_get_info( track_info_t const& info, int* length, bool* has_length )
{
	*length = info.length;
	if ( has_length )
		*has_length = (*length > 0);
	
	if ( *length <= 0 )
		*length = audcfg.loop_length * 1000;
	
	// use filename for formats that don't have field for name of game
	// to do: strip off file extension
	if ( !info.ti->track_name )
		info.ti->track_name = g_strdup( info.ti->file_name );
	
	char* result = xmms_get_titlestring( xmms_get_gentitle_format(), info.ti );
	g_free( info.ti );
	return result;
}

template<class Header>
inline void get_info_t( tag_t tag, Data_Reader& in, track_info_t* out, Header* )
{
	Header h;
	memcpy( &h, tag, tag_size );
	if ( !in.read( (char*) &h + tag_size, sizeof h - tag_size ) )
		get_file_info( h, in, out );
}

static void get_song_info( char* path, char** title, int* length )
{
	int track = 0; // to do: way to select other tracks
	
	*length = -1;
	*title = NULL;
	Audacious_Reader in;
	tag_t tag;
	if ( in.open( path ) || in.read( tag, sizeof tag ) )
		return;
	
	int type = identify_file( path, tag );
	if ( !type )
		return;
	
	track_info_t info;
	if ( begin_get_info( path, &info ) )
		return;
	info.track = track;
	
	switch ( type )
	{
		case type_nsf: get_info_t( tag, in, &info, (Nsf_Emu::header_t*) 0 ); break;
		case type_gbs: get_info_t( tag, in, &info, (Gbs_Emu::header_t*) 0 ); break;
		case type_gym: get_info_t( tag, in, &info, (Gym_Emu::header_t*) 0 ); break;
		case type_vgm: get_info_t( tag, in, &info, (Vgm_Emu::header_t*) 0 ); break;
		case type_spc: get_info_t( tag, in, &info, (Spc_Emu::header_t*) 0 ); break;
		case type_nsfe:get_info_t( tag, in, &info, (Nsfe_Emu::header_t*)0 ); break;
	}
	*title = end_get_info( info, length, 0 );
}

// Playback

static int silence_pending;

static void* play_loop_track( gpointer )
{
	g_static_mutex_lock( &playback_mutex );
	
	while ( console_ip_is_going )
	{
		int const buf_size = 1024;
		Music_Emu::sample_t buf [buf_size];
		
		// handle pending seek
		long s = pending_seek;
		pending_seek = -1; // to do: use atomic swap
		if ( s >= 0 )
		{
			console_ip.output->flush( s * 1000 );
			track_emu.seek( s * 1000 );
		}

		// fill buffer
		if ( track_emu.play( buf_size, buf ) )
			console_ip_is_going = 0;
		produce_audio( console_ip.output->written_time(), 
			FMT_S16_NE, 1, sizeof buf, buf, 
			&console_ip_is_going );
	}
	
	// stop playing
	unload_file();
	console_ip.output->close_audio();
	console_ip_is_going = 0;
	g_static_mutex_unlock( &playback_mutex );
	// to do: should decode_thread be cleared here?
	g_thread_exit( NULL );
	return NULL;
}

template<class Emu>
void load_file( tag_t tag, Data_Reader& in, long rate, track_info_t* out, Emu* dummy )
{
	typename Emu::header_t h;
	memcpy( &h, tag, tag_size );
	if ( in.read( (char*) &h + tag_size, sizeof h - tag_size ) )
		return;
	
	Emu* local_emu = new Emu;
	if ( !local_emu || local_emu->set_sample_rate( rate ) || local_emu->load( h, in ) )
	{
		delete local_emu; // delete NULL is safe
		return;
	}
	
	emu = local_emu;
	get_info_emu( *local_emu, out );
}

static void play_file( char* path )
{
	int track = 0; // to do: some way to select other tracks

	// open and identify file
	unload_file();
	Audacious_Reader in;
	tag_t tag;
	if ( in.open( path ) || in.read( tag, sizeof tag ) )
		return;
	int type = identify_file( path, tag );
	
	// setup info
	long sample_rate = 44100;
	if ( type == type_spc )
		sample_rate = Spc_Emu::native_sample_rate;
	if ( audcfg.resample )
		sample_rate = audcfg.resample_rate;
	track_info_t info;
	info.track = track;
	if ( begin_get_info( path, &info ) )
		return;
	
	// load in emulator and get info
	switch ( type )
	{
		case type_nsf: load_file( tag, in, sample_rate, &info, (Nsf_Emu*) 0 ); break;
		case type_nsfe:load_file( tag, in, sample_rate, &info, (Nsfe_Emu*)0 ); break;
		case type_gbs: load_file( tag, in, sample_rate, &info, (Gbs_Emu*) 0 ); break;
		case type_gym: load_file( tag, in, sample_rate, &info, (Gym_Emu*) 0 ); break;
		case type_vgm: load_file( tag, in, sample_rate, &info, (Vgm_Emu*) 0 ); break;
		case type_spc: load_file( tag, in, sample_rate, &info, (Spc_Emu*) 0 ); break;
	}
	in.close();
	if ( !emu )
		return;
	
	// set info
	int length = -1;
	bool has_length = false;
	char* title = end_get_info( info, &length, &has_length );
	if ( title )
	{
		console_ip.set_info( title, length, emu->voice_count() * 1000, sample_rate, 2 );
		g_free( title );
	}
	
	// start
    if ( !console_ip.output->open_audio( FMT_S16_NE, sample_rate, 2 ) )
		return;
	pending_seek = -1;

	track_emu.start_track( emu, track, length, !has_length );
	console_ip_is_going = 1;
	decode_thread = g_thread_create( play_loop_track, NULL, TRUE, NULL );
}

static void seek( gint time )
{
	// to do: be sure seek works at all
	// to do: disallow seek on slow formats (SPC, GYM, VGM using FM)?
	pending_seek = time;
}

static void console_stop(void)
{
	console_ip_is_going = 0;
	if ( decode_thread )
	{
		g_thread_join( decode_thread );
		decode_thread = NULL;
	}
	console_ip.output->close_audio();
	unload_file();
}

static void console_pause(gshort p)
{
	console_ip.output->pause(p);
}

static int get_time(void)
{
	return console_ip_is_going ? console_ip.output->output_time() : -1;
}

// Setup

static void console_init(void)
{
	ConfigDb *db;

	db = bmp_cfg_db_open();

	bmp_cfg_db_get_int(db, "console", "loop_length", &audcfg.loop_length);
	bmp_cfg_db_get_bool(db, "console", "resample", &audcfg.resample);
	bmp_cfg_db_get_int(db, "console", "resample_rate", &audcfg.resample_rate);
	bmp_cfg_db_get_bool(db, "console", "nsfe_playlist", &audcfg.nsfe_playlist);
	
	bmp_cfg_db_close(db);
}

extern "C" void console_aboutbox(void)
{
	xmms_show_message(_("About the Console Music Decoder"),
			_("Console music decoder engine based on Game_Music_Emu 0.3.0.\n"
			  "Audacious implementation by: William Pitcock <nenolod@nenolod.net>, \n"
			// Please do not put my hotpop.com address in the clear (I hate spam)
			  "        Shay Green <hotpop.com@blargg>"),
			_("Ok"),
			FALSE, NULL, NULL);
}

InputPlugin console_ip =
{
	NULL,
	NULL,
	NULL,
	console_init,
	console_aboutbox,
	NULL,
	is_our_file,
	NULL,
	play_file,
	console_stop,
	console_pause,
	seek,
	NULL,
	get_time,
	NULL,
	NULL,
	NULL,   
	NULL,
	NULL,
	NULL,
	NULL,
	get_song_info,
	NULL,
	NULL
};

extern "C" InputPlugin *get_iplugin_info(void)
{
	console_ip.description = g_strdup_printf(_("SPC, VGM, NSF/NSFE, GBS, and GYM module decoder"));
	return &console_ip;
}

