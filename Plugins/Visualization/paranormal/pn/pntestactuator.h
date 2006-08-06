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

#ifndef __PN_TEST_ACTUATOR_H__
#define __PN_TEST_ACTUATOR_H__

#include "pnactuator.h"


G_BEGIN_DECLS


enum
{
  PN_TEST_ACTUATOR_OPT_TEST_BOOL = PN_ACTUATOR_OPT_LAST,
  PN_TEST_ACTUATOR_OPT_TEST_INT,
  PN_TEST_ACTUATOR_OPT_TEST_FLOAT,
  PN_TEST_ACTUATOR_OPT_TEST_STR,
  PN_TEST_ACTUATOR_OPT_LAST
};

#define PN_TYPE_TEST_ACTUATOR              (pn_test_actuator_get_type ())
#define PN_TEST_ACTUATOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_TEST_ACTUATOR, PnTestActuator))
#define PN_TEST_ACTUATOR_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_TEST_ACTUATOR, PnTestActuatorClass))
#define PN_IS_TEST_ACTUATOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_TEST_ACTUATOR))
#define PN_IS_TEST_ACTUATOR_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_TEST_ACTUATOR))
#define PN_TEST_ACTUATOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_TEST_ACTUATOR, PnTestActuatorClass))

typedef struct _PnTestActuator        PnTestActuator;
typedef struct _PnTestActuatorClass   PnTestActuatorClass;

struct _PnTestActuator
{
  PnActuator parent;
};

struct _PnTestActuatorClass
{
  PnActuatorClass parent_class;
};

/* Creators */
GType                 pn_test_actuator_get_type                (void);
PnTestActuator       *pn_test_actuator_new                     (void);

#endif /* __PN_TEST_ACTUATOR_H__ */
