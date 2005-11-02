
// Abstract file access interfaces

// Copyright (C) 2005 Shay Green. MIT license.

#ifndef ABSTRACT_FILE_H
#define ABSTRACT_FILE_H

#include <stdio.h>

// to do: built-in buffering?

class Data_Reader {
	// noncopyable
	Data_Reader( const Data_Reader& );
	Data_Reader& operator = ( const Data_Reader& );
public:
	Data_Reader() { }
	virtual ~Data_Reader() { }
	
	// NULL on success, otherwise error string
	typedef const char* error_t;
	
	// Read at most 'n' bytes. Return number of bytes read, negative
	// value if error.
	virtual long read_avail( void*, long n ) = 0;
	
	// Read exactly 'n' bytes (error if fewer are available). NULL on success,
	// otherwise error string.
	virtual error_t read( void*, long );
	
	// Number of bytes remaining
	virtual long remain() const = 0;
	
	// Skip forwards by 'n' bytes.
	virtual error_t skip( long n );
	
	// to do: bytes remaining = LONG_MAX when unknown?
};

class File_Reader : public Data_Reader {
public:
	// Size of file
	virtual long size() const = 0;
	
	// Current position in file
	virtual long tell() const = 0;
	
	// Change position in file
	virtual error_t seek( long ) = 0;
	
	virtual long remain() const;
	
	error_t skip( long n );
};

class Subset_Reader : public Data_Reader {
	Data_Reader* in;
	long remain_;
public:
	Subset_Reader( Data_Reader*, long size );
	long remain() const;
	long read_avail( void*, long );
};

class Mem_File_Reader : public File_Reader {
	const char* const begin;
	long pos;
	const long size_;
public:
	Mem_File_Reader( const void*, long size );
	
	long size() const;
	long read_avail( void*, long );
	
	long tell() const;
	error_t seek( long );
};

class Std_File_Reader : public File_Reader {
	FILE* file;
	//FILE* owned_file;
public:
	Std_File_Reader();
	~Std_File_Reader();
	
	error_t open( const char* );
	
	// Forward read requests to file. Caller must close file later.
	//void forward( FILE* );
	
	long size() const;
	long read_avail( void*, long );
	
	long tell() const;
	error_t seek( long );
	
	void close();
};

class Data_Writer {
	// noncopyable
	Data_Writer( const Data_Writer& );
	Data_Writer& operator = ( const Data_Writer& );
public:
	Data_Writer() { }
	virtual ~Data_Writer() { }
	
	typedef const char* error_t;
	
	// Write 'n' bytes. NULL on success, otherwise error string.
	virtual error_t write( const void*, long n ) = 0;
};

class Null_Writer : public Data_Writer {
public:
	error_t write( const void*, long );
};

class Std_File_Writer : public Data_Writer {
	FILE* file;
public:
	Std_File_Writer();
	~Std_File_Writer();
	
	error_t open( const char* );
	
	// Forward writes to file. Caller must close file later.
	//void forward( FILE* );
	
	error_t write( const void*, long );
	
	void close();
};

// to do: mem file writer

// Write to block of memory
class Mem_Writer : public Data_Writer {
	void* out;
	long remain_;
	int ignore_excess;
public:
	// to do: automatic allocation and expansion of memory?
	Mem_Writer( void*, long size, int ignore_excess = 1 );
	error_t write( const void*, long );
	long remain() const;
};

#endif

