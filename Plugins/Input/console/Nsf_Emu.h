
// Nintendo Entertainment System (NES) NSF music file emulator

// Game_Music_Emu 0.3.0

#ifndef NSF_EMU_H
#define NSF_EMU_H

#include "Classic_Emu.h"
#include "Nes_Apu.h"
#include "Nes_Cpu.h"

typedef Nes_Emu Nsf_Emu;

class Nes_Emu : public Classic_Emu {
public:

	// Set internal gain, where 1.0 results in almost no clamping. Default gain
	// roughly matches volume of other emulators.
	Nes_Emu( double gain = 1.4 );
	
	// NSF file header
	struct header_t
	{
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
	
	// Load NSF data
	blargg_err_t load( Data_Reader& );
	
	// Load NSF using already-loaded header and remaining data
	blargg_err_t load( header_t const&, Data_Reader& );
	
	// Header for currently loaded NSF
	header_t const& header() const { return header_; }
	
	// Equalizer profiles for US NES and Japanese Famicom
	static equalizer_t const nes_eq;
	static equalizer_t const famicom_eq;
	
public:
	~Nes_Emu();
	void start_track( int );
	Nes_Apu* apu_() { return &apu; }
	const char** voice_names() const;
protected:
	void set_voice( int, Blip_Buffer*, Blip_Buffer*, Blip_Buffer* );
	void update_eq( blip_eq_t const& );
	blip_time_t run_clocks( blip_time_t, bool* );
	virtual void call_play();
protected:
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
	nes_time_t next_play;
	long play_period;
	int play_extra;
	nes_time_t clock() const;
	nes_time_t next_irq( nes_time_t end_time );
	static void irq_changed( void* );
	
	// rom
	int total_banks;
	blargg_vector<byte> rom;
	static int read_code( Nsf_Emu*, nes_addr_t );
	void unload();
	
	blargg_err_t init_sound();
	
	// expansion sound
	
	class Nes_Namco_Apu* namco;
	static int read_namco( Nsf_Emu*, nes_addr_t );
	static void write_namco( Nsf_Emu*, nes_addr_t, int );
	static void write_namco_addr( Nsf_Emu*, nes_addr_t, int );
	
	class Nes_Vrc6_Apu* vrc6;
	static void write_vrc6( Nsf_Emu*, nes_addr_t, int );
	
	class Nes_Fme7_Apu* fme7;
	static void write_fme7( Nsf_Emu*, nes_addr_t, int );
	
	// large objects
	
	header_t header_;
	
	// cpu
	Nes_Cpu cpu;
	void cpu_jsr( unsigned pc, int adj );
	static int read_low_mem( Nsf_Emu*, nes_addr_t );
	static void write_low_mem( Nsf_Emu*, nes_addr_t, int );
	static int read_unmapped( Nsf_Emu*, nes_addr_t );
	static void write_unmapped( Nsf_Emu*, nes_addr_t, int );
	static void write_exram( Nsf_Emu*, nes_addr_t, int );
	
	// apu
	Nes_Apu apu;
	static int read_snd( Nsf_Emu*, nes_addr_t );
	static void write_snd( Nsf_Emu*, nes_addr_t, int );
	static int pcm_read( void*, nes_addr_t );
	
	// sram
	enum { sram_size = 0x2000 };
	byte sram [sram_size];
	static int read_sram( Nsf_Emu*, nes_addr_t );
	static void write_sram( Nsf_Emu*, nes_addr_t, int );
};

#endif

