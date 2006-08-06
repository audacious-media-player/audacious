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

#ifndef __PN_ACTUATOR_LIST_H__
#define __PN_ACTUATOR_LIST_H__

#include "pncontainer.h"


G_BEGIN_DECLS


enum
{
  PN_ACTUATOR_LIST_OPT_LAST = PN_CONTAINER_OPT_LAST
};

#define PN_TYPE_ACTUATOR_LIST              (pn_actuator_list_get_type ())
#define PN_ACTUATOR_LIST(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_ACTUATOR_LIST, PnActuatorList))
#define PN_ACTUATOR_LIST_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_ACTUATOR_LIST, PnActuatorListClass))
#define PN_IS_ACTUATOR_LIST(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_ACTUATOR_LIST))
#define PN_IS_ACTUATOR_LIST_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_ACTUATOR_LIST))
#define PN_ACTUATOR_LIST_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_ACTUATOR_LIST, PnActuatorListClass))

#define PN_ACTUATOR_LIST_ACTUATORS(obj)    (PN_ACTUATOR_LIST (obj)->actuators)

typedef struct _PnActuatorList        PnActuatorList;
typedef struct _PnActuatorListClass   PnActuatorListClass;

struct _PnActuatorList
{
  PnContainer parent;
};

struct _PnActuatorListClass
{
  PnContainerClass parent_class;
};

/* Creators */
GType                 pn_actuator_list_get_type                (void);
PnActuatorList       *pn_actuator_list_new                     (void);

#endif /* __PN_ACTUATOR_LIST_H__ */
