/* 
 * Copyright (C) 2000-2001 major mms
 * 
 * This file is part of libmms
 * 
 * xine-mms is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine-mms is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * libmms public header
 */
#ifndef HAVE_MMS_H
#define HAVE_MMS_H

typedef struct mms_s mms_t;

mms_t *mms_connect (const char *url);

int mms_read (mms_t *this, char *data, int len);

void mms_close (mms_t *this);

#endif

