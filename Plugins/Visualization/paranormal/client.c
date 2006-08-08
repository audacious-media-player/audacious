/* pnxmms - An xmms visualization plugin using the Paranormal
 * audio visualization library
 * 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <audacious/plugin.h>
#include <audacious/configdb.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include "pn.h"

/* Paranormal visualization object */
static PnVis *vis;

/* Paranormal audio data object */
static PnAudioData *audio_data;

/* SDL window's surface */
static SDL_Surface *screen;

/* Synchronization stuffs */
static SDL_Thread *render_thread;
static int render_func (void *data);
static gboolean kill_render_thread;
static gboolean quit_timeout_func (gpointer data);
static guint quit_timeout;
static gboolean render_thread_finished;

static SDL_mutex *vis_mutex;
static SDL_mutex *audio_data_mutex;
static gfloat intermediate_pcm_data[3][512];
static gfloat intermediate_freq_data[3][256];

/* PNXMMS Options */
static gboolean options_read = FALSE;
static gboolean preset_changed = FALSE;
static gchar *preset;
static guint image_width = 320;
static guint image_height = 240;

/* Config dialog */
GtkWidget *config_dialog;
GtkWidget *load_button;

/* Config functions */
static void read_options (void);
static void write_options (void);
static void load_default_vis (void);
static void create_config_dialog (void);

/* Rendering functions */
static void draw_image (PnImage *image);

/* XMMS interface */
static void pn_xmms_init (void);
static void pn_xmms_cleanup (void);
static void pn_xmms_about (void);
static void pn_xmms_configure (void);
static void pn_xmms_render_pcm (gint16 data[2][512]);
static void pn_xmms_render_freq (gint16 data[2][256]);

static VisPlugin pn_xmms_vp =
{
  NULL,    /* handle */
  NULL,    /* filename */
  0,       /* xmms_session */
  PACKAGE " " VERSION,
  2,
  2,
  pn_xmms_init,
  pn_xmms_cleanup,
  NULL,    /* pn_xmms_about, */
  pn_xmms_configure,
  NULL,    /* disable_plugin */
  NULL,    /* playback_start */
  NULL,    /* pkayback_stop */
  pn_xmms_render_pcm,
  pn_xmms_render_freq
};

VisPlugin*
get_vplugin_info (void)
{
  return &pn_xmms_vp;
}

static void
pn_xmms_init (void)
{
  /* Load the saved options */
  read_options ();

  /* Initialize SDL */
  if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0)
    {
      g_warning ("Unable to initialize SDL: %s\n", SDL_GetError ());
      pn_xmms_vp.disable_plugin (&pn_xmms_vp);
    }

  screen = SDL_SetVideoMode (image_width, image_height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
  if (! screen)
    {
      g_warning ("Unable to create a new SDL window: %s\n", SDL_GetError ());
      pn_xmms_vp.disable_plugin (&pn_xmms_vp);
    }

  SDL_WM_SetCaption (PACKAGE " " VERSION, PACKAGE);

  /* Initialize Paranormal */
  pn_init ();

  /* Set up the visualization objects */
  vis = pn_vis_new (image_width, image_height);
  pn_object_ref (PN_OBJECT (vis));
  pn_object_sink (PN_OBJECT (vis));

  if (! preset || ! pn_vis_load_from_file (vis, preset))
    load_default_vis ();

  audio_data = pn_audio_data_new ();
  pn_object_ref (PN_OBJECT (audio_data));
  pn_object_sink (PN_OBJECT (audio_data));
  pn_audio_data_set_stereo (audio_data, FALSE);
  pn_audio_data_set_pcm_samples (audio_data, 512);
  pn_audio_data_set_freq_bands (audio_data, 256);

  /* Set up the render thread */
  audio_data_mutex = SDL_CreateMutex ();
  vis_mutex = SDL_CreateMutex ();
  if (! audio_data_mutex || ! vis_mutex)
    {
      g_warning ("Unable to create mutex: %s\n", SDL_GetError ());
      pn_xmms_vp.disable_plugin (&pn_xmms_vp);
    }

  kill_render_thread = FALSE;
  render_thread_finished = FALSE;
  render_thread = SDL_CreateThread (render_func, NULL);
  if (! render_thread)
    {
      g_warning ("Unable to create render thread: %s\n", SDL_GetError ());
      pn_xmms_vp.disable_plugin (&pn_xmms_vp);
    }

  /* Create the quit-checker timeout */
  quit_timeout = gtk_timeout_add (500, quit_timeout_func, NULL);
}

