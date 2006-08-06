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

#ifndef __PN_ACTUATOR_FACTORY_H__
#define __PN_ACTUATOR_FACTORY_H__

#include "pnactuator.h"

void               pn_actuator_factory_init                             (void);
void               pn_actuator_factory_register_actuator                (const gchar *name,
									 GType type);
void               pn_actuator_factory_unregister_actuator              (const gchar *name);
gboolean           pn_actuator_factory_is_registered                    (const gchar *name);
PnActuator        *pn_actuator_factory_new_actuator                     (const gchar *name);
PnActuator        *pn_actuator_factory_new_actuator_from_xml            (xmlNodePtr node);


#endif /* __PN_ACTUATOR_FACTORY_H__ */
