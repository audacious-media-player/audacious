
// Game music emulator interface base class

// Game_Music_Emu 0.2.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef MUSIC_EMU_H
#define MUSIC_EMU_H

#include "blargg_common.h"

#include "abstract_file.h"
typedef Data_Reader Emu_Reader; // File reader base class
typedef Std_File_Reader Emu_Std_Reader; // Read from standard file
typedef Mem_File_Reader Emu_Mem_Reader; // Read from block of memory

class Music_Emu {
public:
	Music_Emu();
	virtual ~Music_Emu();
	
	// Number of voices used by currently loaded file
	int voice_count() const;
	
	// Names of voices
	virtual const char** voice_names() const;
	
	// Number of tracks. Zero if file hasn't been loaded yet.
	int track_count() const;
	
	// Start a track, where 0 is the first track. Might un-mute any muted voices.
	virtual blargg_err_t start_track( int ) = 0;
	
	// Mute voice n if bit n (1 << n) of mask is set
	virtual void mute_voices( int mask );
	
	// Generate 'count' samples info 'buf'
	typedef short sample_t;
	virtual blargg_err_t play( long count, sample_t* buf ) = 0;
	
	// Skip 'count' samples
	virtual blargg_err_t skip( long count );
	
	// True if a track was started and has since ended. Currently only dumped
	// format tracks (VGM, GYM) without loop points have an ending.
	bool track_ended() const;
	
	
// End of public interface
protected:
	typedef BOOST::uint8_t byte; // used often
	int track_count_;
	int voice_count_;
	int mute_mask_;
	bool track_ended_;
private:
	// noncopyable
	Music_Emu( const Music_Emu& );
	Music_Emu& operator = ( const Music_Emu& );
};

inline int Music_Emu::voice_count() const   { return voice_count_; }
inline int Music_Emu::track_count() const   { return track_count_; }
inline bool Music_Emu::track_ended() const  { return track_ended_; }

#endif

