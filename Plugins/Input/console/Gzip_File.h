
// Gzip file access

#ifndef GZIP_FILE_H
#define GZIP_FILE_H

#include "abstract_file.h"

// Get size of gzipped file data (or size of file if not gzipped). NULL
// on success, otherwise error string.
const char* get_gzip_eof( const char* path, long* eof_out );

class Gzip_File_Reader : public File_Reader {
	void* file_;
	long size_;
public:
	Gzip_File_Reader();
	~Gzip_File_Reader();
	
	error_t open( const char* );
	
	long size() const;
	long read_avail( void*, long );
	
	long tell() const;
	error_t seek( long );
	
	void close();
};

class Gzip_File_Writer : public Data_Writer {
	void* file_;
public:
	Gzip_File_Writer();
	~Gzip_File_Writer();
	
	error_t open( const char* );
	error_t write( const void*, long );
	void close();
};

#endif

