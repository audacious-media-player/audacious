/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#ifndef __MODPLUG_ARCH_RAR_H__INCLUDED__
#define __MODPLUG_ARCH_RAR_H__INCLUDED__

#include "archive.h"
#include <string>

class arch_Rar: public Archive
{
public:
	arch_Rar(const string& aFileName);
	virtual ~arch_Rar();
	
	static bool ContainsMod(const string& aFileName);
};

#endif
