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

#include <stdio.h>
#include <math.h>

#include <glib.h>
#include "pnrotozoom.h"
#include "pnstringoption.h"
#include "pnscript.h"
#include "pnsymboltable.h"
#include "pncpu.h"

static void         pn_roto_zoom_class_init       (PnRotoZoomClass *class);
static void         pn_roto_zoom_init             (PnRotoZoom *roto_zoom,
						   PnRotoZoomClass *class);

/* PnObject signals */
static void         pn_roto_zoom_destroy          (PnObject *object);

/* PnActuator methods */
static void         pn_roto_zoom_prepare          (PnRotoZoom *roto_zoom,
						   PnImage *image);
static void         pn_roto_zoom_execute          (PnRotoZoom *roto_zoom,
						   PnImage *image,
						   PnAudioData *audio_data);

static PnActuatorClass *parent_class = NULL;

GType
pn_roto_zoom_get_type (void)
{
  static GType roto_zoom_type = 0;

  if (! roto_zoom_type)
    {
      static const GTypeInfo roto_zoom_info =
      {
	sizeof (PnRotoZoomClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) pn_roto_zoom_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
        sizeof (PnRotoZoom),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pn_roto_zoom_init
      };

      /* FIXME: should this be dynamic? */
      roto_zoom_type = g_type_register_static (PN_TYPE_ACTUATOR,
					       "PnRotoZoom",
					       &roto_zoom_info,
					       0);
    }
  return roto_zoom_type;
}

static void
pn_roto_zoom_class_init (PnRotoZoomClass *class)
{
  PnObjectClass *object_class;
  PnUserObjectClass *user_object_class;
  PnActuatorClass *actuator_class;

  parent_class = g_type_class_peek_parent (class);

  object_class = (PnObjectClass *) class;
  user_object_class = (PnUserObjectClass *) class;
  actuator_class = (PnActuatorClass *) class;

  /* PnObject signals */
  object_class->destroy = pn_roto_zoom_destroy;

  /* PnActuator methods */
  actuator_class->prepare = (PnActuatorPrepFunc) pn_roto_zoom_prepare;
  actuator_class->execute = (PnActuatorExecFunc) pn_roto_zoom_execute;
}

static void
pn_roto_zoom_init (PnRotoZoom *roto_zoom, PnRotoZoomClass *class)
{
  PnStringOption *init_script_opt, *frame_script_opt;

  /* Set up the name and description */
  pn_user_object_set_name (PN_USER_OBJECT (roto_zoom), "Transform.Roto_Zoom");
  pn_user_object_set_description (PN_USER_OBJECT (roto_zoom),
				  "Rotate and zoom the image");

  /* Set up the options */
  init_script_opt = pn_string_option_new ("init_script", "The initialization script");
  frame_script_opt = pn_string_option_new ("frame_script", "The per-frame script");

  pn_actuator_add_option (PN_ACTUATOR (roto_zoom), PN_OPTION (init_script_opt));
  pn_actuator_add_option (PN_ACTUATOR (roto_zoom), PN_OPTION (frame_script_opt));

  /* Create the script objects and symbol table */
  roto_zoom->init_script = pn_script_new ();
  pn_object_ref (PN_OBJECT (roto_zoom->init_script));
  pn_object_sink (PN_OBJECT (roto_zoom->init_script));
  roto_zoom->frame_script = pn_script_new ();
  pn_object_ref (PN_OBJECT (roto_zoom->frame_script));
  pn_object_sink (PN_OBJECT (roto_zoom->frame_script));
  roto_zoom->symbol_table = pn_symbol_table_new ();
  pn_object_ref (PN_OBJECT (roto_zoom->symbol_table));
  pn_object_sink (PN_OBJECT (roto_zoom->symbol_table));

  /* Get the variables from the symbol table */
  roto_zoom->zoom_var = pn_symbol_table_ref_variable_by_name (roto_zoom->symbol_table, "zoom");
  roto_zoom->theta_var = pn_symbol_table_ref_variable_by_name (roto_zoom->symbol_table, "theta");
  roto_zoom->volume_var = pn_symbol_table_ref_variable_by_name (roto_zoom->symbol_table, "volume");
}

