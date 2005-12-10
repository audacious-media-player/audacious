/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#ifndef __MODPLUG_ARCHIVE_OPEN_H__INCLUDED__
#define __MODPLUG_ARCHIVE_OPEN_H__INCLUDED__

#include "archive.h"
#include <string>

Archive* OpenArchive(const string& aFileName);
bool ContainsMod(const string& aFileName);

#endif
