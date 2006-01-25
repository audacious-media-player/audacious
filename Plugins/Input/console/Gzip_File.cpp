
#include "Gzip_File.h"

#include "zlib.h"

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

static const char* get_gzip_eof( FILE* file, long* eof )
{
	unsigned char buf [4];
	if ( !fread( buf, 2, 1, file ) )
		return "Couldn't read from file";
	
	if ( buf [0] == 0x1F && buf [1] == 0x8B )
	{
		if ( fseek( file, -4, SEEK_END ) )
			return "Couldn't seek in file";
		
		if ( !fread( buf, 4, 1, file ) )
			return "Couldn't read from file";
		
		*eof = buf [3] * 0x1000000L + buf [2] * 0x10000L + buf [1] * 0x100L + buf [0];
	}
	else
	{
		if ( fseek( file, 0, SEEK_END ) )
			return "Couldn't seek in file";
		
		*eof = ftell( file );
	}
	
	return NULL;
}

const char* get_gzip_eof( const char* path, long* eof )
{
	FILE* file = fopen( path, "rb" );
	if ( !file )
		return "Couldn't open file";
	const char* error = get_gzip_eof( file, eof );
	fclose( file );
	return error;
}

// Gzip_File_Reader

Gzip_File_Reader::Gzip_File_Reader() : file_( NULL )
{
}

Gzip_File_Reader::~Gzip_File_Reader()
{
	close();
}

Gzip_File_Reader::error_t Gzip_File_Reader::open( const char* path )
{
	error_t error = get_gzip_eof( path, &size_ );
	if ( error )
		return error;
	
	file_ = gzopen( path, "rb" );
	if ( !file_ )
		return "Couldn't open file";
	
	return NULL;
}

long Gzip_File_Reader::size() const
{
	return size_;
}

long Gzip_File_Reader::read_avail( void* p, long s )
{
	return (long) gzread( file_, p, s );
}

long Gzip_File_Reader::tell() const
{
	return gztell( file_ );
}

Gzip_File_Reader::error_t Gzip_File_Reader::seek( long n )
{
	if ( gzseek( file_, n, SEEK_SET ) < 0 )
		return "Error seeking in file";
	return NULL;
}

void Gzip_File_Reader::close()
{
	if ( file_ )
	{
		gzclose( file_ );
		file_ = NULL;
	}
}

// Gzip_File_Writer

Gzip_File_Writer::Gzip_File_Writer() : file_( NULL )
{
}

Gzip_File_Writer::~Gzip_File_Writer()
{
	close();
}

Gzip_File_Writer::error_t Gzip_File_Writer::open( const char* path )
{
	file_ = gzopen( path, "wb9" );
	if ( !file_ )
		return "Couldn't open file for writing";
	
	return NULL;
}

Gzip_File_Writer::error_t Gzip_File_Writer::write( const void* p, long s )
{
	long result = (long) gzwrite( file_ , (void*) p, s );
	if ( result != s )
		return "Couldn't write to file";
	return NULL;
}

void Gzip_File_Writer::close()
{
	if ( file_ )
	{
		gzclose( file_ );
		file_ = NULL;
	}
}
