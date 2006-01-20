
// Super Nintendo (SNES) SPC music file emulator

// Game_Music_Emu 0.2.4. Copyright (C) 2004 Shay Green. GNU LGPL license.

#ifndef SPC_EMU_H
#define SPC_EMU_H

#include "Fir_Resampler.h"
#include "Music_Emu.h"
#include "Snes_Spc.h"

#include "blargg_common.h"
#include "abstract_file.h"

class Spc_Emu : public Music_Emu {
public:
	Spc_Emu();
	~Spc_Emu();
	
	// The Super Nintendo hardware samples at 32kHz
	enum { native_sample_rate = 32000 };
	
	// Initialize emulator with given sample rate and gain. A sample rate different than
	// the native 32kHz results in internal resampling to the desired rate. A gain of 1.0
	// results in almost no clamping. Default gain roughly matches volume of other emulators.
	blargg_err_t init( long sample_rate, double gain = 1.4 );
	
	struct header_t {
		char tag [35];
		byte format;
		byte version;
		byte pc [2];
		byte a, x, y, psw, sp;
		byte unused [2];
		char song [32];
		char game [32];
		char dumper [16];
		char comment [32];
		byte date [11];
		char len_secs [3];
		byte fade_msec [5];
		char author [32];
		byte mute_mask;
		byte emulator;
		byte unused2 [45];
		
		enum { copyright = 0 }; // no copyright field
	};

	int length;
	
	// Load SPC, given its header and reader for remaining data
	blargg_err_t load( const header_t&, Emu_Reader& );
	
	void mute_voices( int );
	blargg_err_t start_track( int );
	blargg_err_t play( long count, sample_t* );
	blargg_err_t skip( long );
	const char** voice_names() const;

	
// End of public interface
private:
	Snes_Spc apu;
	Fir_Resampler resampler;
	double resample_ratio;
	bool use_resampler;
	
	struct spc_file_t {
		header_t header;
		char data [0x10080];
	};
	BOOST_STATIC_ASSERT( sizeof (spc_file_t) == 0x10180 );
	
	spc_file_t file;
};

inline void Spc_Emu::mute_voices( int m ) {
	apu.mute_voices( m );
}

class Spc_Reader : public File_Reader {
	FILE* file;
public:
	Spc_Reader();
	~Spc_Reader();
	
	error_t open( const char* );
	
	// Custom reader for SPC headers [tempfix]
	blargg_err_t read_head( Spc_Emu::header_t* );
	
	long size() const;
	long read_avail( void*, long );
	
	long tell() const;
	error_t seek( long );
	
	void close();
};

#endif

