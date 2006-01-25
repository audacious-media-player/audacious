
// Game music emulator interface base class

// Game_Music_Emu 0.3.0

#ifndef MUSIC_EMU_H
#define MUSIC_EMU_H

#include "blargg_common.h"
#include "abstract_file.h"
class Multi_Buffer;

class Music_Emu {
public:
	
	// Initialize emulator with specified sample rate. Currently should only be
	// called once.
	virtual blargg_err_t set_sample_rate( long sample_rate ) = 0;
	
	// Load music file
	blargg_err_t load_file( const char* path );
	
	// Start a track, where 0 is the first track. Might un-mute any muted voices.
	virtual void start_track( int ) = 0;
	
	// Generate 'count' samples info 'buf'. Output is in stereo unless using custom
	// buffer that generates mono output.
	typedef short sample_t;
	virtual void play( long count, sample_t* buf ) = 0;
	
// Additional optional features
	
	// Request use of custom multichannel buffer. Only supported by "classic" emulators;
	// on others this has no effect. Should be called only once *before* set_sample_rate().
	virtual void set_buffer( Multi_Buffer* ) { }
	
	// Load music file data from custom source
	virtual blargg_err_t load( Data_Reader& ) = 0;
	
	// Sample rate sound is generated at
	long sample_rate() const;
	
	// Number of voices used by currently loaded file
	int voice_count() const;
	
	// Names of voices
	virtual const char** voice_names() const;
	
	// Mute voice n if bit n (1 << n) of mask is set
	virtual void mute_voices( int mask );
	
	// Frequency equalizer parameters (see notes.txt)
	struct equalizer_t {
		double treble; // -50.0 = muffled, 0 = flat, +5.0 = extra-crisp
		long   bass;   // 1 = full bass, 90 = average, 16000 = almost no bass
	};
	
	// Current frequency equalizater parameters
	const equalizer_t& equalizer() const;
	
	// Set frequency equalizer parameters
	virtual void set_equalizer( equalizer_t const& );
	
	// Equalizer settings for TV speaker
	static equalizer_t const tv_eq;
	
	// Number of tracks. Zero if file hasn't been loaded yet.
	int track_count() const;
	
	// Skip 'count' samples
	virtual void skip( long count );
	
	// True if a track was started and has since ended. Currently only logged
	// format tracks (VGM, GYM) without loop points have an ending.
	bool track_ended() const;
	
	// Number of errors encountered while playing track due to undefined CPU
	// instructions in emulated formats and undefined stream events in
	// logged formats.
	int error_count() const;
	
	Music_Emu();
	virtual ~Music_Emu();
	
protected:
	typedef BOOST::uint8_t byte;
	void set_voice_count( int n ) { voice_count_ = n; }
	void set_track_count( int n ) { track_count_ = n; }
	void set_track_ended( bool b = true ) { track_ended_ = b; }
	void log_error() { error_count_++; }
	void remute_voices();
private:
	// noncopyable
	Music_Emu( const Music_Emu& );
	Music_Emu& operator = ( const Music_Emu& );
	
	equalizer_t equalizer_;
	long sample_rate_;
	int voice_count_;
	int mute_mask_;
	int track_count_;
	int error_count_;
	bool track_ended_;
};

// Deprecated
typedef Data_Reader Emu_Reader;
typedef Std_File_Reader Emu_Std_Reader;
typedef Mem_File_Reader Emu_Mem_Reader;

inline int Music_Emu::error_count() const   { return error_count_; }
inline int Music_Emu::voice_count() const   { return voice_count_; }
inline int Music_Emu::track_count() const   { return track_count_; }
inline bool Music_Emu::track_ended() const  { return track_ended_; }
inline void Music_Emu::mute_voices( int mask ) { mute_mask_ = mask; }
inline void Music_Emu::remute_voices() { mute_voices( mute_mask_ ); }
inline const Music_Emu::equalizer_t& Music_Emu::equalizer() const { return equalizer_; }
inline void Music_Emu::set_equalizer( const equalizer_t& eq ) { equalizer_ = eq; }
inline long Music_Emu::sample_rate() const { return sample_rate_; }

inline blargg_err_t Music_Emu::set_sample_rate( long r )
{
	assert( !sample_rate_ ); // sample rate can't be changed once set
	sample_rate_ = r;
	return blargg_success;
}

inline void Music_Emu::start_track( int track )
{
	assert( (unsigned) track <= (unsigned) track_count() );
	assert( sample_rate_ ); // set_sample_rate() must have been called first
	track_ended_ = false;
	error_count_ = 0;
}

#endif

