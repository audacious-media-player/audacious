#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <audacious/plugin.h>
#include <audacious/util.h>
#include <libaudacious/beepctrl.h>

#include <SDL.h>
#include <SDL_thread.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <libvisual/libvisual.h>

#include "config.h"

#include "lv_bmp_config.h"
#include "about.h"

#define LV_XMMS_DEFAULT_INPUT_PLUGIN "esd"

/* SDL variables */
static SDL_Surface *screen = NULL;
static SDL_Color sdlpal[256];
static SDL_Thread *render_thread;
static SDL_mutex *pcm_mutex;
static SDL_Surface *icon;

/* Libvisual and visualisation variables */
static VisVideo *video;
static VisPalette *pal;

static char song_name[1024];
static const char *cur_lv_plugin = NULL;

static VisBin *bin = NULL;

static VisSongInfo *songinfo;

static Options *options;

static int gl_plug = 0;

static gint16 xmmspcm[2][512];

/* Thread state variables */
static int visual_running = 0;
static int visual_stopped = 1;

static void lv_bmp_init (void);
static void lv_bmp_cleanup (void);
static void lv_bmp_disable (VisPlugin *);
static void lv_bmp_playback_start (void);
static void lv_bmp_playback_stop (void);
static void lv_bmp_render_pcm (gint16 data[2][512]);

static int sdl_quit (void);
static void sdl_set_pal (void);
static void sdl_draw (SDL_Surface *screen);
static int sdl_create (int width, int height);
static int sdl_event_handle (void);

static int visual_upload_callback (VisInput *input, VisAudio *audio, void *private);
static int visual_resize (int width, int height);
static int visual_initialize (int width, int height);
static int visual_render (void*);

static gint disable_func (gpointer data);
static void dummy (GtkWidget *widget, gpointer data);

VisPlugin *get_vplugin_info (void);
	
VisPlugin lv_bmp_vp =
{
	NULL,				/* (void*) handle, filled in by xmms */
	NULL,				/* (char*) Filename, filled in by xmms */
	0,			 	/* The session ID for attaching to the control socket */
	"libvisual proxy plugin",	/* description */
	2,				/* Numbers of PCM channels wanted in the call to render_pcm */
	0,				/* Numbers of freq channels wanted in the call to render_freq */
	lv_bmp_init,			/* init */
	lv_bmp_cleanup,		/* cleanup */
	NULL,				/* about */
	NULL,				/* configure */
	lv_bmp_disable,		/* disable plugin */
	lv_bmp_playback_start,		/* playback start */
	lv_bmp_playback_stop,		/* playback stop */
	lv_bmp_render_pcm,		/* render pcm */
	NULL,				/* render freq */
};

VisPlugin *get_vplugin_info ()
{
	return &lv_bmp_vp;
}

static char *lv_bmp_get_songname ()
{
	return xmms_remote_get_playlist_title (lv_bmp_vp.xmms_session,
			xmms_remote_get_playlist_pos (lv_bmp_vp.xmms_session));
}

