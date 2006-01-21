
#include "abstract_file.h"

#include <assert.h>
#include <string.h>
#include <stddef.h>

/* Copyright (C) 2005 by Shay Green. Permission is hereby granted, free of
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

#ifndef RAISE_ERROR
	#define RAISE_ERROR( str ) return str
#endif

typedef Data_Reader::error_t error_t;

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

long File_Reader::remain() const
{
	return size() - tell();
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

Std_File_Reader::Std_File_Reader() : file( NULL ) {
}

Std_File_Reader::~Std_File_Reader() {
	close();
}

error_t Std_File_Reader::open( const char* path )
{
	file = vfs_fopen( path, "rb" );
	if ( !file )
		RAISE_ERROR( "Couldn't open file" );
	return NULL;
}

long Std_File_Reader::size() const
{
	long pos = tell();
	vfs_fseek( file, 0, SEEK_END );
	long result = tell();
	vfs_fseek( file, pos, SEEK_SET );
	return result;
}

long Std_File_Reader::read_avail( void* p, long s ) {
	return vfs_fread( p, 1, s, file );
}

long Std_File_Reader::tell() const {
	return vfs_ftell( file );
}

error_t Std_File_Reader::seek( long n )
{
	if ( vfs_fseek( file, n, SEEK_SET ) != 0 )
		RAISE_ERROR( "Error seeking in file" );
	return NULL;
}

void Std_File_Reader::close()
{
	if ( file ) {
		vfs_fclose( file );
		file = NULL;
	}
}

// Std_File_Writer

Std_File_Writer::Std_File_Writer() : file( NULL ) {
}

Std_File_Writer::~Std_File_Writer() {
	close();
}

error_t Std_File_Writer::open( const char* path )
{
	file = vfs_fopen( path, "wb" );
	if ( !file )
		RAISE_ERROR( "Couldn't open file for writing" );
		
	// to do: increase file buffer size
	//setvbuf( file, NULL, _IOFBF, 32 * 1024L );
	
	return NULL;
}

error_t Std_File_Writer::write( const void* p, long s )
{
	long result = vfs_fwrite( p, 1, s, file );
	if ( result != s )
		RAISE_ERROR( "Couldn't write to file" );
	return NULL;
}

void Std_File_Writer::close()
{
	if ( file ) {
		vfs_fclose( file );
		file = NULL;
	}
}

// Mem_Writer

Mem_Writer::Mem_Writer( void* p, long s, int b ) :
	out( p ),
	remain_( s ),
	ignore_excess( b )
{
}

error_t Mem_Writer::write( const void* p, long s )
{
	if ( s > remain_ )
	{
		if ( !ignore_excess )
			RAISE_ERROR( "Tried to write more data than expected" );
		s = remain_;
	}
	remain_ -= s;
	memcpy( out, p, s );
	out = (char*) out + s;
	return NULL;
}

long Mem_Writer::remain() const {
	return remain_;
}

// Null_Writer

error_t Null_Writer::write( const void*, long ) {
	return NULL;
}

