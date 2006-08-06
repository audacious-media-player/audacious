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

#ifndef __PN_USER_OBJECT_H__
#define __PN_USER_OBJECT_H__

#include "pnobject.h"
#include "pnxml.h"


G_BEGIN_DECLS


#define PN_TYPE_USER_OBJECT              (pn_user_object_get_type ())
#define PN_USER_OBJECT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), PN_TYPE_USER_OBJECT, PnUserObject))
#define PN_USER_OBJECT_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), PN_TYPE_USER_OBJECT, PnUserObjectClass))
#define PN_IS_USER_OBJECT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PN_TYPE_USER_OBJECT))
#define PN_IS_USER_OBJECT_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), PN_TYPE_USER_OBJECT))
#define PN_USER_OBJECT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PN_TYPE_USER_OBJECT, PnUserObjectClass))
#define PN_USER_OBJECT_CLASS_TYPE(class) (G_TYPE_FROM_CLASS (class))
#define PN_USER_OBJECT_CLASS_NAME(class) (g_type_name (PN_USER_OBJECT_CLASS_TYPE (class)))

#define PN_USER_OBJECT_TYPE(obj)         (G_TYPE_FROM_INSTANCE (obj))
#define PN_USER_OBJECT_TYPE_NAME(obj)    (g_type_name (PN_USER_OBJECT_TYPE (obj)))

#define PN_USER_OBJECT_NAME(obj)         (PN_USER_OBJECT (obj)->name)
#define PN_USER_OBJECT_DESC(obj)         (PN_USER_OBJECT (obj)->desc)
#define PN_USER_OBJECT_OWNER(obj)        (PN_USER_OBJECT (obj)->owner)

typedef struct _PnUserObject        PnUserObject;
typedef struct _PnUserObjectClass   PnUserObjectClass;

struct _PnUserObject
{
  PnObject parent;

  gchar *name;
  gchar *desc;
  PnUserObject *owner;
};

struct _PnUserObjectClass
{
  PnObjectClass parent_class;

  void (* save_thyself) (PnUserObject *user_object,
			 xmlNodePtr node);
  void (* load_thyself) (PnUserObject *user_object,
			 xmlNodePtr node);
};

GType             pn_user_object_get_type             (void);
#define           pn_user_object_destroy(object)      pn_object_destroy (PN_OBJECT (object))

/* Accessors */
void              pn_user_object_set_name             (PnUserObject *user_object,
						       const gchar *name);
gchar            *pn_user_object_get_name             (PnUserObject *user_object);
void              pn_user_object_set_description      (PnUserObject *user_object,
						       const gchar *desc);
gchar            *pn_user_object_get_description      (PnUserObject *user_object);
void              pn_user_object_set_owner            (PnUserObject *user_object,
						       PnUserObject *owner);
PnUserObject     *pn_user_object_get_owner            (PnUserObject *user_object);

/* Actions */
void              pn_user_object_save_thyself         (PnUserObject *user_object,
						       xmlNodePtr node);
void              pn_user_object_load_thyself         (PnUserObject *user_object,
						       xmlNodePtr node);





#endif /* __PN_USER_OBJECT_H__ */