static void
pn_roto_zoom_destroy (PnObject *object)
{
  PnRotoZoom *roto_zoom;

  roto_zoom = (PnRotoZoom *) object;

  pn_object_unref (PN_OBJECT (roto_zoom->init_script));
  pn_object_unref (PN_OBJECT (roto_zoom->frame_script));
  pn_object_unref (PN_OBJECT (roto_zoom->symbol_table));
}  

static void
pn_roto_zoom_prepare (PnRotoZoom *roto_zoom, PnImage *image)
{
  PnStringOption *init_script_opt, *frame_script_opt;

  g_return_if_fail (roto_zoom != NULL);
  g_return_if_fail (PN_IS_ROTO_ZOOM (roto_zoom));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));

  /* Parse the script strings */
  init_script_opt =
    (PnStringOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (roto_zoom),
							PN_ROTO_ZOOM_OPT_INIT_SCRIPT);
  frame_script_opt =
    (PnStringOption *) pn_actuator_get_option_by_index (PN_ACTUATOR (roto_zoom),
							PN_ROTO_ZOOM_OPT_FRAME_SCRIPT);

  pn_script_parse_string (roto_zoom->init_script, roto_zoom->symbol_table,
			  pn_string_option_get_value (init_script_opt));
  pn_script_parse_string (roto_zoom->frame_script, roto_zoom->symbol_table,
			  pn_string_option_get_value (frame_script_opt));

  /* Set up in-script vars for no rotozooming */
  PN_VARIABLE_VALUE (roto_zoom->zoom_var) = 1.0;
  PN_VARIABLE_VALUE (roto_zoom->theta_var) = 0.0;

  /* Run the init script */
  pn_script_execute (roto_zoom->init_script);

  if (PN_ACTUATOR_CLASS (parent_class)->prepare)
    PN_ACTUATOR_CLASS (parent_class)->prepare (PN_ACTUATOR (roto_zoom), image);
}

