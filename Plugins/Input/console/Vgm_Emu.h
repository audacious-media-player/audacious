
// Sega Master System VGM-format game music file emulator (PSG chip only)

// Game_Music_Emu 0.2.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef VGM_EMU_H
#define VGM_EMU_H

#include "Classic_Emu.h"
#include "Sms_Apu.h"

class Vgm_Emu : public Classic_Emu {
public:
	// Set internal gain, where 1.0 results in almost no clamping. Default gain
	// roughly matches volume of other emulators.
	Vgm_Emu( double gain = 1.0 );
	~Vgm_Emu();
	
	struct header_t {
		char tag [4];
		byte data_size [4];
		byte vers [4];
		byte psg_rate [4];
		byte fm_rate [4];
		byte g3d_offset [4];
		byte sample_count [4];
		byte loop_offset [4];
		byte loop_duration [4];
		byte frame_rate [4];
		char unused [0x18];
		
	    enum { track_count = 1 }; // one track per file
		
		// no text fields
		enum { game = 0 };
		enum { song = 0 };
		enum { author = 0 };
		enum { copyright = 0 };
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 64 );
	
	// Load VGM, given its header and reader for remaining data
	blargg_err_t load( const header_t&, Emu_Reader& );
	
	// Determine length of track, in seconds (0 if track is endless).
	// Optionally returns pointer and size of data past end of sequence data
	// (i.e. any tagging information).
	int track_length( const byte** end_out = NULL, int* remain_out = NULL ) const;
	
	blargg_err_t start_track( int );
	const char** voice_names() const;
	

// End of public interface
protected:
	void set_voice( int, Blip_Buffer*, Blip_Buffer*, Blip_Buffer* );
	void update_eq( blip_eq_t const& );
	blip_time_t run( int, bool* );
private:
	byte* data;
	const byte* pos;
	const byte* end;
	const byte* loop_begin;
	long loop_duration;
	long loop_remain;
	long time_factor;
	int delay;
	Sms_Apu apu;
	
	sms_time_t clocks_from_samples( int ) const;
	void unload();
};

#endif

