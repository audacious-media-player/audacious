/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#ifndef __MODPLUG_ARCH_GZIP_H__INCLUDED__
#define __MODPLUG_ARCH_GZIP_H__INCLUDED__

#include "archive.h"
#include <string>

class arch_Gzip: public Archive
{
public:
	arch_Gzip(const string& aFileName);
	virtual ~arch_Gzip();
	
	static bool ContainsMod(const string& aFileName);
};

#endif
