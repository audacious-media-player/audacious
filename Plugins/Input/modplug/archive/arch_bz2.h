/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *          Colin DeVilbiss <crdevilb@mtu.edu>
 *
 * This source code is public domain.
 */

// BZ2 support added by Colin DeVilbiss <crdevilb@mtu.edu>

#ifndef __MODPLUG_ARCH_BZIP2_H__INCLUDED__
#define __MODPLUG_ARCH_BZIP2_H__INCLUDED__

#include "archive.h"
#include <string>

class arch_Bzip2: public Archive
{
public:
	arch_Bzip2(const string& aFileName);
	virtual ~arch_Bzip2();
	
	static bool ContainsMod(const string& aFileName);
};

#endif