static void
pn_xmms_cleanup (void)
{
  if (quit_timeout != 0)
    {
      gtk_timeout_remove (quit_timeout);
      quit_timeout = 0;
    }

  if (render_thread)
    {
      kill_render_thread = TRUE;
      SDL_WaitThread (render_thread, NULL);
      render_thread = NULL;
    }

  if (audio_data_mutex)
    {
      SDL_DestroyMutex (audio_data_mutex);
      audio_data_mutex = NULL;
    }

  if (screen)
    {
      SDL_FreeSurface (screen);
      screen = NULL;
    }
  SDL_Quit ();

  if (vis)
    {
      pn_object_unref (PN_OBJECT (vis));
      vis = NULL;
    }

  write_options ();
}

static void
pn_xmms_about (void)
{
  /* FIXME: This needs to wait until XMMS supports GTK+ 2.0 */
}

static void
pn_xmms_configure (void)
{
  read_options ();
  create_config_dialog ();
}

static void
pn_xmms_render_pcm (gint16 data[2][512])
{
  guint i;

  /* Lock the audio data */
  SDL_mutexP (audio_data_mutex);

  /* Set up the audio data */
  for (i=0; i<512; i++)
    {
      intermediate_pcm_data[PN_CHANNEL_LEFT][i] = (gfloat)(data[0][i]) / 32768.0;
      intermediate_pcm_data[PN_CHANNEL_RIGHT][i] = (gfloat)(data[0][i]) / 32768.0;
      intermediate_pcm_data[PN_CHANNEL_CENTER][i] = .5 * (intermediate_pcm_data[PN_CHANNEL_LEFT][i]
							  + intermediate_pcm_data[PN_CHANNEL_RIGHT][i]);
    }

  /* Unlock the audio data */
  SDL_mutexV (audio_data_mutex);
}

static void
pn_xmms_render_freq (gint16 data[2][256])
{
  guint i;

  /* Lock the audio data */
  SDL_mutexP (audio_data_mutex);

  /* Set up the audio data */
  for (i=0; i<256; i++)
    {
      intermediate_freq_data[PN_CHANNEL_LEFT][i] = (gfloat)(data[0][i]) / 32768.0;
      intermediate_freq_data[PN_CHANNEL_RIGHT][i] = (gfloat)(data[0][i]) / 32768.0;
      intermediate_freq_data[PN_CHANNEL_CENTER][i] = .5 * (intermediate_freq_data[PN_CHANNEL_LEFT][i]
							  + intermediate_freq_data[PN_CHANNEL_RIGHT][i]);
    }

  /* Unlock the audio data */
  SDL_mutexV (audio_data_mutex);
}

static gboolean
quit_timeout_func (gpointer data)
{
  if (render_thread_finished)
    {
      pn_xmms_vp.disable_plugin (&pn_xmms_vp);
      quit_timeout = 0;
      return FALSE;
    }
  return TRUE;
}

static int
render_func (void *data)
{
  guint i;
  PnImage *image;
  guint last_time = 0, last_second = 0;
  gfloat fps = 0.0;
  guint this_time;
  SDL_Event event;

  while (! kill_render_thread)
    {
      /* Lock & copy the audio data */
      SDL_mutexP (audio_data_mutex);

      for (i=0; i<3; i++)
	{
	  memcpy (PN_AUDIO_DATA_PCM_DATA (audio_data, i), intermediate_pcm_data[i], 512 * sizeof (gfloat));
	  memcpy (PN_AUDIO_DATA_FREQ_DATA (audio_data, i), intermediate_freq_data[i], 256 * sizeof (gfloat));
	}

      SDL_mutexV (audio_data_mutex);

      pn_audio_data_update (audio_data);

      /* Render the image */
      SDL_mutexP (vis_mutex);
      image = pn_vis_render (vis, audio_data);
      SDL_mutexV (vis_mutex);

      /* Draw the image */
      draw_image (image);

      /* Compute the FPS */
      this_time = SDL_GetTicks ();

      fps = fps * .95 + (1000.0 / (gfloat) (this_time - last_time)) * .05;
      if (this_time > 2000 + last_second)
	{
	  last_second = this_time;
	  printf ("paranormal fps: %f\n", fps);
	}
      last_time = this_time;

      /* Handle window events */
      while (SDL_PollEvent (&event))
	{
	  switch (event.type)
	    {
	    case SDL_QUIT:
	      render_thread_finished = TRUE;
	      return 0;

	    case SDL_VIDEORESIZE:
	      image_width = event.resize.w;
	      image_height = event.resize.h;
	      screen = SDL_SetVideoMode (image_width, image_height, 32,
					 SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE);
	      printf ("changed to: w: %u, h: %u\n", image_width, image_height);
	      pn_vis_set_image_size (vis, image_width, image_height);
	      break;

	    case SDL_KEYDOWN:
	      switch (event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
		  render_thread_finished = TRUE;
		  return 0;

		case SDLK_RETURN:
		  if (event.key.keysym.mod & (KMOD_ALT | KMOD_META))
		    {
		      /* Toggle fullscreen */
		      SDL_WM_ToggleFullScreen (screen);  
		      if (SDL_ShowCursor (SDL_QUERY) == SDL_ENABLE)
			SDL_ShowCursor (SDL_DISABLE);
		      else
			SDL_ShowCursor (SDL_ENABLE);
		    }
		  break;
		case SDLK_BACKQUOTE:
		  {
		    char fname[20];
		    int i = 0;
		    struct stat buf;

		    do
		      {
			sprintf (fname, "pnxmms_ss_%05u.bmp", i++);
		      } while(!(stat(fname, &buf) != 0 && errno == ENOENT ));
		    SDL_SaveBMP(screen, fname); 
		  }
		  break;

		default:
		  break;
		}
	      break;
	    }
	}
    }

  return 0;
}

