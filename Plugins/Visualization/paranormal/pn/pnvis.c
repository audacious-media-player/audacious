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

#include <glib.h>
#include "pnvis.h"
#include "pnactuatorfactory.h"
#include "pnerror.h"

static void         pn_vis_class_init       (PnVisClass *class);
static void         pn_vis_init             (PnVis *vis,
					     PnVisClass *class);

/* GObject signals */
static void         pn_vis_finalize         (GObject *gobject);

/* PnObject signals */
static void         pn_vis_destroy          (PnObject *object);

/* PnUserObject methods */
static void         pn_vis_save_thyself     (PnUserObject *user_object,
					     xmlNodePtr node);
static void         pn_vis_load_thyself     (PnUserObject *user_object,
					     xmlNodePtr node);

static PnUserObjectClass *parent_class = NULL;

GType
pn_vis_get_type (void)
{
  static GType vis_type = 0;

  if (! vis_type)
    {
      static const GTypeInfo vis_info =
      {
	sizeof (PnVisClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_vis_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnVis),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_vis_init
      };

      /* FIXME: should this be dynamic? */
      vis_type = g_type_register_static (PN_TYPE_USER_OBJECT,
					 "PnVis",
					 &vis_info,
					 0);
    }
  return vis_type;
}

static void
pn_vis_class_init (PnVisClass *class)
{
  GObjectClass *gobject_class;
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;

  parent_class = g_type_class_peek_parent (class);

  gobject_class = (GObjectClass *) class;
  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;

  /* GObject signals */
  gobject_class->finalize = pn_vis_finalize;

  /* PnObject signals */
  object_class->destroy = pn_vis_destroy;

  /* PnUserObject methods */
  user_object_class->save_thyself = pn_vis_save_thyself;
  user_object_class->load_thyself = pn_vis_load_thyself;
}

static void
pn_vis_init (PnVis *vis, PnVisClass *class)
{
  /* Set a name for xml stuffs */
  pn_user_object_set_name (PN_USER_OBJECT (vis), "Paranormal_Visualization");

  /* Create a new image context */
  vis->root_image = pn_image_new ();
  pn_object_ref (PN_OBJECT (vis->root_image));
  pn_object_sink (PN_OBJECT (vis->root_image));
}

static void
pn_vis_destroy (PnObject *object)
{
  PnVis *vis = (PnVis *) object;

  if (vis->root_actuator)
    pn_object_unref (PN_OBJECT (vis->root_actuator));

  if (vis->root_image)
    pn_object_unref (PN_OBJECT (vis->root_image));
}

static void
pn_vis_finalize (GObject *gobject)
{
  PnVis *vis;

  vis = (PnVis *) gobject;
}

static void
pn_vis_save_thyself (PnUserObject *user_object, xmlNodePtr node)
{
  PnVis *vis;
  xmlNsPtr ns;
  xmlNodePtr actuators_node, actuator_node;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_VIS (user_object));
  g_return_if_fail (node != NULL);

  vis = (PnVis *) user_object;

  ns = xmlNewNs (node, PN_XML_NS_HREF, (xmlChar *) "pn");
  xmlSetNs (node, ns);

  actuators_node = xmlNewChild (node, NULL, (xmlChar *) "Actuators", NULL);
  actuator_node = xmlNewChild (actuators_node, NULL, (xmlChar *) user_object->name, NULL);
  if (vis->root_actuator)
    pn_user_object_save_thyself (PN_USER_OBJECT (vis->root_actuator), actuator_node);

  if (parent_class->save_thyself)
    parent_class->save_thyself (user_object, node);
}

static void
pn_vis_load_thyself (PnUserObject *user_object, const xmlNodePtr node)
{
  PnVis *vis;
  xmlNodePtr actuators_node, actuator_node, tptr;
  PnActuator *root_actuator = NULL;

  g_return_if_fail (user_object != NULL);
  g_return_if_fail (PN_IS_VIS (user_object));
  g_return_if_fail (node != NULL);

  vis = (PnVis *) user_object;

  /* find the 'Actuators' node */
  for (actuators_node = node->xmlChildrenNode; actuators_node; actuators_node = actuators_node->next)
    if (g_strcasecmp (actuators_node->name, "Actuators") == 0)
      break;
  if (! actuators_node)
    {
      pn_error ("unable to load a PnVis from xml node \"%s\"", node->name);
      return;
    }
      
  /* get the root actuator's node */
  for (actuator_node = actuators_node->xmlChildrenNode; actuator_node != NULL && !g_strcasecmp(actuator_node->name, "text");
	actuator_node = actuator_node->next);
  if (! actuator_node)
    goto done;

  /* Create a new actuator (reworked by nenolod) */
#if 0
  for (tptr = actuator_node; tptr != NULL && root_actuator == NULL; tptr = tptr->next)
  {
     xmlNodePtr ttptr;

     for (ttptr = tptr; ttptr != NULL; ttptr = ttptr->xmlChildrenNode)
  }
#endif

  root_actuator = pn_actuator_factory_new_actuator_from_xml (actuator_node);
  if (!root_actuator)
    pn_error ("Unknown actuator \"%s\" -> '%s'", actuator_node->name, actuator_node->content);

  pn_vis_set_root_actuator (vis, root_actuator);

 done:
  if (parent_class->load_thyself)
    parent_class->load_thyself (user_object, node);
}

