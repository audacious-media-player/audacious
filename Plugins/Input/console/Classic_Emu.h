
// Classic game music emulator interface base class for emulators which use Blip_Buffer
// for sound output.

// Game_Music_Emu 0.2.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef CLASSIC_EMU_H
#define CLASSIC_EMU_H

#include "Music_Emu.h"
class Blip_Buffer;
class blip_eq_t;
class Multi_Buffer;

class Classic_Emu : public Music_Emu {
public:
	Classic_Emu();
	~Classic_Emu();
	
	// Initialize emulator with specified sample rate. Sample output is in stereo.
	virtual blargg_err_t init( long sample_rate );
	
	// Initialize emulator using custom output buffer
	blargg_err_t init( Multi_Buffer* buf );
	
	// Frequency equalizer parameters (see notes.txt)
	struct equalizer_t {
		double treble; // treble level at 22kHz, in dB (-3.0dB = 0.50)
		long cutoff;   // beginning of low-pass rolloff, in Hz
		long bass;     // high-pass breakpoint, in Hz
		equalizer_t( double treble_ = 0, long cutoff_ = 0, int bass_ = 33 ) :
				treble( treble_ ), cutoff( cutoff_ ), bass( bass_ ) { }
	};
	
	// Current frequency equalizater parameters
	const equalizer_t& equalizer() const;
	
	// Set frequency equalizer parameters
	void set_equalizer( const equalizer_t& );
	
	// See Music_Emu.h
	void mute_voices( int );
	blargg_err_t play( long, sample_t* );
	
	
// End of public interface
protected:
	virtual void starting_track();
	virtual blargg_err_t setup_buffer( long clock_rate );
	virtual void set_voice( int index, Blip_Buffer* center,
			Blip_Buffer* left = NULL, Blip_Buffer* right = NULL ) = 0;
	virtual long run( int msec, bool* added_stereo = NULL ) = 0;
	virtual void update_eq( blip_eq_t const& ) = 0;
private:
	Multi_Buffer* buf;
	Multi_Buffer* std_buf; // owned
	equalizer_t equalizer_;
	void update_eq_();
};

inline blargg_err_t Classic_Emu::init( Multi_Buffer* buf_ ) {
	buf = buf_;
	return blargg_success;
}
inline const Classic_Emu::equalizer_t& Classic_Emu::equalizer() const {
	return equalizer_;
}
#endif

