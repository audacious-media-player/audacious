
// Music track emulator that handles fading and lookahead silence detection

#ifndef TRACK_EMU_H
#define TRACK_EMU_H

#include "Music_Emu.h"

class Track_Emu {
public:
	// Start track and start fade at fade_time. If detect_silence is true,
	// continually checks for end-of-track silence of more than around 6
	// seconds. Keeps pointer to emulator.
	void start_track( Music_Emu*, int track, long fade_time_msec, bool detect_silence );
	
	// Seek to new time in track
	void seek( long msec );

	long tell() const;
	
	// Play for 'count' samples and write to output buffer. Returns true when track
	// has ended.
	bool play( int count, Music_Emu::sample_t* out );
	
private:
	Music_Emu* emu;
	Music_Emu::sample_t* buffer;
	double fade_factor;
	long emu_time;      // number of samples emulator has generated since start of track
	long out_time;      // number of samples played since start of track
	long silence_time;  // number of samples where most recent silence began
	long fade_time;     // number of samples to begin fading at
	int silence_count;  // number of samples of silence to play before using bud
	int buf_count;      // number of samples left in buffer
	int track;
	bool detect_silence;
	bool track_ended;
	enum { buf_size = 1024 };
	Music_Emu::sample_t buf [buf_size];
	
	void end_track();
	void restart_track();
	void sync( long time );
	void fill_buf( bool check_silence );
	long msec_to_samples( long msec ) const;
};


#endif