static void lv_bmp_init ()
{
        char **argv;
        int argc;
	int ret;
	gchar *msg;
	GtkWidget *msgwin;

	if (!visual_is_initialized ()) {
	        argv = g_malloc (sizeof(char*));
	        argv[0] = g_strdup (_("XMMS plugin"));
        	argc = 1;

		visual_init (&argc, &argv);

        	g_free (argv[0]);
	        g_free (argv);
	}

#if LV_XMMS_ENABLE_DEBUG
	visual_log_set_verboseness (VISUAL_LOG_VERBOSENESS_HIGH);
#endif
	/*g_print ("Trying to set verboseness\n");
	visual_log_set_verboseness (VISUAL_LOG_VERBOSENESS_LOW);
	g_print ("Verboseness done\n");*/

	options = lv_bmp_config_open ();
	if (!options) {
		visual_log (VISUAL_LOG_CRITICAL, _("Cannot get options"));
		return;
	}

	lv_bmp_config_load_prefs ();

	if (SDL_Init (SDL_INIT_VIDEO) < 0) {
		msg = g_strconcat (_("Cannot initialize SDL!\n"),
					SDL_GetError(),
					"\n\n", PACKAGE_NAME,
					_(" will not be loaded."), 0);
		msgwin = xmms_show_message ("libvisual-proxy", msg, _("Accept"), TRUE, dummy, NULL);
		gtk_widget_show (msgwin);
		g_free (msg);
		return;
	}

	icon = SDL_LoadBMP (options->icon_file);
	if (icon)
		SDL_WM_SetIcon (icon, NULL);
	else
		visual_log (VISUAL_LOG_WARNING, _("Cannot not load icon: %s"), SDL_GetError());
	
	pcm_mutex = SDL_CreateMutex ();
	
	if (strlen (options->last_plugin) <= 0 ) {
		visual_log (VISUAL_LOG_INFO, _("Last plugin: (none)"));
	} else {
		visual_log (VISUAL_LOG_INFO, _("Last plugin: %s"), options->last_plugin);
	}

	cur_lv_plugin = options->last_plugin;
	if (!(visual_actor_valid_by_name (cur_lv_plugin))) {
		visual_log (VISUAL_LOG_INFO, _("%s is not a valid actor plugin"), cur_lv_plugin);
		cur_lv_plugin = lv_bmp_config_get_next_actor ();
	}

	SDL_WM_SetCaption (cur_lv_plugin, cur_lv_plugin);

	if (!cur_lv_plugin) {
		visual_log (VISUAL_LOG_CRITICAL, _("Could not get actor plugin"));
		lv_bmp_config_close ();
		return;
	} else {
		lv_bmp_config_set_current_actor (cur_lv_plugin);
	}

	visual_log (VISUAL_LOG_DEBUG, "calling SDL_CreateThread()");

	render_thread = SDL_CreateThread ((void *) visual_render, NULL);
}

static void lv_bmp_cleanup ()
{
	visual_log (VISUAL_LOG_DEBUG, "entering cleanup...");
	visual_running = 0;

	SDL_WaitThread (render_thread, NULL);

	render_thread = NULL;
	visual_stopped = 1;

	visual_log (VISUAL_LOG_DEBUG, "calling SDL_DestroyMutex()");
	SDL_DestroyMutex (pcm_mutex);
	
	pcm_mutex = NULL;

	/*
	 * WARNING This must be synchronized with config module.
	 */

	options->last_plugin = cur_lv_plugin;

	visual_log (VISUAL_LOG_DEBUG, "calling lv_bmp_config_save_prefs()");
	lv_bmp_config_save_prefs ();

	visual_log (VISUAL_LOG_DEBUG, "closing config file");
	lv_bmp_config_close ();

	if (icon != NULL)
		SDL_FreeSurface (icon);

	visual_log (VISUAL_LOG_DEBUG, "destroying VisBin...");
	visual_object_unref (VISUAL_OBJECT (bin));

	visual_log (VISUAL_LOG_DEBUG, "calling sdl_quit()");
	sdl_quit ();
	
	visual_log (VISUAL_LOG_DEBUG, "calling visual_quit()");
	visual_quit ();
}

static void lv_bmp_disable (VisPlugin* plugin)
{

}

static void lv_bmp_playback_start ()
{

}
static void lv_bmp_playback_stop ()
{

}

static void lv_bmp_render_pcm (gint16 data[2][512])
{
	if (visual_running == 1) {
		SDL_mutexP (pcm_mutex);
                memcpy (xmmspcm, data, sizeof(gint16)*2*512);
		strncpy (song_name, lv_bmp_get_songname (), 1023);
		SDL_mutexV (pcm_mutex);
	}
}

static int sdl_quit ()
{
	visual_log (VISUAL_LOG_DEBUG, "Calling SDL_FreeSurface()");
	if (screen != NULL)
		SDL_FreeSurface (screen);

	screen = NULL;

	visual_log (VISUAL_LOG_DEBUG, "sdl_quit: calling SDL_Quit()");
	/*
	 * FIXME this doesn't work!
	 */
	SDL_Quit ();
	
	visual_log (VISUAL_LOG_DEBUG, "Leaving...");
	return 0;
}

static void sdl_set_pal ()
{
	int i;

	visual_log_return_if_fail (screen != NULL);

	if (pal != NULL) {
		for (i = 0; i < 256; i ++) {
			sdlpal[i].r = pal->colors[i].r;
			sdlpal[i].g = pal->colors[i].g;
			sdlpal[i].b = pal->colors[i].b;
		}
		SDL_SetColors (screen, sdlpal, 0, 256);
	}
}

static void sdl_draw (SDL_Surface *screen)
{
	visual_log_return_if_fail (screen != NULL);
	SDL_Flip (screen);
}

