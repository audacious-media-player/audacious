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

#ifndef __PN_CONTAINER_H__
#define __PN_CONTAINER_H__

#include "pnactuator.h"


G_BEGIN_DECLS


enum
{
  PN_CONTAINER_OPT_LAST = PN_ACTUATOR_OPT_LAST
};

typedef enum
{
  PN_POSITION_TAIL = -1,
  PN_POSITION_HEAD = 0
} PnContainerPosition;

#define PN_TYPE_CONTAINER              (pn_container_get_type ())
#define PN_CONTAINER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_CONTAINER, PnContainer))
#define PN_CONTAINER_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_CONTAINER, PnContainerClass))
#define PN_IS_CONTAINER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_CONTAINER))
#define PN_IS_CONTAINER_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_CONTAINER))
#define PN_CONTAINER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_CONTAINER, PnContainerClass))

#define PN_CONTAINER_ACTUATORS(obj)    (PN_CONTAINER (obj)->actuators)

typedef struct _PnContainer        PnContainer;
typedef struct _PnContainerClass   PnContainerClass;

struct _PnContainer
{
  PnActuator parent;

  GArray *actuators; /* read-only */
};

struct _PnContainerClass
{
  PnActuatorClass parent_class;

  gboolean (* add_actuator) (PnContainer *container,
			     PnActuator *actuator,
			     gint position);
};

/* Creators */
GType                 pn_container_get_type                (void);

/* Actions */
gboolean              pn_container_add_actuator            (PnContainer *container,
							    PnActuator *actuator,
							    gint position);
void                  pn_container_remove_actuator         (PnContainer *container,
							    PnActuator *actuator);
void                  pn_container_remove_all_actuators    (PnContainer *container);





#endif /* __PN_CONTAINER_H__ */
