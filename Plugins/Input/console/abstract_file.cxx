
#include "abstract_file.h"

#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

/* Copyright (C) 2005 Shay Green. Permission is hereby granted, free of
charge, to any person obtaining a copy of this software module and associated
documentation files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the
following conditions: The above copyright notice and this permission notice
shall be included in all copies or substantial portions of the Software. THE
SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

// to do: remove?
#ifndef RAISE_ERROR
	#define RAISE_ERROR( str ) return str
#endif

typedef Data_Reader::error_t error_t;

error_t Data_Writer::write( const void*, long ) { return NULL; }

void Data_Writer::satisfy_lame_linker_() { }

error_t Data_Reader::read( void* p, long s )
{
	long result = read_avail( p, s );
	if ( result != s )
	{
		if ( result >= 0 && result < s )
			RAISE_ERROR( "Unexpected end-of-file" );
		
		RAISE_ERROR( "Read error" );
	}
	
	return NULL;
}

error_t Data_Reader::skip( long count )
{
	char buf [512];
	while ( count )
	{
		int n = sizeof buf;
		if ( n > count )
			n = count;
		count -= n;
		RAISE_ERROR( read( buf, n ) );
	}
	return NULL;
}

long File_Reader::remain() const
{
	return size() - tell();
}

error_t File_Reader::skip( long n )
{
	assert( n >= 0 );
	if ( n )
		RAISE_ERROR( seek( tell() + n ) );
	
	return NULL;
}


// Subset_Reader

Subset_Reader::Subset_Reader( Data_Reader* in_, long size ) :
	in( in_ ),
	remain_( in_->remain() )
{
	if ( remain_ > size )
		remain_ = size;
}

long Subset_Reader::remain() const {
	return remain_;
}

long Subset_Reader::read_avail( void* p, long s )
{
	if ( s > remain_ )
		s = remain_;
	remain_ -= s;
	return in->read_avail( p, s );
}

// Mem_File_Reader

Mem_File_Reader::Mem_File_Reader( const void* p, long s ) :
	begin( (const char*) p ),
	pos( 0 ),
	size_( s )
{
}
	
long Mem_File_Reader::size() const {
	return size_;
}

long Mem_File_Reader::read_avail( void* p, long s )
{
	long r = remain();
	if ( s > r )
		s = r;
	memcpy( p, begin + pos, s );
	pos += s;
	return s;
}

long Mem_File_Reader::tell() const {
	return pos;
}

error_t Mem_File_Reader::seek( long n )
{
	if ( n > size_ )
		RAISE_ERROR( "Tried to go past end of file" );
	pos = n;
	return NULL;
}

// Std_File_Reader

Std_File_Reader::Std_File_Reader() : file_( NULL ) {
}

Std_File_Reader::~Std_File_Reader() {
	close();
}

error_t Std_File_Reader::open( const char* path )
{
	file_ = fopen( path, "rb" );
	if ( !file_ )
		RAISE_ERROR( "Couldn't open file" );
	return NULL;
}

long Std_File_Reader::size() const
{
	long pos = tell();
	fseek( file_, 0, SEEK_END );
	long result = tell();
	fseek( file_, pos, SEEK_SET );
	return result;
}

long Std_File_Reader::read_avail( void* p, long s ) {
	return (long) fread( p, 1, s, file_ );
}

long Std_File_Reader::tell() const {
	return ftell( file_ );
}

error_t Std_File_Reader::seek( long n )
{
	if ( fseek( file_, n, SEEK_SET ) != 0 )
		RAISE_ERROR( "Error seeking in file" );
	return NULL;
}

void Std_File_Reader::close()
{
	if ( file_ ) {
		fclose( file_ );
		file_ = NULL;
	}
}

// Std_File_Writer

Std_File_Writer::Std_File_Writer() : file_( NULL ) {
}

Std_File_Writer::~Std_File_Writer() {
	close();
}

error_t Std_File_Writer::open( const char* path )
{
	file_ = fopen( path, "wb" );
	if ( !file_ )
		RAISE_ERROR( "Couldn't open file for writing" );
		
	// to do: increase file buffer size
	//setvbuf( file_, NULL, _IOFBF, 32 * 1024L );
	
	return NULL;
}

error_t Std_File_Writer::write( const void* p, long s )
{
	long result = (long) fwrite( p, 1, s, file_ );
	if ( result != s )
		RAISE_ERROR( "Couldn't write to file" );
	return NULL;
}

void Std_File_Writer::close()
{
	if ( file_ ) {
		fclose( file_ );
		file_ = NULL;
	}
}

// Mem_Writer

Mem_Writer::Mem_Writer( void* p, long s, int b )
{
	data_ = (char*) p;
	size_ = 0;
	allocated = s;
	mode = b ? ignore_excess : fixed;
}

Mem_Writer::Mem_Writer()
{
	data_ = NULL;
	size_ = 0;
	allocated = 0;
	mode = expanding;
}

Mem_Writer::~Mem_Writer()
{
	if ( mode == expanding )
		free( data_ );
}

error_t Mem_Writer::write( const void* p, long s )
{
	long remain = allocated - size_;
	if ( s > remain )
	{
		if ( mode == fixed )
			RAISE_ERROR( "Tried to write more data than expected" );
		
		if ( mode == ignore_excess )
		{
			s = remain;
		}
		else // expanding
		{
			long new_allocated = size_ + s;
			new_allocated += (new_allocated >> 1) + 2048;
			void* p = realloc( data_, new_allocated );
			if ( !p )
				RAISE_ERROR( "Out of memory" );
			data_ = (char*) p;
			allocated = new_allocated;
		}
	}
	
	assert( size_ + s <= allocated );
	memcpy( data_ + size_, p, s );
	size_ += s;
	
	return NULL;
}

// Null_Writer

error_t Null_Writer::write( const void*, long )
{
	return NULL;
}