static void
draw_image (PnImage *image)
{
  guint width, height, src_stride, dest_stride;
  guint i;
  register PnColor *src;
  register Uint32 *dest;

  /* Lock the SDL surface */
  SDL_LockSurface (screen);

  /* Get the dimentions */
  width = pn_image_get_width (image);
  height = pn_image_get_height (image);

  src_stride = pn_image_get_pitch (image) >> 2;
  dest_stride = screen->pitch >> 2;

  src = pn_image_get_image_buffer (image);
  dest = (Uint32 *) screen->pixels;

  /* Copy the pixel data */
  while (--height > 0)
    {
      for (i=0; i<width; i++)
	dest[i] = SDL_MapRGB (screen->format, src[i].red, src[i].green, src[i].blue);
      dest += dest_stride;
      src += src_stride;
    }

  /* Unlock the SDL surface */
  SDL_UnlockSurface (screen);

  /* Draw it to the screen */
  SDL_Flip (screen);
}

static void
read_options (void)
{
  ConfigDb *cfg;
  gint i;

  if (options_read == TRUE)
    return;

  cfg = bmp_cfg_db_open ();
  if (!cfg)
    return;

  if (bmp_cfg_db_get_int (cfg, "pnxmms", "image_width", &i))
    image_width = i;

  if (bmp_cfg_db_get_int (cfg, "pnxmms", "image_height", &i))
    image_height = i;

  bmp_cfg_db_close (cfg);

  return;
}

static void
write_options (void)
{
  ConfigDb *cfg;

  cfg = bmp_cfg_db_open ();
  if (!cfg)
    fprintf (stderr, "PNXMMS: Unable to open XMMS config file!\n");

  bmp_cfg_db_set_int (cfg, "pnxmms", "image_width", image_width);
  bmp_cfg_db_set_int (cfg, "pnxmms", "image_height", image_height);

  bmp_cfg_db_close (cfg);
}

static void
load_default_vis (void)
{
  PnContainer *container;
  PnActuator *actuator;
  PnOption *option;

  /* Actuator List */
  container = (PnContainer *) pn_actuator_list_new ();

  /* Scope */
  actuator = (PnActuator *) pn_scope_new ();
  option = pn_actuator_get_option_by_name (actuator, "init_script");
  pn_string_option_set_value (PN_STRING_OPTION (option), "samples = width/2;");
  option = pn_actuator_get_option_by_name (actuator, "frame_script");
  pn_string_option_set_value (PN_STRING_OPTION (option),
			      "base = base + .04;\n"
			      "red = abs (sin (.5 * pi + .1 * base));\n"
			      "green = .5 * abs (sin (.5 * pi - .2 * base));\n"
			      "blue = abs (sin (.5 * pi + .3 * base));");
  option = pn_actuator_get_option_by_name (actuator, "sample_script");
  pn_string_option_set_value (PN_STRING_OPTION (option),
			      "x = 2 * iteration - 1;\n"
			      "y = value;");
  option = pn_actuator_get_option_by_name (actuator, "draw_method");
  pn_list_option_set_value (PN_LIST_OPTION (option), "Lines");
  pn_container_add_actuator (container, actuator, PN_POSITION_TAIL);

  /* Distortion */
  actuator = (PnActuator *) pn_distortion_new ();
  option = pn_actuator_get_option_by_name (actuator, "distortion_script");
  pn_string_option_set_value (PN_STRING_OPTION (option),
			      "intensity = .99 + .08 * r;\n"
			      "r = .98 * atan (r);\n"
			      "theta = theta + .01;");
  option = pn_actuator_get_option_by_name (actuator, "polar_coords");
  pn_boolean_option_set_value (PN_BOOLEAN_OPTION (option), TRUE);
  pn_container_add_actuator (container, actuator, PN_POSITION_TAIL);

  /* Add the actuator list to the vis */
  pn_vis_set_root_actuator (vis, PN_ACTUATOR (container));
}

