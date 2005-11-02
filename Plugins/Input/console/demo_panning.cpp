
// Use Panning_Buffer to add left/right panning while playing "test.vgm" using Vgm_Emu,
// and record to "out.wav".

#include "Vgm_Emu.h"
#include "Wave_Writer.hpp"
#include "Panning_Buffer.h"

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
	
	// Create panning buffer with 1/30 second length
	Panning_Buffer buf;
	exit_if_error( buf.sample_rate( sample_rate, 1000 / 30 ) );
	
	// Prepare emulator with output set to panning buffer
	Vgm_Emu* emu = new Vgm_Emu;
	if ( !emu )
		exit_if_error( "Out of memory" );
	exit_if_error( emu->init( &buf ) );
	
	// Load file
	Emu_Std_Reader reader;
	exit_if_error( reader.open( "test.vgm" ) );
	Vgm_Emu::header_t header;
	exit_if_error( reader.read( &header, sizeof header ) );
	exit_if_error( emu->load( header, reader ) );
	
	// Configure panning buffer
	buf.set_pan( 0, 1.40, 0.60 );  // pulse 1 - left
	buf.set_pan( 1, 1.00, 1.00 );  // pulse 2 - center
	buf.set_pan( 2, 0.40, 1.60 );  // pulse 3 - right
	buf.set_pan( 3, 1.00,-1.00 );  // noise   - "surround" (phase-inverted left/right)
	
	// Record first track for several seconds
	exit_if_error( emu->start_track( 0 ) );
	Wave_Writer wave( sample_rate, "out.wav" );
	wave.stereo( true );
	while ( wave.sample_count() < 2 * sample_rate * 10 )
	{
		const long buf_size = 1234; // can be any size
		Music_Emu::sample_t buf [buf_size];
		
		// fill buffer
		exit_if_error( emu->play( buf_size, buf ) );
		
		// write to sound file
		wave.write( buf, buf_size );
	}
	
	delete emu;
	
	return 0;
}

