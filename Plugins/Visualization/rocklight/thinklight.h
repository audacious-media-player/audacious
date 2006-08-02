/***************************************************************************
 *            thinklight.h
 *
 *  Sun Dec 26 17:01:00 2004
 *  Copyright  2004  Benedikt 'Hunz' Heinz
 *  rocklight@hunz.org
 *  $Id: thinklight.h,v 1.2 2005/03/26 21:29:17 hunz Exp $
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#ifndef _THINKLIGHT_H
#define _THINKLIGHT_H

int thinklight_open(void);
void thinklight_close(int fd);
int thinklight_set(int fd, int state);
int thinklight_get(int fd);

#endif /* _THINKLIGHT_H */