static int sdl_create (int width, int height)
{
	const SDL_VideoInfo *videoinfo;
	int videoflags;

	if (screen != NULL)
		SDL_FreeSurface (screen);

        visual_log (VISUAL_LOG_DEBUG, "sdl_create video->bpp %d", video->bpp);
        visual_log (VISUAL_LOG_DEBUG, gl_plug ? "OpenGl plugin at create: yes" : "OpenGl plugin at create: no");

	if (gl_plug == 1) {
		videoinfo = SDL_GetVideoInfo ();

		if (videoinfo == 0) {
			visual_log (VISUAL_LOG_CRITICAL, _("Could not get video info"));
			return -1;
		}

		videoflags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE | SDL_RESIZABLE;

		if (videoinfo->hw_available)
			videoflags |= SDL_HWSURFACE;
		else
			videoflags |= SDL_SWSURFACE;

		if (videoinfo->blit_hw)
			videoflags |= SDL_HWACCEL;

		SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

		visual_log (VISUAL_LOG_DEBUG, "Setting video mode %dx%d", width, height);
		screen = SDL_SetVideoMode (width, height, 16, videoflags);
	} else {
		visual_log (VISUAL_LOG_DEBUG, "Setting video mode %dx%d", width, height);
		screen = SDL_SetVideoMode (width, height, video->bpp * 8, SDL_RESIZABLE);
	}

	SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY / 4, SDL_DEFAULT_REPEAT_INTERVAL / 4);

	visual_video_set_buffer (video, screen->pixels);
        visual_log (VISUAL_LOG_DEBUG, "pointer to the pixels: %p", screen->pixels);

	visual_video_set_pitch (video, screen->pitch);
        visual_log (VISUAL_LOG_DEBUG, "pitch: %d", video->pitch);

	return 0;
}

static int visual_initialize (int width, int height)
{
	VisInput *input;
	VisVideoDepth depth;
        int ret;

	bin = visual_bin_new ();
	visual_bin_set_supported_depth (bin, VISUAL_VIDEO_DEPTH_ALL);
//	visual_bin_set_preferred_depth (bin, VISUAL_BIN_DEPTH_LOWEST);

	depth = visual_video_depth_enum_from_value (options->depth);
	if (depth == VISUAL_VIDEO_DEPTH_ERROR)
		depth = VISUAL_VIDEO_DEPTH_24BIT;
	options->depth = depth;

	video = visual_video_new ();

	ret = visual_video_set_depth (video, depth);
        if (ret < 0) {
                visual_log (VISUAL_LOG_CRITICAL, _("Cannot set video depth"));
                return -1;
        }
	visual_video_set_dimension (video, width, height);

        ret = visual_bin_set_video (bin, video);
	if (ret < 0) {
                visual_log (VISUAL_LOG_CRITICAL, _("Cannot set video"));
                return -1;
        }
	/*visual_bin_connect_by_names (bin, cur_lv_plugin, NULL);*/
	visual_bin_connect_by_names (bin, cur_lv_plugin, LV_XMMS_DEFAULT_INPUT_PLUGIN);

	if (visual_bin_get_depth (bin) == VISUAL_VIDEO_DEPTH_GL) {
		visual_video_set_depth (video, VISUAL_VIDEO_DEPTH_GL);
		gl_plug = 1;
	} else {
		gl_plug = 0;
	}

	visual_log (VISUAL_LOG_DEBUG, gl_plug ? "OpenGl plugin: yes" : "OpenGl plugin: no");
	ret = sdl_create (width, height);
	if (ret < 0) {
                return -1;
        }
	
	/* Called so the flag is set to FALSE, seen we create the initial environment here */
	visual_bin_depth_changed (bin);
	
	input = visual_bin_get_input (bin);
	ret = visual_input_set_callback (input, visual_upload_callback, NULL);
	if (ret < 0) {
                visual_log (VISUAL_LOG_CRITICAL, _("Cannot set input plugin callback"));
                return -1;
        }        
	
	visual_bin_switch_set_style (bin, VISUAL_SWITCH_STYLE_MORPH);
	visual_bin_switch_set_automatic (bin, TRUE);
	visual_bin_switch_set_mode (bin, VISUAL_MORPH_MODE_TIME);
	visual_bin_switch_set_time (bin, 4, 0);

	visual_bin_realize (bin);
	visual_bin_sync (bin, FALSE);

	return 0;
}