static void
apply_settings (void)
{
  if (preset_changed)
    {
      preset_changed = FALSE;
      SDL_mutexP (vis_mutex);
      pn_vis_load_from_file (vis, preset);
      SDL_mutexV (vis_mutex);
    }

  write_options ();
}

static void
destroy_config_dialog (GtkObject *user_data, gpointer data)
{
  gtk_widget_destroy (GTK_WIDGET(config_dialog));
  config_dialog = NULL;
}

static void
apply_button_cb (GtkButton *button, gpointer data)
{
  apply_settings ();
}

static void
ok_button_cb (GtkButton *button, gpointer data)
{
  apply_settings ();
  destroy_config_dialog (NULL, NULL);
}

static void
cancel_button_cb (GtkButton *button, gpointer data)
{
  destroy_config_dialog (NULL, NULL);
}

/* If selector != NULL, then it's 'OK', otherwise it's 'Cancel' */
static void
load_preset_cb (GtkButton *button, GtkFileSelection *selector)
{
  if (selector)
    {
      if (preset)
	g_free (preset);
      preset = g_strdup (gtk_file_selection_get_filename (selector));
      gtk_label_set_text (GTK_LABEL (GTK_BIN (load_button)->child), preset);
      preset_changed = TRUE;
    }

  gtk_widget_set_sensitive (config_dialog, TRUE);
}

static void
load_preset_button_cb (GtkButton *button, gpointer data)
{
  GtkWidget *selector;
  
  selector = gtk_file_selection_new ("Load Preset");

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (selector)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC (load_preset_cb), selector);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (selector)->cancel_button),
		      "clicked", GTK_SIGNAL_FUNC (load_preset_cb), NULL);

  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (selector)->ok_button),
			     "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) selector);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (selector)->cancel_button),
			     "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (gpointer) selector);

  gtk_widget_set_sensitive (config_dialog, FALSE);
  gtk_widget_show (selector);
}

static void
create_config_dialog (void)
{
  GtkWidget *bbox, *button, *vbox, *frame, *table, *label;

  if (config_dialog != NULL)
    return;

  /* Create the dialog */
  config_dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (config_dialog), PACKAGE " " VERSION
			" - Configuration");
  gtk_widget_set_usize (config_dialog, 500, 300);
  gtk_container_border_width (GTK_CONTAINER (config_dialog), 8);
  gtk_signal_connect_object (GTK_OBJECT (config_dialog), "delete-event",
			     destroy_config_dialog, NULL);

  /* OK / Cancel / Apply */
  bbox = gtk_hbutton_box_new ();
  gtk_widget_show (bbox);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 8);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (bbox), 64, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (config_dialog)->action_area),
		      bbox, FALSE, FALSE, 0);
  button = gtk_button_new_with_label ("OK");
  gtk_widget_show (button);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (ok_button_cb), NULL);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  button = gtk_button_new_with_label ("Cancel");
  gtk_widget_show (button);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (cancel_button_cb), NULL);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);
  button = gtk_button_new_with_label ("Apply");
  gtk_widget_show (button);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (apply_button_cb), NULL);
  gtk_box_pack_start (GTK_BOX (bbox), button, FALSE, FALSE, 0);


  /* The vertical box */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (config_dialog)->vbox), vbox,
		      TRUE, TRUE, 0);

  /* General options */
  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_frame_set_label (GTK_FRAME (frame), "General");
  table = gtk_table_new (2, 1, TRUE);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (frame), table);
  label = gtk_label_new ("Preset File: ");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, 0, 3, 3);
  load_button = gtk_button_new_with_label (preset ? preset : "<default>");
  gtk_widget_show (load_button);
  gtk_table_attach (GTK_TABLE (table), load_button, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, 0, 3, 3);
  gtk_signal_connect (GTK_OBJECT (load_button), "clicked",
		      GTK_SIGNAL_FUNC (load_preset_button_cb), NULL);

  /* Show it all */
  gtk_widget_show (config_dialog);
  gtk_widget_grab_focus (config_dialog);
}