static void
pn_roto_zoom_execute (PnRotoZoom *roto_zoom, PnImage *image, PnAudioData *audio_data)
{
  gint width, height, stride;
  gdouble ax, ay, bx, by, cx, cy;
  gint xdx, xdy, ydx, ydy;
  gint xx, xy, yx, yy;
  gint i, j;
  gdouble zoom, theta, r;

  PnColor *src, *dest;

  g_return_if_fail (roto_zoom != NULL);
  g_return_if_fail (PN_IS_ROTO_ZOOM (roto_zoom));
  g_return_if_fail (image != NULL);
  g_return_if_fail (PN_IS_IMAGE (image));
  g_return_if_fail (audio_data != NULL);
  g_return_if_fail (PN_IS_AUDIO_DATA (audio_data));

  width = pn_image_get_width (image);
  height = pn_image_get_height (image);
  stride = pn_image_get_pitch (image) >> 2;
  src = pn_image_get_image_buffer (image);
  dest = pn_image_get_transform_buffer (image);

  /* set up the volume in-script variable */
  PN_VARIABLE_VALUE (roto_zoom->volume_var) = pn_audio_data_get_volume (audio_data);

  /* run the frame scipt */
  pn_script_execute (roto_zoom->frame_script);

  /* get the in-script variable values */
  zoom = PN_VARIABLE_VALUE (roto_zoom->zoom_var);
  theta = PN_VARIABLE_VALUE (roto_zoom->theta_var);

  if (zoom == 0.0)
    return;

  r = G_SQRT2 / zoom;

  ax = r * cos ((G_PI *  .75) + theta);
  ay = r * sin ((G_PI *  .75) + theta);
  bx = r * cos ((G_PI *  .25) + theta);
  by = r * sin ((G_PI *  .25) + theta);
  cx = r * cos ((G_PI * 1.25) + theta);
  cy = r * sin ((G_PI * 1.25) + theta);

/*    fprintf (stderr, "ax: %f, ay: %f\n", ax, ay); */
/*    fprintf (stderr, "bx: %f, by: %f\n", bx, by); */
/*    fprintf (stderr, "cx: %f, cy: %f\n", cx, cy); */

  ax = ((width-1)<<8) * ((ax + 1) * .5);
  ay = ((height-1)<<8) * ((-ay + 1) * .5);
  bx = ((width-1)<<8) * ((bx + 1) * .5);
  by = ((height-1)<<8) * ((-by + 1) * .5);
  cx = ((width-1)<<8) * ((cx + 1) * .5);
  cy = ((height-1)<<8) * ((-cy + 1) * .5);

/*    fprintf (stderr, "ax': %f, ay': %f\n", ax, ay); */
/*    fprintf (stderr, "bx': %f, by': %f\n", bx, by); */
/*    fprintf (stderr, "cx': %f, cy': %f\n", cx, cy); */

  xdx = (bx - ax) / (width-1);
  xdy = (by - ay) / (width-1);
  ydx = (cx - ax) / (height-1);
  ydy = (cy - ay) / (height-1);

/*    fprintf (stderr, "xdx: %i, xdy: %i\n", xdx, xdy); */
/*    fprintf (stderr, "ydx: %i, ydy: %i\n", ydx, ydy); */

  yx = ax;
  yy = ay;

  for (j=0; j<height; j++)
    {
      while (yx < 0) yx += width<<8;
      while (yy < 0) yy += height<<8;
      while (yx >= width<<8) yx -= width<<8;
      while (yy >= height<<8) yy -= height<<8;

      xx = yx;
      xy = yy;

      for (i=0; i<width; i++)
	{
	  guint r, g, b, offset;
	  guint xfrac, yfrac;

	  while (xx < 0) xx += width<<8;
	  while (xy < 0) xy += height<<8;
	  while (xx >= width<<8) xx -= width<<8;
	  while (xy >= height<<8) xy -= height<<8;

	  offset = (xy >> 8) * stride + (xx >> 8);
	  xfrac = xx & 0xff;
	  yfrac = xy & 0xff;

	  r = src[offset].red * (256 - xfrac) * (256 - yfrac);
	  g = src[offset].green * (256 - xfrac) * (256 - yfrac);
	  b = src[offset].blue * (256 - xfrac) * (256 - yfrac);

	  if (xx >> 8 < width-1)
	    {
	      r += src[offset+1].red * xfrac * (256 - yfrac);
	      g += src[offset+1].green * xfrac * (256 - yfrac);
	      b += src[offset+1].blue * xfrac * (256 - yfrac);
	    }
	  else
	    {
	      r += src[(xy>>8) * stride].red * xfrac * (256 - yfrac);
	      g += src[(xy>>8) * stride].green * xfrac * (256 - yfrac);
	      b += src[(xy>>8) * stride].blue * xfrac * (256 - yfrac);
	    }

	  if (xy >> 8 < height-1)
	    {
	      r += src[offset+stride].red * (256 - xfrac) * yfrac;
	      g += src[offset+stride].green * (256 - xfrac) * yfrac;
	      b += src[offset+stride].blue * (256 - xfrac) * yfrac;
	    }
	  else
	    {
	      r += src[xx>>8].red * (256 - xfrac) * yfrac;
	      g += src[xx>>8].green * (256 - xfrac) * yfrac;
	      b += src[xx>>8].blue * (256 - xfrac) * yfrac;
	    }

	  if (xx >> 8 < width-1 && xy >> 8 < height-1)
	    {
	      r += src[offset+stride+1].red * xfrac * yfrac;
	      g += src[offset+stride+1].green * xfrac * yfrac;
	      b += src[offset+stride+1].blue * xfrac * yfrac;
	    }
	  else
	    {
	      r += src[0].red * xfrac * yfrac;
	      g += src[0].green * xfrac * yfrac;
	      b += src[0].blue * xfrac * yfrac;
	    }

	  dest[j * stride + i].red = r >> 16;
	  dest[j * stride + i].green = g >> 16;
	  dest[j * stride + i].blue = b >> 16;

	  xx += xdx;
	  xy += xdy;
	}

      yx += ydx;
      yy += ydy;
    }
  
/*    fprintf (stderr, "xx: %i, xy: %i\n", xx, xy); */

  pn_image_apply_transform (image);

  if (PN_ACTUATOR_CLASS (parent_class)->execute != NULL)
    PN_ACTUATOR_CLASS (parent_class)->execute (PN_ACTUATOR (roto_zoom), image, audio_data);
}

PnRotoZoom*
pn_roto_zoom_new (void)
{
  return (PnRotoZoom *) g_object_new (PN_TYPE_ROTO_ZOOM, NULL);
}
