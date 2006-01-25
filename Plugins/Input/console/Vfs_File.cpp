
#include "Vfs_File.h"

#include "libaudacious/vfs.h"

Vfs_File_Reader::Vfs_File_Reader() : file_( NULL ) { }

Vfs_File_Reader::~Vfs_File_Reader() { close(); }

Vfs_File_Reader::error_t Vfs_File_Reader::open( const char* path )
{
	file_ = vfs_fopen( path, "rb" );
	if ( !file_ )
		return "Couldn't open file";
	return 0;
}

long Vfs_File_Reader::size() const
{
	long pos = tell();
	vfs_fseek( (VFSFile*) file_, 0, SEEK_END );
	long result = tell();
	vfs_fseek( (VFSFile*) file_, pos, SEEK_SET );
	return result;
}

long Vfs_File_Reader::read_avail( void* p, long s )
{
	return (long) vfs_fread( p, 1, s, (VFSFile*) file_ );
}

long Vfs_File_Reader::tell() const
{
	return vfs_ftell( (VFSFile*) file_ );
}

Vfs_File_Reader::error_t Vfs_File_Reader::seek( long n )
{
	if ( n == 0 ) // optimization
		vfs_rewind( (VFSFile*) file_ );
	else if ( vfs_fseek( (VFSFile*) file_, n, SEEK_SET ) != 0 )
		return "Error seeking in file";
	return 0;
}

void Vfs_File_Reader::close()
{
	if ( file_ )
	{
		vfs_fclose( (VFSFile*) file_ );
		file_ = 0;
	}
}