PnVis*
pn_vis_new (guint image_width, guint image_height)
{
  PnVis *vis;

  g_return_val_if_fail (image_width > 0, NULL);
  g_return_val_if_fail (image_height > 0, NULL);

  vis = (PnVis *) g_object_new (PN_TYPE_VIS, NULL);
  pn_vis_set_image_size (vis, image_width, image_height);

  return vis;
}

void
pn_vis_set_root_actuator (PnVis *vis, PnActuator *actuator)
{
  g_return_if_fail (vis != NULL);
  g_return_if_fail (PN_IS_VIS (vis));
  g_return_if_fail (actuator != NULL);
  g_return_if_fail (PN_IS_ACTUATOR (actuator));

  if (vis->root_actuator)
    pn_object_unref (PN_OBJECT (vis->root_actuator));

  vis->root_actuator = actuator;

  pn_object_ref (PN_OBJECT (actuator));
  pn_object_sink (PN_OBJECT (actuator));
  pn_user_object_set_owner (PN_USER_OBJECT (actuator), PN_USER_OBJECT (vis));

  pn_actuator_prepare (actuator, vis->root_image);
}

PnActuator*
pn_vis_get_root_actuator (PnVis *vis)
{
  g_return_val_if_fail (vis != NULL, NULL);
  g_return_val_if_fail (PN_IS_VIS (vis), NULL);

  return vis->root_actuator;
}

void
pn_vis_set_image_size (PnVis *vis, guint width, guint height)
{
  g_return_if_fail (vis != NULL);
  g_return_if_fail (PN_IS_VIS (vis));

  pn_image_set_size (vis->root_image, width, height);

  if (vis->root_actuator)
    pn_actuator_prepare (vis->root_actuator, vis->root_image);
}

gboolean
pn_vis_save_to_file (PnVis *vis, const gchar *fname)
{
  xmlDocPtr doc;

  doc = xmlNewDoc ((xmlChar *) "1.0");

  doc->xmlRootNode = xmlNewDocNode (doc, NULL, (xmlChar *) "BUG", NULL);

  pn_user_object_save_thyself (PN_USER_OBJECT (vis), doc->xmlRootNode);

  if ( xmlSaveFile (fname, doc) == -1)
    {
      xmlFreeDoc (doc);
      return FALSE;
    }

  xmlFreeDoc (doc);
  return TRUE;
}

gboolean
pn_vis_load_from_file (PnVis *vis, const gchar *fname)
{
  xmlDocPtr doc;
  xmlNodePtr root_node;
  xmlNsPtr ns;

  doc = xmlParseFile (fname);
  if (! doc)
    {
      pn_error ("unable to parse file \"%s\"", fname);
      return FALSE;
    }

  root_node = xmlDocGetRootElement (doc);
  if (! root_node)
    {
      pn_error ("no root element for file \"%s\"", fname);
      return FALSE;
    }

  ns = xmlSearchNsByHref (doc, root_node, PN_XML_NS_HREF);
  if (! ns)
    {
      pn_error ("invalid file format: paranormal namespace not found");
      return FALSE;
    }

  if (g_strcasecmp (root_node->name, pn_user_object_get_name (PN_USER_OBJECT (vis))))
    {
      pn_error ("invalid file format: this file does not contain a Paranormal visualization");
      return FALSE;
    }

  pn_user_object_load_thyself (PN_USER_OBJECT (vis), root_node);

  return TRUE;
}

PnImage*
pn_vis_render (PnVis *vis, PnAudioData *audio_data)
{
  g_return_val_if_fail (vis != NULL, NULL);
  g_return_val_if_fail (PN_IS_VIS (vis), NULL);
  g_return_val_if_fail (audio_data != NULL, NULL);
/*    g_return_val_if_fail (PN_IS_AUDIO_DATA (audio_data), NULL); */

  if (vis->root_actuator)
      pn_actuator_execute (vis->root_actuator,
			   vis->root_image,
			   audio_data);

  return vis->root_image;
}
