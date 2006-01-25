
// Sega Genesis GYM music file emulator

// Game_Music_Emu 0.3.0

#ifndef GYM_EMU_H
#define GYM_EMU_H

#include "Dual_Resampler.h"
#include "Ym2612_Emu.h"
#include "Music_Emu.h"
#include "Sms_Apu.h"

class Gym_Emu : public Music_Emu, private Dual_Resampler {
public:
	
	// GYM file header
	struct header_t
	{
	    char tag [4];
	    char song [32];
	    char game [32];
	    char copyright [32];
	    char emulator [32];
	    char dumper [32];
	    char comment [256];
	    byte loop_start [4]; // in 1/60 seconds, 0 if not looped
	    byte packed [4];
	    
	    enum { track_count = 1 }; // one track per file
		enum { author = 0 }; // no author field
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 428 );
	
	// Load GYM data
	blargg_err_t load( Data_Reader& );
	
	// Load GYM file using already-loaded header and remaining data
	blargg_err_t load( header_t const&, Data_Reader& );
	blargg_err_t load( void const* data, long size ); // keeps pointer to data
	
	// Header for currently loaded GYM (cleared to zero if GYM lacks header)
	header_t const& header() const { return header_; }
	
	// Length of track in 1/60 seconds
	enum { gym_rate = 60 }; // GYM time units (frames) per second
	long track_length() const;

public:
	typedef Music_Emu::sample_t sample_t;
	Gym_Emu();
	~Gym_Emu();
	blargg_err_t set_sample_rate( long sample_rate );
	void mute_voices( int );
	void start_track( int );
	void play( long count, sample_t* );
	const char** voice_names() const;
	void skip( long count );
public:
	// deprecated
	blargg_err_t init( long r, double gain = 1.5, double oversample = 5 / 3.0 )
	{
		return set_sample_rate( r );
	}
protected:
	int play_frame( blip_time_t blip_time, int sample_count, sample_t* buf );
private:
	// sequence data begin, loop begin, current position, end
	const byte* data;
	const byte* loop_begin;
	const byte* pos;
	const byte* data_end;
	long loop_remain; // frames remaining until loop beginning has been located
	blargg_vector<byte> mem;
	header_t header_;
	blargg_err_t load_( const void* file, long data_offset, long file_size );
	void unload();
	void parse_frame();
	
	// dac (pcm)
	int dac_amp;
	int prev_dac_count;
	bool dac_enabled;
	bool dac_muted;
	void run_dac( int );
	
	// sound
	Blip_Buffer blip_buf;
	Ym2612_Emu fm;
	Blip_Synth<blip_med_quality,1> dac_synth;
	Sms_Apu apu;
	byte dac_buf [1024];
};

#endif