static int visual_upload_callback (VisInput *input, VisAudio *audio, void *private_data)
{
	int i;

	visual_log_return_val_if_fail (audio != NULL, -1);

	for (i = 0; i < 512; i++) {
		audio->plugpcm[0][i] = xmmspcm[0][i];
		audio->plugpcm[1][i] = xmmspcm[1][i];
	}

	return 0;
}

static int visual_resize (int width, int height)
{
	visual_video_set_dimension (video, width, height);

	sdl_create (width, height);
	
	options->width = width;
	options->height = height;

	visual_bin_sync (bin, FALSE);

	return 0;
}

static int visual_render (void *arg)
{
	visual_running = 1;
	visual_stopped = 0;
        long render_time, now;
        long frame_length;
        long idle_time;
	long frames;
	int ret;

	ret = visual_initialize (options->width, options->height);
        if (ret < 0) {
                visual_log (VISUAL_LOG_CRITICAL, _("Cannot initialize plugin's visual stuff"));
		return -1;
	}

        frame_length = (1.0 / options->fps) * 1000;
	frames = 0;
	while (visual_running == 1) {
		/* Update songinfo */
		songinfo = visual_actor_get_songinfo (visual_bin_get_actor (bin));
		visual_songinfo_set_type (songinfo, VISUAL_SONGINFO_TYPE_SIMPLE);

		visual_songinfo_set_simple_name (songinfo, song_name);

		/* On depth change */
		if (visual_bin_depth_changed (bin) == TRUE) {
			if (SDL_MUSTLOCK (screen) == SDL_TRUE)
				SDL_LockSurface (screen);

			visual_video_set_buffer (video, screen->pixels);
			if (visual_bin_get_depth (bin) == VISUAL_VIDEO_DEPTH_GL)
				gl_plug = 1;
			else
				gl_plug = 0;
			
			sdl_create (options->width, options->height);
			visual_bin_sync (bin, TRUE);

			if (SDL_MUSTLOCK (screen) == SDL_TRUE)
				SDL_UnlockSurface (screen);
		}
                       
		if (gl_plug == 1) {
			visual_bin_run (bin);

			SDL_GL_SwapBuffers ();
		} else {
			if (SDL_MUSTLOCK (screen) == SDL_TRUE)
				SDL_LockSurface (screen);

			visual_bin_run (bin);

			if (SDL_MUSTLOCK (screen) == SDL_TRUE)
				SDL_UnlockSurface (screen);

			pal = visual_bin_get_palette (bin);
			sdl_set_pal ();

			sdl_draw (screen);
		}

		usleep(10000);

		sdl_event_handle ();

		if (options->fullscreen && !(screen->flags & SDL_FULLSCREEN))
                        SDL_WM_ToggleFullScreen (screen);
		frames++;
		/*
		 * Sometime we actualize the frame_length, because we let user
		 * choose maximum FPS dinamically.
		 */
		if (frames > options->fps) {
			frames = 0;
	        	frame_length = (1.0 / options->fps) * 1000;
		}
	}

	visual_stopped = 1;
	return 0;
}

