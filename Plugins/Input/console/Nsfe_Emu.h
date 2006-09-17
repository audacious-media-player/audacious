
// Nintendo Entertainment System (NES) NSFE-format game music file emulator

// Game_Music_Emu 0.3.0

#ifndef NSFE_EMU_H
#define NSFE_EMU_H

#include "blargg_common.h"
#include "Nsf_Emu.h"

// to do: eliminate dependence on bloated std vector
#include <vector>

class Nsfe_Info {
public:
	struct header_t
	{
		char tag [4]; // 'N', 'S', 'F', 'E'
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 4 );
	
	// Load NSFE info and optionally load file into Nsf_Emu
	blargg_err_t load_file( const char* path, Nsf_Emu* = 0 );
	
	// Load NSFE info and optionally load file into Nsf_Emu
	blargg_err_t load( Data_Reader&, Nsf_Emu* = 0 );
	
	// Load NSFE info and optionally load file into Nsf_Emu
	blargg_err_t load( header_t const&, Data_Reader&, Nsf_Emu* = 0 );
	
	// Information about current file
	struct info_t : Nsf_Emu::header_t
	{
		// These (longer) fields hide those in Nsf_Emu::header_t
		char game [256];
		char author [256];
		char copyright [256];
		char ripper [256];
	};
	const info_t& info() const { return info_; }
	
	// All track indicies are 0-based
	
	// Name of track [i], or "" if none available
	const char* track_name( unsigned i ) const;
	
	// Duration of track [i] in milliseconds, negative if endless, or 0 if none available
	long track_time( unsigned i ) const;
	
	// Optional playlist consisting of track indicies
	int playlist_size() const { return playlist.size(); }
	int playlist_entry( int i ) const { return playlist [i]; }
	
	// If true and playlist is present in NSFE file, remap track numbers using it
	void enable_playlist( bool = true );
	
public:
	Nsfe_Info();
	~Nsfe_Info();
	int track_count() const { return info_.track_count; }
private:
	std::vector<char> track_name_data;
	std::vector<const char*> track_names;
	std::vector<unsigned char> playlist;
	std::vector<long> track_times;
	int track_count_;
	info_t info_;
	bool playlist_enabled;
	
	int remap_track( int i ) const;
	friend class Nsfe_Emu;
};

class Nsfe_Emu : public Nsf_Emu, public Nsfe_Info {
public:
	// See Nsf_Emu.h for further information
	
	Nsfe_Emu( double gain = 1.4 ) : Nsf_Emu( gain ) { }
	
	typedef Nsfe_Info::header_t header_t;
	
	// Load NSFE data
	blargg_err_t load( Emu_Reader& r ) { return Nsfe_Info::load( r, this ); }
	
	// Load NSFE using already-loaded header and remaining data
	blargg_err_t load( header_t const& h, Emu_Reader& r ) { return Nsfe_Info::load( h, r, this ); }
	
public:
	Nsf_Emu::track_count;
	Nsf_Emu::load_file;
	void start_track( int );
	void enable_playlist( bool = true );
};

inline void Nsfe_Emu::enable_playlist( bool b )
{
	Nsfe_Info::enable_playlist( b );
	set_track_count( info().track_count );
}

#endif

