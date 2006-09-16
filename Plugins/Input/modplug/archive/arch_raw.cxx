/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

//open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//mmap()
#include <unistd.h>
#include <sys/mman.h>

#include "arch_raw.h"

arch_Raw::arch_Raw(const string& aFileName)
{
	mFileDesc = open(aFileName.c_str(), O_RDONLY);

	struct stat lStat;

	//open and mmap the file
	if(mFileDesc == -1)
	{
		mSize = 0;
		return;
	}
	fstat(mFileDesc, &lStat);
	mSize = lStat.st_size;

	mMap =
		(uchar*)mmap(0, mSize, PROT_READ,
		MAP_PRIVATE, mFileDesc, 0);
	if(!mMap)
	{
		close(mFileDesc);
		mSize = 0;
		return;
	}
}

arch_Raw::~arch_Raw()
{
	if(mSize != 0)
	{
		munmap(mMap, mSize);
		close(mFileDesc);
	}
}

bool arch_Raw::ContainsMod(const string& aFileName)
{
	return IsOurFile(aFileName);
}
