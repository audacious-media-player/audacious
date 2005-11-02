
// Finite Impulse Response (FIR) Resampler

// Game_Music_Emu 0.2.4. Copyright (C) 2004 Shay Green. GNU LGPL license.

#ifndef FIR_RESAMPLER_H
#define FIR_RESAMPLER_H

#include "blargg_common.h"

class Fir_Resampler {
	enum { width = 24 };
	enum { latency = (width - 1) * 2 };
public:
	typedef short sample_t;
	
	Fir_Resampler();
	~Fir_Resampler();
	
	// interface hasn't been stabilized yet
	
	// Set size of buffer. Return true if out of memory.
	blargg_err_t buffer_size( int );
	
	// Set input/output resampling ratio and frequency rolloff. Return
	// actual (rounded) ratio used.
	double time_ratio( double ratio, double rolloff = 0.999, double volume = 1.0 );
	
	// Remove any buffered samples and clear buffer
	void clear();
	
	// Pointer to buffer to write input samples to
	sample_t* buffer() { return write_pos; }
	
	// Maximum number of samples that can be written to buffer at current position
	int max_write() const { return buf_size - (write_pos - buf); }
	
	// Number of unread input samples
	int written() const { return write_pos - buf - latency; }
	
	// Advance buffer position by 'count' samples. Call after writing 'count' samples
	// to buffer().
	void write( int count );
	
	// True if there are there aren't enough input samples to read at least one
	// output sample.
	bool empty() const { return (write_pos - buf) <= latency; }
	
	// Resample and output at most 'count' into 'buf', then remove input samples from
	// buffer. Return number of samples written into 'buf'.
	int read( sample_t* buf, int count );
	
	// Skip at most 'count' *input* samples. Return number of samples actually skipped.
	int skip_input( int count );
	
private:
	enum { max_res = 32 };
	
	sample_t* buf;
	sample_t* write_pos;
	double ratio;
	int buf_size;
	int res;
	int imp;
	unsigned long skip_bits;
	int step;
	sample_t impulses [max_res] [width];
};

	inline void Fir_Resampler::write( int count ) {
		write_pos += count;
		assert( unsigned (write_pos - buf) <= buf_size );
	}

#endif

