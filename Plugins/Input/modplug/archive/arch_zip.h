/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#ifndef __MODPLUG_ARCH_ZIP_H__INCLUDED__
#define __MODPLUG_ARCH_ZIP_H__INCLUDED__

#include "archive.h"
#include <string>

class arch_Zip: public Archive
{
public:
	arch_Zip(const string& aFileName);
	virtual ~arch_Zip();
	
	static bool ContainsMod(const string& aFileName);
	static bool processLine(char *buffer, uint32 *mSize, char *filename);
};

#endif
