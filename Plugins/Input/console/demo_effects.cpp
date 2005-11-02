
// Use Effects_Buffer to add stereo effects while playing "test.gbs" using Gbs_Emu,
// and record to "out.wav".

#include "Gbs_Emu.h"
#include "Wave_Writer.hpp"
#include "Effects_Buffer.h"

#include <stdlib.h>
#include <stdio.h>

static void exit_if_error( const char* str )
{
	if ( str ) {
		fprintf( stderr, "Error: %s\n", str );
		exit( EXIT_FAILURE );
	}
}

int main()
{
	const long sample_rate = 44100;
	
	// Create effects buffer with 1/30 second length
	Effects_Buffer buf;
	exit_if_error( buf.sample_rate( sample_rate, 1000 / 30 ) );
	
	// Create emulator and output to effects buffer
	Gbs_Emu* emu = new Gbs_Emu;
	if ( !emu )
		exit_if_error( "Out of memory" );
	exit_if_error( emu->init( &buf ) );
	
	// Load file
	Emu_Std_Reader reader;
	exit_if_error( reader.open( "test.gbs" ) );
	Gbs_Emu::header_t header;
	exit_if_error( reader.read( &header, sizeof header ) );
	exit_if_error( emu->load( header, reader ) );
	
	// Configure effects buffer
	Effects_Buffer::config_t cfg;
	cfg.pan_1 = -0.12;          // put first two oscillators slightly off-center
	cfg.pan_2 = 0.12;
	cfg.reverb_delay = 88;      // delays are in milliseconds
	cfg.reverb_level = 0.20;    // significant reverb
	cfg.echo_delay = 61;        // echo applies to noise and percussion oscillators
	cfg.echo_level = 0.15;
	cfg.delay_variance = 18;    // left/right delays must differ for stereo effect
	cfg.effects_enabled = true;
	buf.config( cfg );
	
	// Record first track for several seconds
	exit_if_error( emu->start_track( 0 ) );
	Wave_Writer wave( sample_rate, "out.wav" );
	wave.stereo( true );
	while ( wave.sample_count() < 2 * sample_rate * 10 )
	{
		const long buf_size = 1024; // must be even
		Music_Emu::sample_t buf [buf_size];
		
		exit_if_error( emu->play( buf_size, buf ) );
		
		wave.write( buf, buf_size );
	}
	
	delete emu;
	
	return 0;
}

