
// File_Reader based on a VFSFile

#ifndef VFS_FILE_H
#define VFS_FILE_H

#include "abstract_file.h"

class Vfs_File_Reader : public File_Reader {
	void* file_;
public:
	Vfs_File_Reader();
	~Vfs_File_Reader();
	error_t open( const char* );
	long size() const;
	long read_avail( void*, long );
	long tell() const;
	error_t seek( long );
	void close();
};

#endif

