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

#include <config.h>

#include "pnobject.h"

enum
{
  DESTROY,
  LAST_SIGNAL
};

/* Initialization */
static void         pn_object_class_init       (PnObjectClass *class);
static void         pn_object_init             (PnObject *object,
						PnObjectClass *class);

/* GObject signals */
static void         pn_object_finalize         (GObject *gobject);
static void         pn_object_dispose         (GObject *gobject);

/* PnObject signals */
static void         pn_object_real_destroy     (PnObject *object);

static GObjectClass *parent_class = NULL;
static guint         object_signals[LAST_SIGNAL] = { 0 };

GType
pn_object_get_type (void)
{
  static GType object_type = 0;

  if (! object_type)
    {
      static const GTypeInfo object_info =
      {
	sizeof (PnObjectClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_object_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnObject),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_object_init
      };

      /* FIXME: should this be dynamic? */
      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "PnObject",
					    &object_info,
					    G_TYPE_FLAG_ABSTRACT);
    }
  return object_type;
}

static void
pn_object_class_init (PnObjectClass *class)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) class;

  parent_class = g_type_class_peek_parent (class);

  gobject_class->dispose = pn_object_dispose;
  gobject_class->finalize = pn_object_finalize;

  class->destroy = pn_object_real_destroy;

  object_signals[DESTROY] =
    g_signal_new ("destroy",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		  G_STRUCT_OFFSET (PnObjectClass, destroy),
		  NULL,
		  NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE,
		  0);
}

static void
pn_object_init (PnObject *object, PnObjectClass *class)
{
  object->flags = PN_FLOATING;
}

void
pn_object_destroy (PnObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (PN_IS_OBJECT (object));
  
  if (!PN_OBJECT_DESTROYED (object))
    {
      /* need to hold a reference count around all class method
       * invocations.
       */
      g_object_run_dispose (G_OBJECT (object));
    }
}

static void
pn_object_dispose (GObject *gobject)
{
  PnObject *object = PN_OBJECT (gobject);

  /* guard against reinvocations during
   * destruction with the PN_DESTROYED flag.
   */
  if (!PN_OBJECT_DESTROYED (object))
    {
      PN_OBJECT_SET_FLAGS (object, PN_DESTROYED);
      
      g_signal_emit (object, object_signals[DESTROY], 0);
      
      PN_OBJECT_UNSET_FLAGS (object, PN_DESTROYED);
    }

  G_OBJECT_CLASS (parent_class)->dispose (gobject);
}

static void
pn_object_real_destroy (PnObject *object)
{
  g_signal_handlers_destroy (G_OBJECT (object));
}

static void
pn_object_finalize (GObject *gobject)
{
  PnObject *object;

  object = PN_OBJECT (gobject);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

/**
 * pn_object_sink
 * @object: a #PnObject
 *
 * Removes floating reference on an object.  Any newly created object has
 * a refcount of 1 and is FLOATING.  This function should be used when
 * creating a new object to symbolically 'take ownership of' the object.
 */
void
pn_object_sink (PnObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (PN_IS_OBJECT (object));

  if (PN_OBJECT_FLOATING (object))
    {
      PN_OBJECT_UNSET_FLAGS (object, PN_FLOATING);
      pn_object_unref (object);
    }
}

/**
 * pn_object_ref
 * @object: a #PnObject
 *
 * Increments the reference count on an object
 *
 * Returns: A pointer to the object
 */
PnObject*
pn_object_ref (PnObject *object)
{
  g_return_val_if_fail (object != NULL, NULL);
  g_return_val_if_fail (PN_IS_OBJECT (object), NULL);

  g_object_ref ((GObject*) object);
  
  return object;
}

/**
 * pn_object_unref
 * @object: a #PnObject
 *
 * Decrements the reference count on an object.  If the object's reference
 * count becomes 0, the object is freed from memory.
 */
void
pn_object_unref (PnObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (PN_IS_OBJECT (object));

  g_object_unref ((GObject*) object);
}
