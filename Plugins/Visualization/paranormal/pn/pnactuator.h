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

#ifndef __PN_ACTUATOR_H__
#define __PN_ACTUATOR_H__

#include <glib.h>
#include "pnuserobject.h"
#include "pnoption.h"
#include "pnimage.h"
#include "pnaudiodata.h"


G_BEGIN_DECLS


enum
{
  PN_ACTUATOR_OPT_LAST = 0
};

#define PN_TYPE_ACTUATOR              (pn_actuator_get_type ())
#define PN_ACTUATOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_ACTUATOR, PnActuator))
#define PN_ACTUATOR_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_ACTUATOR, PnActuatorClass))
#define PN_IS_ACTUATOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_ACTUATOR))
#define PN_IS_ACTUATOR_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_ACTUATOR))
#define PN_ACTUATOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_ACTUATOR, PnActuatorClass))

typedef struct _PnActuator        PnActuator;
typedef struct _PnActuatorClass   PnActuatorClass;

typedef void (*PnActuatorPrepFunc) (PnActuator *actuator,
				    PnImage *image);
typedef void (*PnActuatorExecFunc) (PnActuator *actuator,
				    PnImage *image,
				    PnAudioData *audio_data);

struct _PnActuator
{
  PnUserObject parent;

  GArray *options;
};

struct _PnActuatorClass
{
  PnUserObjectClass parent_class;

  void (* prepare) (PnActuator *actuator,
		    PnImage *image);
  void (* execute) (PnActuator *actuator,
		    PnImage *image,
		    PnAudioData *audio_data);
};

/* Creators */
GType                 pn_actuator_get_type                (void);

/* Accessors */
void                  pn_actuator_add_option              (PnActuator *actuator,
							   PnOption *option);
PnOption             *pn_actuator_get_option_by_index     (PnActuator *actuator,
							   guint index);
PnOption             *pn_actuator_get_option_by_name      (PnActuator *actuator,
							   const gchar *name);
/* Actions */
void                  pn_actuator_prepare                 (PnActuator *actuator,
							   PnImage *image);
void                  pn_actuator_execute                 (PnActuator *actuator,
							   PnImage *image,
							   PnAudioData *audio_data);




#endif /* __PN_ACTUATOR_H__ */
