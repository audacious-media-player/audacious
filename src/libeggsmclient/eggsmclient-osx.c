/*
 * Copyright (C) 2007 Novell, Inc.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#define	G_LOG_DOMAIN "EggSMClient"

#include "eggsmclient.h"

#define EGG_TYPE_SM_CLIENT_OSX            (egg_sm_client_osx_get_type ())
#define EGG_SM_CLIENT_OSX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_SM_CLIENT_OSX, EggSMClientOSX))
#define EGG_SM_CLIENT_OSX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_SM_CLIENT_OSX, EggSMClientOSXClass))
#define EGG_IS_SM_CLIENT_OSX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_SM_CLIENT_OSX))
#define EGG_IS_SM_CLIENT_OSX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EGG_TYPE_SM_CLIENT_OSX))
#define EGG_SM_CLIENT_OSX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_SM_CLIENT_OSX, EggSMClientOSXClass))

typedef struct _EggSMClientOSX        EggSMClientOSX;
typedef struct _EggSMClientOSXClass   EggSMClientOSXClass;

struct _EggSMClientOSX {
  EggSMClient parent;

}

struct _EggSMClientOSXClass
{
  EggSMClientClass parent_class;

};

static void     sm_client_osx_startup (EggSMClient *client,
				       const char  *client_id);
static void     sm_client_osx_will_quit (EggSMClient *client,
					 gboolean     will_quit);
static gboolean sm_client_osx_end_session (EggSMClient         *client,
					   EggSMClientEndStyle  style,
					   gboolean  request_confirmation);

static GdkFilterReturn sm_client_osx_filter (GdkXEvent *xevent,
					     GdkEvent  *event,
					     gpointer   data);

G_DEFINE_TYPE (EggSMClientOSX, egg_sm_client_osx, EGG_TYPE_SM_CLIENT)

static void
egg_sm_client_osx_init (EggSMClientOSX *osxclient)
{
  ;
}

static void
egg_sm_client_osx_class_init (EggSMClientOSXClass *klass)
{
  EggSMClientClass *sm_client_class = EGG_SM_CLIENT_CLASS (klass);

  sm_client_class->startup             = sm_client_osx_startup;
  sm_client_class->will_quit           = sm_client_osx_will_quit;
  sm_client_class->end_session         = sm_client_osx_end_session;
}

EggSMClient *
egg_sm_client_osx_new (void)
{
  return g_object_new (EGG_TYPE_SM_CLIENT_OSX, NULL);
}

static void
sm_client_osx_startup (EggSMClient *client)
{
  gdk_window_add_filter (NULL, sm_client_osx_filter, client);
}

void
sm_client_osx_will_quit (EggSMClient *client,
			 gboolean     will_quit)
{
  EggSMClientOSX *osxclient = (EggSMClientOSX *)client;

  if (will_quit)
    {
      /* OS X doesn't send another message. We're supposed to just
       * quit after agreeing that it's OK.
       *
       * FIXME: do it from an idle handler though.
       */
      egg_sm_client_quit (client);
    }
  else
    {
      /* FIXME: "respond to the event by returning a userCancelledErr
       * error"
       */
    }
}

void
sm_client_osx_end_session (EggSMClient         *client,
			   EggSMClientEndStyle  style,
			   gboolean             request_confirmation)
{
  EggSMClientOSX *osxclient = (EggSMClientOSX *)client;
  FIXME_t event;

  switch (style)
    {
    case EGG_SM_CLIENT_END_SESSION_DEFAULT:
    case EGG_SM_CLIENT_END_LOGOUT:
      event = request_confirmation ? kAELogOut : kAEReallyLogOut;
      break;
    case EGG_SM_CLIENT_END_REBOOT:
      event = request_confirmation ? kAEShowRestartDialog : kAERestart;
      break;
    case EGG_SM_CLIENT_END_SHUTDOWN:
      event = request_confirmation ? kAEShowShutdownDialog : kAEShutDown;
      break;
    }

  /* FIXME: send event to loginwindow process */
}

static GdkFilterReturn
egg_sm_client_osx_filter (GdkXEvent *xevent,
			  GdkEvent  *event,
			  gpointer   data)
{
  EggSMClientOSX *osxclient = data;
  EggSMClient *client = data;
  NSEvent *nsevent = (NSEvent *)xevent;

  switch (FIXME_get_apple_event_type (nsevent))
    {
    case kAEQuitApplication:
      if (FIXME_app_is_a_foreground_app)
	egg_sm_client_quit_requested (client);
      else
	egg_sm_client_quit (client);
      return GDK_FILTER_REMOVE;

    default:
      return GDK_FILTER_CONTINUE;
    }
}
