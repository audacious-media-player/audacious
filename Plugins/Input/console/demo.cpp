
// Play "test.nsf" using Nsf_Emu and record to "out.wav".

#include "Nsf_Emu.h"
#include "Wave_Writer.hpp"

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
	
	// Prepare emulator
	Nsf_Emu* emu = new Nsf_Emu;
	if ( !emu )
		exit_if_error( "Out of memory" );
	exit_if_error( emu->init( sample_rate ) );
	
	// Load file
	Emu_Std_Reader reader;
	exit_if_error( reader.open( "test.nsf" ) );
	Nsf_Emu::header_t header;
	exit_if_error( reader.read( &header, sizeof header ) );
	exit_if_error( emu->load( header, reader ) );
	
	// Print game and song info
	printf( "Game     : %-32s\n", header.game      ? (char*) header.game : "" );
	printf( "Song     : %-32s\n", header.song      ? (char*) header.song : "" );
	printf( "Author   : %-32s\n", header.author    ? (char*) header.author : "" );
	printf( "Copyright: %-32s\n", header.copyright ? (char*) header.copyright : "" );
	printf( "Tracks   : %d\n", emu->track_count() );
	printf( "\n" );
	
	// Record first track for several seconds
	exit_if_error( emu->start_track( 0 ) );
	Wave_Writer wave( sample_rate, "out.wav" );
	wave.stereo( true );
	while ( wave.sample_count() < 2 * sample_rate * 10 )
	{
		const long buf_size = 1024; // must be even
		Music_Emu::sample_t buf [buf_size];
		
		// fill buffer
		exit_if_error( emu->play( buf_size, buf ) );
		
		// write to sound file
		wave.write( buf, buf_size );
	}
	
	delete emu;
	
	return 0;
}

