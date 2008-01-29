/* Scale & Dither library (libSAD)
 * High-precision bit depth converter with ReplayGain support
 *
 * (c)2007 by Eugene Zagidullin (e.asphyx@gmail.com)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef BUFFER_H
#define BUFFER_H

#include "common.h"
#include "dither.h"

SAD_buffer_ops* SAD_assign_buf_ops (SAD_buffer_format *format);

#endif /*BUFFER_H*/
