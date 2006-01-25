
// Nintendo Game Boy GBS music file emulator

// Game_Music_Emu 0.3.0

#ifndef GBS_EMU_H
#define GBS_EMU_H

#include "Classic_Emu.h"
#include "Gb_Apu.h"
#include "Gb_Cpu.h"

class Gbs_Emu : public Classic_Emu {
public:
	
	// Sets internal gain, where 1.0 results in almost no clamping. Default gain
	// roughly matches volume of other emulators.
	Gbs_Emu( double gain = 1.2 );
	
	// GBS file header
	struct header_t
	{
		char tag [3];
		byte vers;
		byte track_count;
		byte first_track;
		byte load_addr [2];
		byte init_addr [2];
		byte play_addr [2];
		byte stack_ptr [2];
		byte timer_modulo;
		byte timer_mode;
		char game [32];
		char author [32];
		char copyright [32];
		
		enum { song = 0 }; // no song titles
	};
	BOOST_STATIC_ASSERT( sizeof (header_t) == 112 );
	
	// Load GBS data
	blargg_err_t load( Data_Reader& );
	
	// Load GBS using already-loaded header and remaining data
	blargg_err_t load( header_t const&, Data_Reader& );
	
	// Header for currently loaded GBS
	header_t const& header() const { return header_; }
	
	// Equalizer profiles for Game Boy Color speaker and headphones
	static equalizer_t const handheld_eq;
	static equalizer_t const headphones_eq;
	
public:
	~Gbs_Emu();
	const char** voice_names() const;
	void start_track( int );
protected:
	void set_voice( int, Blip_Buffer*, Blip_Buffer*, Blip_Buffer* );
	void update_eq( blip_eq_t const& );
	blip_time_t run_clocks( blip_time_t, bool* );
private:
	// rom
	const byte* rom_bank;
	blargg_vector<byte> rom;
	void unload();
	int bank_count;
	void set_bank( int );
	static void write_rom( Gbs_Emu*, gb_addr_t, int );
	static int read_rom( Gbs_Emu*, gb_addr_t );
	static int read_bank( Gbs_Emu*, gb_addr_t );
	
	// state
	gb_addr_t load_addr;
	gb_addr_t init_addr;
	gb_addr_t play_addr;
	gb_addr_t stack_ptr;
	int timer_modulo_init;
	int timer_mode;
	
	// timer
	gb_time_t cpu_time;
	gb_time_t play_period;
	gb_time_t next_play;
	int double_speed;
	
	// hardware
	Gb_Apu apu;
	void set_timer( int tma, int tmc );
	static int read_io( Gbs_Emu*, gb_addr_t );
	static void write_io( Gbs_Emu*, gb_addr_t, int );
	static int read_unmapped( Gbs_Emu*, gb_addr_t );
	static void write_unmapped( Gbs_Emu*, gb_addr_t, int );
	
	// large objects
	
	header_t header_;
	byte hi_page [0x100];
	Gb_Cpu cpu;
	void cpu_jsr( gb_addr_t );
	gb_time_t clock() const;
	byte ram [0x4000];
	static int read_ram( Gbs_Emu*, gb_addr_t );
	static void write_ram( Gbs_Emu*, gb_addr_t, int );
};

#endif

