/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#ifndef __MODPLUG_ARCH_RAW_H__INCLUDED__
#define __MODPLUG_ARCH_RAW_H__INCLUDED__

#include "archive.h"
#include <string>

class arch_Raw: public Archive
{
	int mFileDesc;

public:
	arch_Raw(const string& aFileName);
	virtual ~arch_Raw();
	
	static bool ContainsMod(const string& aFileName);
};

#endif