static int sdl_event_handle ()
{
	SDL_Event event;
	VisEventQueue *vevent;
	const char *next_plugin;

	while (SDL_PollEvent (&event)) {
		vevent = visual_plugin_get_eventqueue (visual_actor_get_plugin (visual_bin_get_actor (bin)));
		
		switch (event.type) {
			case SDL_KEYUP:
				visual_event_queue_add_keyboard (vevent, event.key.keysym.sym, event.key.keysym.mod, VISUAL_KEY_UP);
				break;
				
			case SDL_KEYDOWN:
				visual_event_queue_add_keyboard (vevent, event.key.keysym.sym, event.key.keysym.mod, VISUAL_KEY_DOWN);
				
				switch (event.key.keysym.sym) {
					/* XMMS CONTROLS */
					case SDLK_UP:
                				xmms_remote_set_main_volume (lv_bmp_vp.xmms_session,
                                             					xmms_remote_get_main_volume (lv_bmp_vp.xmms_session) + 1);
				                break;
					case SDLK_DOWN:
                				xmms_remote_set_main_volume (lv_bmp_vp.xmms_session,
                                             					xmms_remote_get_main_volume (lv_bmp_vp.xmms_session) - 1);
				                break;
            				case SDLK_LEFT:
				                if (xmms_remote_is_playing (lv_bmp_vp.xmms_session))
							xmms_remote_jump_to_time (lv_bmp_vp.xmms_session,
											xmms_remote_get_output_time (lv_bmp_vp.xmms_session) - 5000);
						break;
            				case SDLK_RIGHT:
				                if (xmms_remote_is_playing (lv_bmp_vp.xmms_session))
							xmms_remote_jump_to_time (lv_bmp_vp.xmms_session,
											xmms_remote_get_output_time (lv_bmp_vp.xmms_session) + 5000);
						break;
					case SDLK_z:
						xmms_remote_playlist_prev (lv_bmp_vp.xmms_session);
						break;

					case SDLK_x:
						xmms_remote_play (lv_bmp_vp.xmms_session);
						break;

					case SDLK_c:
						xmms_remote_pause (lv_bmp_vp.xmms_session);
						break;

					case SDLK_v:
						xmms_remote_stop (lv_bmp_vp.xmms_session);
						break;

					case SDLK_b:
						xmms_remote_playlist_next (lv_bmp_vp.xmms_session);
						break;

					/* PLUGIN CONTROLS */
					case SDLK_F11:
					case SDLK_TAB:
						SDL_WM_ToggleFullScreen (screen);
                                                lv_bmp_config_toggle_fullscreen();

						if ((screen->flags & SDL_FULLSCREEN) > 0)
							SDL_ShowCursor (SDL_DISABLE);
						else
							SDL_ShowCursor (SDL_ENABLE);

						break;

					case SDLK_a:
						next_plugin = lv_bmp_config_get_prev_actor ();
						
						if (SDL_MUSTLOCK (screen) == SDL_TRUE)
							SDL_LockSurface (screen);

                                                if (next_plugin != NULL && (strcmp (next_plugin, cur_lv_plugin) != 0)) {
						    lv_bmp_config_set_current_actor (next_plugin);
                                                    cur_lv_plugin = next_plugin;
                                                    visual_bin_set_morph_by_name (bin, lv_bmp_config_morph_plugin());
                                                    visual_bin_switch_actor_by_name (bin, cur_lv_plugin);
                                                }

						SDL_WM_SetCaption (cur_lv_plugin, cur_lv_plugin);
	
						if (SDL_MUSTLOCK (screen) == SDL_TRUE)
							SDL_UnlockSurface (screen);

						break;

					case SDLK_s:
						next_plugin = lv_bmp_config_get_next_actor ();

                                                if (SDL_MUSTLOCK (screen) == SDL_TRUE)
                                                    SDL_LockSurface (screen);

                                                if (next_plugin != NULL && (strcmp (next_plugin, cur_lv_plugin) != 0)) {
						    lv_bmp_config_set_current_actor (next_plugin);
                                                    cur_lv_plugin = next_plugin;
                                                    visual_bin_set_morph_by_name (bin, lv_bmp_config_morph_plugin());
                                                    visual_bin_switch_actor_by_name (bin, cur_lv_plugin);
                                                }

                                                SDL_WM_SetCaption (cur_lv_plugin, cur_lv_plugin);
                                                        
                                                if (SDL_MUSTLOCK (screen) == SDL_TRUE)
                                                    SDL_UnlockSurface (screen);

						break;
						
					default: /* to avoid warnings */
						break;
				}
				break;

			case SDL_VIDEORESIZE:
				visual_resize (event.resize.w, event.resize.h);
				break;

			case SDL_MOUSEMOTION:
				visual_event_queue_add_mousemotion (vevent, event.motion.x, event.motion.y);
				break;

			case SDL_MOUSEBUTTONDOWN:
				visual_event_queue_add_mousebutton (vevent, event.button.button, VISUAL_MOUSE_DOWN,
						event.button.x, event.button.y);
				break;

			case SDL_MOUSEBUTTONUP:
				visual_event_queue_add_mousebutton (vevent, event.button.button, VISUAL_MOUSE_UP,
						event.button.x, event.button.y);
				break;

			case SDL_QUIT:
				GDK_THREADS_ENTER ();
				gtk_idle_add (disable_func, NULL);
				GDK_THREADS_LEAVE ();
				break;
						
			default: /* to avoid warnings */
				break;
		}
	}

	return 0;
}

static gint disable_func (gpointer data)
{
	lv_bmp_vp.disable_plugin (&lv_bmp_vp);

	return FALSE;
}


static void dummy (GtkWidget *widget, gpointer data)
{
}

