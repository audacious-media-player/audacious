
// Multi-channel buffer with pan control for each buffer

// Game_Music_Emu 0.2.4. Copyright (C) 2004 Shay Green. GNU LGPL license.

#ifndef PANNING_BUFFER_H
#define PANNING_BUFFER_H

#include "Multi_Buffer.h"

// Panning_Buffer uses several buffers and outputs stereo sample pairs.
class Panning_Buffer : public Multi_Buffer {
public:
	Panning_Buffer();
	~Panning_Buffer();
	
	// Set pan of a channel, using left and right gain values (1.0 = normal).
	// Use left_chan and right_chan for the common left and right buffers used
	// by all channels.
	enum { left_chan = -2 };
	enum { right_chan = -1 };
	void set_pan( int channel, double left, double right );
	
	// See Multi_Buffer.h
	blargg_err_t sample_rate( long rate, int msec );
	void clock_rate( long );
	void bass_freq( int );
	void clear();
	blargg_err_t set_channel_count( int );
	channel_t channel( int );
	void end_frame( blip_time_t, bool unused = true );
	long read_samples( blip_sample_t*, long );
	
private:
	typedef long fixed_t;
	
	struct buf_t : Blip_Buffer {
		fixed_t left_gain;
		fixed_t right_gain;
	};
	buf_t* bufs;
	int buf_count;
	long clock_rate_;
	int bass_freq_;
	
	void add_panned( buf_t&, blip_sample_t*, long );
};

#endif

