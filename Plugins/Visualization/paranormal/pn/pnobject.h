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

#ifndef __PN_OBJECT_H__
#define __PN_OBJECT_H__

#include <config.h>

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define PN_TYPE_OBJECT              (pn_object_get_type ())
#define PN_OBJECT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_OBJECT, PnObject))
#define PN_OBJECT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_OBJECT, PnObjectClass))
#define PN_IS_OBJECT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_OBJECT))
#define PN_IS_OBJECT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_OBJECT))
#define PN_OBJECT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_OBJECT, PnObjectClass))
#define PN_OBJECT_CLASS_TYPE(class) (G_TYPE_FROM_CLASS (class))
#define PN_OBJECT_CLASS_NAME(class) (g_type_name (PN_OBJECT_CLASS_TYPE (class)))

#define PN_OBJECT_TYPE(obj)         (G_TYPE_FROM_INSTANCE (obj))
#define PN_OBJECT_TYPE_NAME(obj)    (g_type_name (PN_OBJECT_TYPE (obj)))

typedef enum
{
  PN_DESTROYED		= 1 << 0,
  PN_FLOATING		= 1 << 1,
  PN_RESERVED_1		= 1 << 2,
  PN_RESERVED_2		= 1 << 3
} PnObjectFlags;

#define PN_OBJECT_FLAGS(obj)             (PN_OBJECT (obj)->flags)
#define PN_OBJECT_DESTROYED(obj)         ((PN_OBJECT_FLAGS (obj) & PN_DESTROYED) != 0)
#define PN_OBJECT_FLOATING(obj)	         ((PN_OBJECT_FLAGS (obj) & PN_FLOATING) != 0)
#define PN_OBJECT_CONNECTED(obj)         ((PN_OBJECT_FLAGS (obj) & PN_CONNECTED) != 0)

#define PN_OBJECT_SET_FLAGS(obj,flag)	 G_STMT_START{ (PN_OBJECT_FLAGS (obj) |= (flag)); }G_STMT_END
#define PN_OBJECT_UNSET_FLAGS(obj,flag)  G_STMT_START{ (PN_OBJECT_FLAGS (obj) &= ~(flag)); }G_STMT_END

typedef struct _PnObject        PnObject;
typedef struct _PnObjectClass   PnObjectClass;

struct _PnObject
{
  GObject parent;

  /* Only the first four bits are used, so derived classes can use this
   * for their own flags
   */
  guint32 flags;
};

struct _PnObjectClass
{
  GObjectClass parent_class;

  /* if a class overrides this then it MUST call its superclass'
   * implimentation
   */
  void (*destroy) (PnObject *object);
};

/* Creators */
GType             pn_object_get_type                 (void);

/* Referencing */
PnObject*         pn_object_ref                      (PnObject *object);
void              pn_object_unref                    (PnObject *object);
void              pn_object_sink                     (PnObject *object);

/* Destruction */
void              pn_object_destroy                  (PnObject *object);

#endif /* __PN_OBJECT_H__ */
