/* Paranormal - A highly customizable audio visualization library
 * Copyright (C) 2001  Jamie Gennis <jgennis@mindspring.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __PN_XML_H__
#define __PN_XML_H__

#include <libxml/parser.h>

/* compatibility w/ libxml1 */
#ifndef xmlChildrenNode
#define xmlChildrenNode childs
#define xmlRootNode root
#endif

/* The namespace href */
#define PN_XML_NS_HREF "http://paranormal.sf.net/pnvis/1.0/"

#endif /* __PN_XML_H__ */
