/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

// stddefs.h: Standard defs to streamline coding style.
//
//////////////////////////////////////////////////////////////////////

#if !defined(__MODPLUGXMMS_STDDEFS_H__INCLUDED__)
#define __MODPLUGXMMS_STDDEFS_H__INCLUDED__

//Invalid pointer
#ifndef NULL
#define NULL 0
#endif

//Standard types. ----------------------------------------
//These data types are provided because the standard types vary across
// platforms.  For example, long is 64-bit on some 64-bit systems.
//u = unsigned
//# = size in bits
typedef unsigned char          uchar;

typedef char                   int8;
typedef short                  int16;
typedef int                    int32;
typedef long long              int64;

typedef unsigned char          uint8;
typedef unsigned short         uint16;
typedef unsigned int           uint32;
typedef unsigned long long     uint64;

typedef float                  float32;
typedef double                 float64;
typedef long double            float128;

#endif // included
