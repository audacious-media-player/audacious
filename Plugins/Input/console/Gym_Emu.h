
// Sega Genesis GYM music file emulator

// Game_Music_Emu 0.2.4. Copyright (C) 2004-2005 Shay Green. GNU LGPL license.

#ifndef GYM_EMU_H
#define GYM_EMU_H

#include "Fir_Resampler.h"
#include "Blip_Buffer.h"
#include "Music_Emu.h"
#include "Sms_Apu.h"
#include "ym2612.h"

class Gym_Emu : public Music_Emu {
public:
	Gym_Emu();
	~Gym_Emu();
	
	// Initialize emulator with given sample rate, gain, and oversample. A gain of 1.0
	// results in almost no clamping. Default gain roughly matches volume of other emulators.
	// The FM chip is synthesized at an increased rate governed by the oversample factor,
	// where 1.0 results in no oversampling and > 1.0 results in oversampling.
	blargg_err_t init( long sample_rate, double gain = 1.5, double oversample = 5 / 3.0 );
	
	struct header_t {
	    char tag [4];
	    char song [32];
	    char game [32];
	    char copyright [32];
	    char emulator [32];
	    char dumper [32];
	    char comment [256];
	    byte loop [4];
	    byte packed [4];
	    
	    enum { track_count = 1 }; // one track per file
		enum { author = 0 }; // no author field
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 428 );
	
	// Load GYM, given its header and reader for remaining data
	blargg_err_t load( const header_t&, Emu_Reader& );
	
	// Load GYM, given pointer to complete file data. Keeps reference
	// to data, but doesn't free it.
	blargg_err_t load( const void*, long size );
	
	// Length of track, in seconds (0 if looped)
	int track_length() const;
	
	void mute_voices( int );
	blargg_err_t start_track( int );
	blargg_err_t play( long count, sample_t* );
	const char** voice_names() const;
	blargg_err_t skip( long count );
	
// End of public interface
private:
	// sequence data begin, loop begin, current position, end
	const byte* data;
	const byte* loop_begin;
	const byte* pos;
	const byte* data_end;
	long loop_offset;
	long loop_remain; // frames remaining until loop beginning has been located
	byte* mem;
	blargg_err_t load_( const void* file, long data_offset, long file_size );
	
	// frames
	double oversample;
	double clocks_per_sample;
	int pairs_per_frame;
	int oversamples_per_frame;
	void parse_frame();
	void play_frame( sample_t* );
	void mix_samples( sample_t* );
	
	// dac (pcm)
	int last_dac;
	int prev_dac_count;
	bool dac_enabled;
	bool dac_disabled;
	void run_dac( int );
	
	// sound
	int extra_pos; // extra samples remaining from last read
	Blip_Buffer blip_buf;
	YM2612_Emu fm;
	Blip_Synth<blip_med_quality,256> dac_synth;
	Sms_Apu apu;
	Fir_Resampler resampler;
	byte dac_buf [1024];
	enum { sample_buf_size = 4096 };
	sample_t sample_buf [sample_buf_size];
	
	void unload();
};

class Gym_Reader : public Std_File_Reader {
	VFSFile* file;
public:
	Gym_Reader();
	~Gym_Reader();
	
	// Custom reader for SPC headers [tempfix]
	blargg_err_t read_head( Gym_Emu::header_t* );
};

#endif

