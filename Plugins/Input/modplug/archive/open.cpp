/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#include "open.h"
#include "arch_raw.h"
#include "arch_gzip.h"
#include "arch_zip.h"
#include "arch_rar.h"
#include "arch_bz2.h"

Archive* OpenArchive(const string& aFileName)
{
	string lExt;
	uint32 lPos;

	lPos = aFileName.find_last_of('.');
	if(lPos < 0)
		return NULL;
	lExt = aFileName.substr(lPos);
	for(uint32 i = 0; i < lExt.length(); i++)
		lExt[i] = tolower(lExt[i]);

	if (lExt == ".mdz")
		return new arch_Zip(aFileName);
	if (lExt == ".mdr")
		return new arch_Rar(aFileName);
	if (lExt == ".mdgz")
		return new arch_Gzip(aFileName);
	if (lExt == ".mdbz")
		return new arch_Bzip2(aFileName);
	if (lExt == ".s3z")
		return new arch_Zip(aFileName);
	if (lExt == ".s3r")
		return new arch_Rar(aFileName);
	if (lExt == ".s3gz")
		return new arch_Gzip(aFileName);
	if (lExt == ".xmz")
		return new arch_Zip(aFileName);
	if (lExt == ".xmr")
		return new arch_Rar(aFileName);
	if (lExt == ".xmgz")
		return new arch_Gzip(aFileName);
	if (lExt == ".itz")
		return new arch_Zip(aFileName);
	if (lExt == ".itr")
		return new arch_Rar(aFileName);
	if (lExt == ".itgz")
		return new arch_Gzip(aFileName);
	if (lExt == ".zip")
		return new arch_Zip(aFileName);
	if (lExt == ".rar")
		return new arch_Rar(aFileName);
	if (lExt == ".gz")
		return new arch_Gzip(aFileName);
	if (lExt == ".bz2")
		return new arch_Bzip2(aFileName);

	return new arch_Raw(aFileName);
}

bool ContainsMod(const string& aFileName)
{
	string lExt;
	uint32 lPos;

	lPos = aFileName.find_last_of('.');
	if(lPos < 0)
		return false;
	lExt = aFileName.substr(lPos);
	for(uint32 i = 0; i < lExt.length(); i++)
		lExt[i] = tolower(lExt[i]);

	if (lExt == ".mdz")
		return arch_Zip::ContainsMod(aFileName);
	if (lExt == ".mdr")
		return arch_Rar::ContainsMod(aFileName);
	if (lExt == ".mdgz")
		return arch_Gzip::ContainsMod(aFileName);
	if (lExt == ".mdbz")
		return arch_Bzip2::ContainsMod(aFileName);
	if (lExt == ".s3z")
		return arch_Zip::ContainsMod(aFileName);
	if (lExt == ".s3r")
		return arch_Rar::ContainsMod(aFileName);
	if (lExt == ".s3gz")
		return arch_Gzip::ContainsMod(aFileName);
	if (lExt == ".xmz")
		return arch_Zip::ContainsMod(aFileName);
	if (lExt == ".xmr")
		return arch_Rar::ContainsMod(aFileName);
	if (lExt == ".xmgz")
		return arch_Gzip::ContainsMod(aFileName);
	if (lExt == ".itz")
		return arch_Zip::ContainsMod(aFileName);
	if (lExt == ".itr")
		return arch_Rar::ContainsMod(aFileName);
	if (lExt == ".itgz")
		return arch_Gzip::ContainsMod(aFileName);
	if (lExt == ".zip")
		return arch_Zip::ContainsMod(aFileName);
	if (lExt == ".rar")
		return arch_Rar::ContainsMod(aFileName);
	if (lExt == ".gz")
		return arch_Gzip::ContainsMod(aFileName);
	if (lExt == ".bz2")
		return arch_Bzip2::ContainsMod(aFileName);

	return arch_Raw::ContainsMod(aFileName);
}
