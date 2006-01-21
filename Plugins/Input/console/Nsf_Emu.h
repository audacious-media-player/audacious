
// Nintendo Entertainment System (NES) NSF-format game music file emulator

// Game_Music_Emu 0.2.4. Copyright (C) 2003-2005 Shay Green. GNU LGPL license.

#ifndef NSF_EMU_H
#define NSF_EMU_H

#include "Classic_Emu.h"
#include "Nes_Apu.h"
#include "Nes_Cpu.h"

// If NSF_EMU_APU_ONLY is non-zero, external sound chip support is disabled
#if !NSF_EMU_APU_ONLY
	#include "Nes_Vrc6.h"
	#include "Nes_Namco.h"
#endif

class Nsf_Emu : public Classic_Emu {
public:
	// Set internal gain, where 1.0 results in almost no clamping. Default gain
	// roughly matches volume of other emulators.
	Nsf_Emu( double gain = 1.4 );
	~Nsf_Emu();
	
	struct header_t {
		char tag [5];
		byte vers;
		byte track_count;
		byte first_track;
		byte load_addr [2];
		byte init_addr [2];
		byte play_addr [2];
		char game [32];
		char author [32];
		char copyright [32];
		byte ntsc_speed [2];
		byte banks [8];
		byte pal_speed [2];
		byte speed_flags;
		byte chip_flags;
		byte unused [4];
		
		enum { song = 0 }; // no song titles
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 0x80 );
	
	// Load NSF, given its header and reader for remaining data
	blargg_err_t load( const header_t&, Emu_Reader& );
	
	blargg_err_t start_track( int );
	Nes_Apu* apu_() { return &apu; }
	const char** voice_names() const;
	

// End of public interface
protected:
	void set_voice( int, Blip_Buffer*, Blip_Buffer*, Blip_Buffer* );
	void update_eq( blip_eq_t const& );
	blip_time_t run( int, bool* );
private:
	// initial state
	enum { bank_count = 8 };
	byte initial_banks [bank_count];
	int initial_pcm_dac;
	double gain;
	bool needs_long_frames;
	bool pal_only;
	unsigned init_addr;
	unsigned play_addr;
	int exp_flags;
	
	// timing
	double clocks_per_msec;
	nes_time_t next_play;
	long play_period;
	int play_extra;
	nes_time_t clock() const;
	nes_time_t next_irq( nes_time_t end_time );
	static void irq_changed( void* );
	
	// rom
	int total_banks;
	byte* rom;
	static int read_code( Nsf_Emu*, nes_addr_t );
	void unload();
	
	// cpu
	Nes_Cpu cpu;
	void cpu_jsr( unsigned pc, int adj );
	static int read_low_mem( Nsf_Emu*, nes_addr_t );
	static void write_low_mem( Nsf_Emu*, nes_addr_t, int );
	static int read_unmapped( Nsf_Emu*, nes_addr_t );
	static void write_unmapped( Nsf_Emu*, nes_addr_t, int );
	static void write_exram( Nsf_Emu*, nes_addr_t, int );
	
	blargg_err_t init_sound();
	
	// apu
	Nes_Apu apu;
	static int read_snd( Nsf_Emu*, nes_addr_t );
	static void write_snd( Nsf_Emu*, nes_addr_t, int );
	static int pcm_read( void*, nes_addr_t );
	
#if !NSF_EMU_APU_ONLY
	// namco
	Nes_Namco namco;
	static int read_namco( Nsf_Emu*, nes_addr_t );
	static void write_namco( Nsf_Emu*, nes_addr_t, int );
	static void write_namco_addr( Nsf_Emu*, nes_addr_t, int );
	
	// vrc6
	Nes_Vrc6 vrc6;
	static void write_vrc6( Nsf_Emu*, nes_addr_t, int );
#endif
	
	// sram
	enum { sram_size = 0x2000 };
	byte sram [sram_size];
	static int read_sram( Nsf_Emu*, nes_addr_t );
	static void write_sram( Nsf_Emu*, nes_addr_t, int );
	
	friend class Nsf_Remote_Emu; // hack
};

class Nsf_Reader : public Std_File_Reader {
	VFSFile* file;
public:
	Nsf_Reader();
	~Nsf_Reader();
	
	// Custom reader for SPC headers [tempfix]
	blargg_err_t read_head( Nsf_Emu::header_t* );
};
#endif

