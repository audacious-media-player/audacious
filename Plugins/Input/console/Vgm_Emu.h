
// Multi-format VGM music emulator with support for SMS PSG and Mega Drive FM

// Game_Music_Emu 0.3.0

#ifndef VGM_EMU_H
#define VGM_EMU_H

#include "abstract_file.h"
#include "Vgm_Emu_Impl.h"

// Emulates VGM music using SN76489/SN76496 PSG, YM2612, and YM2413 FM sound chips.
// Supports custom sound buffer and frequency equalization when VGM uses just the PSG.
// FM sound chips can be run at their proper rates, or slightly higher to reduce
// aliasing on high notes. Currently YM2413 support requires that you supply a
// YM2413 sound chip emulator. I can provide one I've modified to work with the library.
class Vgm_Emu : public Vgm_Emu_Impl {
public:
	
	// Oversample runs FM chips at higher than normal rate. Tempo adjusts speed of
	// music, but not pitch.
	Vgm_Emu( bool oversample = true, double tempo = 1.0 );
	
	// VGM header format
	struct header_t
	{
		char tag [4];
		byte data_size [4];
		byte version [4];
		byte psg_rate [4];
		byte ym2413_rate [4];
		byte gd3_offset [4];
		byte track_duration [4];
		byte loop_offset [4];
		byte loop_duration [4];
		byte frame_rate [4];
		byte noise_feedback [2];
		byte noise_width;
		byte unused1;
		byte ym2612_rate [4];
		byte ym2151_rate [4];
		byte data_offset [4];
		byte unused2 [8];
		
	    enum { track_count = 1 }; // one track per file
		enum { time_rate = 44100 }; // all times specified at this rate
		
		// track information is in gd3 data
		enum { game = 0 };
		enum { song = 0 };
		enum { author = 0 };
		enum { copyright = 0 };
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 64 );

	// Load VGM data
	blargg_err_t load( Data_Reader& );
	
	// Load VGM using already-loaded header and remaining data
	blargg_err_t load( header_t const&, Data_Reader& );
	
	// Load VGM using pointer to file data. Keeps pointer to data.
	blargg_err_t load( void const* data, long size );
	
	// Header for currently loaded VGM
	header_t const& header() const { return header_; }
	
	// Pointer to gd3 data, or NULL if none. Optionally returns size of data.
	// Checks for GD3 header and that version is less than 2.0.
	byte const* gd3_data( int* size_out = NULL ) const;
	
	// to do: find better name for this
	// True if Classic_Emu operations are supported
	bool is_classic_emu() const { return !uses_fm; }
	
public:
	~Vgm_Emu();
	blargg_err_t set_sample_rate( long sample_rate );
	void start_track( int );
	void mute_voices( int mask );
	const char** voice_names() const;
	void play( long count, sample_t* );
public:
	// deprecated
	int track_length( const byte** end_out = NULL, int* remain_out = NULL ) const
	{
		return  (header().track_duration [3]*0x1000000L +
				header().track_duration [2]*0x0010000L + 
				header().track_duration [1]*0x0000100L + 
				header().track_duration [0]) / header_t::time_rate;
	}
protected:
	// Classic_Emu
	void set_voice( int, Blip_Buffer*, Blip_Buffer*, Blip_Buffer* );
	void update_eq( blip_eq_t const& );
	blip_time_t run( int, bool* );
private:
	header_t header_;
	blargg_vector<byte> mem;
	long vgm_rate;
	bool oversample;
	bool uses_fm;
	
	blargg_err_t init_( long sample_rate );
	blargg_err_t load_( const header_t&, void const* data, long size );
	blargg_err_t setup_fm();
	void unload();
};

inline blargg_err_t Vgm_Emu::load( void const* data, long size )
{
	unload();
	return load_( *(header_t*) data, (char*) data + sizeof (header_t),
			size - sizeof (header_t) );
}

#endif

