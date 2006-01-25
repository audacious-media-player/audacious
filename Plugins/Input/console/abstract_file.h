
// Abstract file access interfaces

#ifndef ABSTRACT_FILE_H
#define ABSTRACT_FILE_H

#include <stdio.h>

// Supports reading and finding out how many bytes are remaining
class Data_Reader {
public:
	Data_Reader() { }
	virtual ~Data_Reader() { }
	
	// NULL on success, otherwise error string
	typedef const char* error_t;
	
	// Read at most 'n' bytes. Return number of bytes read, zero or negative
	// if error.
	virtual long read_avail( void*, long n ) = 0;
	
	// Read exactly 'n' bytes (error if fewer are available).
	virtual error_t read( void*, long );
	
	// Number of bytes remaining
	virtual long remain() const = 0;
	
	// Skip forwards by 'n' bytes.
	virtual error_t skip( long n );
	
	// to do: bytes remaining = LONG_MAX when unknown?
	
private:
	// noncopyable
	Data_Reader( const Data_Reader& );
	Data_Reader& operator = ( const Data_Reader& );
};

// Adds seeking operations
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

// Limit access to a subset of data
class Subset_Reader : public Data_Reader {
	Data_Reader* in;
	long remain_;
public:
	Subset_Reader( Data_Reader*, long size );
	long remain() const;
	long read_avail( void*, long );
};

// Treat range of memory as a file
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

// File reader based on C FILE
class Std_File_Reader : public File_Reader {
	FILE* file_;
protected:
	void reset( FILE* f ) { file_ = f; }
	//FILE* owned_file;
public:
	Std_File_Reader();
	~Std_File_Reader();
	
	error_t open( const char* );
	
	FILE* file() const { return file_; }
	
	// Forward read requests to file. Caller must close file later.
	//void forward( FILE* );
	
	long size() const;
	long read_avail( void*, long );
	
	long tell() const;
	error_t seek( long );
	
	void close();
};

// Supports writing
class Data_Writer {
public:
	Data_Writer() { }
	virtual ~Data_Writer() { }
	
	typedef const char* error_t;
	
	// Write 'n' bytes. NULL on success, otherwise error string.
	virtual error_t write( const void*, long n ) = 0;
	
	void satisfy_lame_linker_();
private:
	// noncopyable
	Data_Writer( const Data_Writer& );
	Data_Writer& operator = ( const Data_Writer& );
};

class Std_File_Writer : public Data_Writer {
	FILE* file_;
protected:
	void reset( FILE* f ) { file_ = f; }
public:
	Std_File_Writer();
	~Std_File_Writer();
	
	error_t open( const char* );
	
	FILE* file() const { return file_; }
	
	// Forward writes to file. Caller must close file later.
	//void forward( FILE* );
	
	error_t write( const void*, long );
	
	void close();
};

// Write data to memory
class Mem_Writer : public Data_Writer {
	char* data_;
	long size_;
	long allocated;
	enum { expanding, fixed, ignore_excess } mode;
public:
	// Keep all written data in expanding block of memory
	Mem_Writer();
	
	// Write to fixed-size block of memory. If ignore_excess is false, returns
	// error if more than 'size' data is written, otherwise ignores any excess.
	Mem_Writer( void*, long size, int ignore_excess = 0 );
	
	error_t write( const void*, long );
	
	// Pointer to beginning of written data
	char* data() { return data_; }
	
	// Number of bytes written
	long size() const { return size_; }
	
	~Mem_Writer();
};

// Written data is ignored
class Null_Writer : public Data_Writer {
public:
	error_t write( const void*, long );
};

#endif

