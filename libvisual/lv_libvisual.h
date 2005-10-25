/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_LIBVISUAL_H
#define _LV_LIBVISUAL_H

#include <libvisual/lv_param.h>
#include <libvisual/lv_ui.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

const char *visual_get_version (void);
VisParamContainer *visual_get_params (void);
VisUIWidget *visual_get_userinterface (void);
int visual_init_path_add (char *pathadd);
int visual_init (int *argc, char ***argv);
int visual_is_initialized (void);
int visual_quit (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LV_LIBVISUAL_H */
