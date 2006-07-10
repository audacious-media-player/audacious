/*      xmms - jack output plugin
 *	Copyright 2002 Chris Morgan<cmorgan@alum.wpi.edu>
 *
 *      audacious port (2005) by Giacomo Lozito from develia.org
 *
 *	This code maps xmms calls into the jack translation library
 */

#include "libaudacious/configdb.h"
#include "libaudacious/util.h"
#include <dlfcn.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include "config.h"
#include "bio2jack.h" /* includes for the bio2jack library */
#include "jack.h"
#include "xconvert.h" /* xmms rate conversion header file */
#include <string.h>



/* set to 1 for verbose output */
#define VERBOSE_OUTPUT          0

jackconfig jack_cfg;

#define OUTFILE stderr

#define TRACE(...)                                      \
    if(jack_cfg.isTraceEnabled) {                       \
        fprintf(OUTFILE, "%s:", __FUNCTION__),          \
        fprintf(OUTFILE, __VA_ARGS__),				    \
        fflush(OUTFILE);                                \
    }

#define ERR(...)                                        \
    if(jack_cfg.isTraceEnabled) {                       \
        fprintf(OUTFILE, "ERR: %s:", __FUNCTION__),     \
        fprintf(OUTFILE, __VA_ARGS__),				    \
        fflush(OUTFILE);                                \
    }


static int driver = 0; /* handle to the jack output device */

typedef struct format_info {
  AFormat format;
  long    frequency;
  int     channels;
  long    bps;
} format_info_t;

static format_info_t input;
static format_info_t effect;
static format_info_t output;

static convert_freq_func_t freq_convert; /* rate convert function */
static struct xmms_convert_buffers *convertb; /* convert buffer */

#define MAKE_FUNCPTR(f) static typeof(f) * fp_##f = NULL;
MAKE_FUNCPTR(xmms_convert_buffers_new);
MAKE_FUNCPTR(xmms_convert_buffers_destroy);
MAKE_FUNCPTR(xmms_convert_get_frequency_func);
void *xmmslibhandle; /* handle to the dlopen'ed libxmms.so */

static int isXmmsFrequencyAvailable = 0;

static gboolean output_opened; /* true if we have a connection to jack */

static GtkWidget *dialog, *button, *label;


/* Giacomo's note: removed the destructor from the original xmms-jack, cause
   destructors + thread join + NPTL currently leads to problems; solved this
   by adding a cleanup function callback for output plugins in Audacious, this
   is used to close the JACK connection and to perform a correct shutdown */
void jack_cleanup(void)
{
  int errval;
  TRACE("cleanup\n");

  if((errval = JACK_Close(driver)))
    ERR("error closing device, errval of %d\n", errval);

  /* only clean this up if we have the function to call */
  if(isXmmsFrequencyAvailable)
  {
    fp_xmms_convert_buffers_destroy(convertb); /* clean up the rate conversion buffers */
    dlclose(xmmslibhandle);
  }

  return;
}


void jack_sample_rate_error(void)
{
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), ("Sample rate mismatch"));
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);
	label = gtk_label_new((
		"Xmms is asking for a sample rate that differs from\n "
		"that of the jack server.  Xmms 1.2.8 or later\n"
		"contains resampling routines that xmms-jack will\n"
		"dynamically load and use to perform resampling.\n"
		"Or you can restart the jack server\n"
		"with a sample rate that matches the one that\n"
		"xmms desires.  -r is the option for the jack\n"
		"alsa driver so -r 44100 or -r 48000 should do\n\n"
		"Chris Morgan <cmorgan@alum.wpi.edu>\n"));

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 0);
	gtk_widget_show(label);

	button = gtk_button_new_with_label((" Close "));
	gtk_signal_connect_object(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(dialog));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	gtk_widget_show(dialog);
	gtk_widget_grab_focus(button);
}


/* Return the number of milliseconds of audio data that has been */
/* written out to the device */
gint jack_get_written_time(void)
{
  long return_val;
  return_val = JACK_GetPosition(driver, MILLISECONDS, WRITTEN);

  TRACE("returning %ld milliseconds\n", return_val);
  return return_val;
}


/* Return the current number of milliseconds of audio data that has */
/* been played out of the audio device, not including the buffer */
gint jack_get_output_time(void)
{
  gint return_val;
  
  /* don't try to get any values if the device is still closed */
  if(JACK_GetState(driver) == CLOSED)
    return_val = 0;
  else
    return_val = JACK_GetPosition(driver, MILLISECONDS, PLAYED); /* get played position in terms of milliseconds */

  TRACE("returning %d milliseconds\n", return_val);
  return return_val;
}


/* returns TRUE if we are currently playing */
/* NOTE: this was confusing at first BUT, if the device is open and there */
/* is no more audio to be played, then the device is NOT PLAYING */
gint jack_playing(void)
{
  gint return_val;

  /* If we are playing see if we ACTUALLY have something to play */
  if(JACK_GetState(driver) == PLAYING)
  {
    /* If we have zero bytes stored, we are done playing */
    if(JACK_GetBytesStored(driver) == 0)
      return_val = FALSE;
    else
      return_val = TRUE;
  }
  else
    return_val = FALSE;

  TRACE("returning %d\n", return_val);
  return return_val;
}


void jack_set_port_connection_mode()
{
  /* setup the port connection mode that determines how bio2jack will connect ports */
  enum JACK_PORT_CONNECTION_MODE mode;
  
  if(strcmp(jack_cfg.port_connection_mode, "CONNECT_ALL") == 0)
      mode = CONNECT_ALL;
  else if(strcmp(jack_cfg.port_connection_mode, "CONNECT_OUTPUT") == 0)
      mode = CONNECT_OUTPUT;
  else if(strcmp(jack_cfg.port_connection_mode, "CONNECT_NONE") == 0)
      mode = CONNECT_NONE;
  else
  {
      TRACE("Defaulting to CONNECT_ALL");
      mode = CONNECT_ALL;
  }
  JACK_SetPortConnectionMode(mode);
}

/* Initialize necessary things */
void jack_init(void)
{
  /* read the isTraceEnabled setting from the config file */
  ConfigDb *cfgfile;

  cfgfile = bmp_cfg_db_open();
  if (!cfgfile)
  {
      jack_cfg.isTraceEnabled = FALSE;
      jack_cfg.port_connection_mode = "CONNECT_ALL"; /* default to connect all */
      jack_cfg.volume_left = 25; /* set default volume to 25 % */
      jack_cfg.volume_right = 25;
  } else
  {
      bmp_cfg_db_get_bool(cfgfile, "jack", "isTraceEnabled", &jack_cfg.isTraceEnabled);
      if(!bmp_cfg_db_get_string(cfgfile, "jack", "port_connection_mode", &jack_cfg.port_connection_mode))
          jack_cfg.port_connection_mode = "CONNECT_ALL";
      if(!bmp_cfg_db_get_int(cfgfile, "jack", "volume_left", &jack_cfg.volume_left))
          jack_cfg.volume_left = 25;
      if(!bmp_cfg_db_get_int(cfgfile, "jack", "volume_right", &jack_cfg.volume_right))
          jack_cfg.volume_right = 25;
  }

  bmp_cfg_db_close(cfgfile);

  TRACE("initializing\n");
  JACK_Init(); /* initialize the driver */

  /* set the bio2jack name so users will see xmms-jack in their */
  /* jack client list */
  JACK_SetClientName("audacious-jack");

  /* set the port connection mode */
  jack_set_port_connection_mode();

  xmmslibhandle = dlopen("libaudacious.so", RTLD_NOW);
  if(xmmslibhandle)
  {
    fp_xmms_convert_buffers_new = dlsym(xmmslibhandle, "xmms_convert_buffers_new");
    fp_xmms_convert_buffers_destroy = dlsym(xmmslibhandle, "xmms_convert_buffers_destroy");
    fp_xmms_convert_get_frequency_func = dlsym(xmmslibhandle, "xmms_convert_get_frequency_func");

    if(!fp_xmms_convert_buffers_new)
    {
      TRACE("fp_xmms_convert_buffers_new couldn't be dlsym'ed\n");
      TRACE("dlerror: %s\n", dlerror());
    }

    if(!fp_xmms_convert_buffers_destroy)
    {
      TRACE("fp_xmms_convert_buffers_destroy couldn't be dlsym'ed\n");
      TRACE("dlerror: %s\n", dlerror());
    }

    if(!fp_xmms_convert_get_frequency_func)
    {
      TRACE("fp_xmms_get_frequency_func couldn't be dlsym'ed\n");
      TRACE("dlerror: %s\n", dlerror());
    }

    if(!fp_xmms_convert_buffers_new || !fp_xmms_convert_buffers_destroy ||
       !fp_xmms_convert_get_frequency_func)
    {
      dlclose(xmmslibhandle); /* close the library, no need to keep it open */
      TRACE("One or more frequency convertion functions are missing, upgrade to xmms >=1.2.8\n");
    } else
    {
      TRACE("Found frequency convertion functions, setting isXmmsFrequencyAvailable to 1\n");
      isXmmsFrequencyAvailable = 1;
    }
  } else
  {
    TRACE("unable to dlopen '%s'\n", "libaudacious.so");
  }

  /* only initialize this stuff if we have the functions available */
  if(isXmmsFrequencyAvailable)
  {
    convertb = fp_xmms_convert_buffers_new ();
    freq_convert = fp_xmms_convert_get_frequency_func(FMT_S16_LE, 2);
  }

  output_opened = FALSE;
}


/* Return the amount of data that can be written to the device */
gint jack_free(void)
{
  unsigned long return_val = JACK_GetBytesFreeSpace(driver);
  unsigned long tmp;

  /* adjust for frequency differences, otherwise xmms could send us */
  /* as much data as we have free, then we go to convert this to */
  /* the output frequency and won't have enough space, so adjust */
  /* by the ratio of the two */
  if(effect.frequency != output.frequency)
  {
    tmp = return_val;
    return_val = (return_val * effect.frequency) / output.frequency;
    TRACE("adjusting from %ld to %ld free bytes to compensate for frequency differences\n", tmp, return_val);
  }

  if(return_val > G_MAXINT)
  {
      TRACE("Warning: return_val > G_MAXINT\n");
      return_val = G_MAXINT;
  }

  TRACE("free space of %ld bytes\n", return_val);

  return return_val;
}


/* Close the device */
void jack_close(void)
{
  ConfigDb *cfgfile;

  cfgfile = bmp_cfg_db_open();
  bmp_cfg_db_set_int(cfgfile, "jack", "volume_left", jack_cfg.volume_left); /* stores the volume setting */
  bmp_cfg_db_set_int(cfgfile, "jack", "volume_right", jack_cfg.volume_right);
  bmp_cfg_db_close(cfgfile);

  TRACE("\n");

  JACK_Reset(driver); /* flush buffers, reset position and set state to STOPPED */
  TRACE("resetting driver, not closing now, destructor will close for us\n");
}


/* Open the device up */
gint jack_open(AFormat fmt, gint sample_rate, gint num_channels)
{
  int bits_per_sample;
  int retval;
  unsigned long rate;

  TRACE("fmt == %d, sample_rate == %d, num_channels == %d\n",
	fmt, sample_rate, num_channels);

  if((fmt == FMT_U8) || (fmt == FMT_S8))
  {
    bits_per_sample = 8;
  } else
  {
    bits_per_sample = 16;
  }

  /* record some useful information */
  input.format    = fmt;
  input.frequency = sample_rate;
  input.bps       = bits_per_sample * sample_rate * num_channels;
  input.channels  = num_channels;

  /* setup the effect as matching the input format */
  effect.format    = input.format;
  effect.frequency = input.frequency;
  effect.channels  = input.channels;
  effect.bps       = input.bps;

  /* if we are already opened then don't open again */
  if(output_opened)
  {
    /* if something has changed we should close and re-open the connect to jack */
    if((output.channels != input.channels) ||
       (output.frequency != input.frequency) ||
       (output.format != input.format))
    {
      TRACE("output.channels is %d, jack_open called with %d channels\n", output.channels, input.channels);
      TRACE("output.frequency is %ld, jack_open called with %ld\n", output.frequency, input.frequency);
      TRACE("output.format is %d, jack_open called with %d\n", output.format, input.format);
      jack_close();
    } else
    {
        TRACE("output_opened is TRUE and no options changed, not reopening\n");
        return 1;
    }
  }

  /* try to open the jack device with the requested rate at first */
  output.frequency = input.frequency;
  output.bps       = input.bps;
  output.channels  = input.channels;
  output.format    = input.format;

  rate = output.frequency;
  retval = JACK_Open(&driver, bits_per_sample, &rate, output.channels);
  output.frequency = rate; /* avoid compile warning as output.frequency differs in type
                              from what JACK_Open() wants for the type of the rate parameter */
  if((retval == ERR_RATE_MISMATCH) && isXmmsFrequencyAvailable)
  {
    TRACE("xmms(input) wants rate of '%ld', jacks rate(output) is '%ld', opening at jack rate\n", input.frequency, output.frequency);

    /* open the jack device with true jack's rate, return 0 upon failure */
    retval = JACK_Open(&driver, bits_per_sample, &rate, output.channels);
    output.frequency = rate; /* avoid compile warning as output.frequency differs in type
                                from what JACK_Open() wants for the type of the rate parameter */
    if(retval)
    {
      TRACE("failed to open jack with JACK_Open(), error %d\n", retval);
      return 0;
    }
    TRACE("success!!\n");
  } else if((retval == ERR_RATE_MISMATCH) && !isXmmsFrequencyAvailable)
  {
    TRACE("JACK_Open(), sample rate mismatch with no resampling routines available\n");

    jack_sample_rate_error(); /* notify the user that we can't resample */

    return 0;
  } else if(retval != ERR_SUCCESS)
  {
    TRACE("failed to open jack with JACK_Open(), error %d\n", retval);
    return 0;
  }

  jack_set_volume(jack_cfg.volume_left, jack_cfg.volume_right); /* sets the volume to stored value */
  output_opened = TRUE;

  return 1;
}


/* write some audio out to the device */
void jack_write(gpointer ptr, gint length)
{
  long written;
  EffectPlugin *ep;
  AFormat new_format;
  int new_frequency, new_channels;
  long positionMS;

  TRACE("starting length of %d\n", length);

  /* copy the current values into temporary values */
  new_format = input.format;
  new_frequency = input.frequency;
  new_channels = input.channels;

  /* query xmms for the current plugin */
  ep = get_current_effect_plugin();
  if(effects_enabled() && ep && ep->query_format)
  {
    ep->query_format(&new_format, &new_frequency, &new_channels);
  }

  /* if the format has changed take this into account by modifying */
  /* the time offset and reopening the device with the new format settings */
  if (new_format != effect.format ||
      new_frequency != effect.frequency ||
      new_channels != effect.channels)
  {
    TRACE("format changed, storing new values and opening/closing jack\n");
    TRACE("effect.format == %d, new_format == %d, effect.frequency == %ld, new_frequency == %d, effect.channels == %d, new_channels = %d\n",
	  effect.format, new_format, effect.frequency, new_frequency, effect.channels, new_channels);
 
    positionMS = JACK_GetPosition(driver, MILLISECONDS, PLAYED);

    jack_close();
    jack_open(new_format, new_frequency, new_channels);
 
    /* restore the position after the open and close */
    JACK_SetState(driver, PAUSED);
    JACK_SetPosition(driver, MILLISECONDS, positionMS);
    JACK_SetState(driver, PLAYING);
  }

  /* if effects are enabled and we have a plugin, run the current */
  /* samples through the plugin */
  if (effects_enabled() && ep && ep->mod_samples)
  {
    length = ep->mod_samples(&ptr, length,
                             input.format,
                             input.frequency,
                             input.channels);
    TRACE("effects_enabled(), length is now %d\n", length);
  }

  TRACE("effect.frequency == %ld, input.frequency == %ld, output.frequency == %ld\n",
        effect.frequency, input.frequency, output.frequency);

  /* if we need rate conversion, perform it here */
  if((effect.frequency != output.frequency) && isXmmsFrequencyAvailable)
  {
    TRACE("performing rate conversion from '%ld'(effect) to '%ld'(output)\n", effect.frequency, output.frequency);
    length = freq_convert (convertb, &ptr, length, effect.frequency, output.frequency);
  }

  TRACE("length = %d\n", length);
  /* loop until we have written all the data out to the jack device */
  /* this is due to xmms' audio driver api */
  char *buf = (char*)ptr;
  while(length > 0)
  {
    TRACE("writing %d bytes\n", length);
    written = JACK_Write(driver, (unsigned char*)buf, length);
    length-=written;
    buf+=written;
  }
  TRACE("finished\n");
}


/* Flush any output currently buffered */
/* and set the number of bytes written based on ms_offset_time, */
/* the number of milliseconds of offset passed in */
/* This is done so the driver itself keeps track of */
/* current playing position of the mp3 */
void jack_flush(gint ms_offset_time)
{
  TRACE("setting values for ms_offset_time of %d\n", ms_offset_time);

  JACK_Reset(driver); /* flush buffers and set state to STOPPED */

  /* update the internal driver values to correspond to the input time given */
  JACK_SetPosition(driver, MILLISECONDS, ms_offset_time);

  JACK_SetState(driver, PLAYING);
}


/* Pause the jack device */
void jack_pause(short p)
{
  TRACE("p == %d\n", p);

  /* pause the device if p is non-zero, unpause the device if p is zero and */
  /* we are currently paused */
  if(p)
    JACK_SetState(driver, PAUSED);
  else if(JACK_GetState(driver) == PAUSED)
    JACK_SetState(driver, PLAYING);
}


/* Set the volume */
void jack_set_volume(int l, int r)
{
  if(output.channels == 1)
  {
    TRACE("l(%d)\n", l);
  } else if(output.channels > 1)
  {
    TRACE("l(%d), r(%d)\n", l, r);
  }

  if(output.channels > 0) {
      JACK_SetVolumeForChannel(driver, 0, l);
      jack_cfg.volume_left = l;
  }
  if(output.channels > 1) {
      JACK_SetVolumeForChannel(driver, 1, r);
      jack_cfg.volume_right = r;
  }
}


/* Get the current volume setting */
void jack_get_volume(int *l, int *r)
{
  unsigned int _l, _r;

  if(output.channels > 0)
  {
      JACK_GetVolumeForChannel(driver, 0, &_l);
      (*l) = _l;
  }
  if(output.channels > 1)
  {
      JACK_GetVolumeForChannel(driver, 1, &_r);
      (*r) = _r;
  }

#if VERBOSE_OUTPUT
  if(output.channels == 1)
      TRACE("l(%d)\n", *l);
  else if(output.channels > 1)
      TRACE("l(%d), r(%d)\n", *l, *r);
#endif
}


void jack_about(void)
{
	static GtkWidget *aboutbox;
	
	if (!aboutbox)
	{
		aboutbox = xmms_show_message(
			_("About JACK Output Plugin 0.15"),
			_("XMMS jack Driver 0.15\n\n"
			  "xmms-jack.sf.net\nChris Morgan<cmorgan@alum.wpi.edu>\n\n"
			  "Audacious port by\nGiacomo Lozito from develia.org"),
			_("Ok"), FALSE, NULL, NULL);
		g_signal_connect(GTK_OBJECT(aboutbox), "destroy",
				   (GCallback)gtk_widget_destroyed, &aboutbox);
	}
}

static void
jack_tell_audio(AFormat * fmt, gint * srate, gint * nch)
{
	(*fmt) = input.format;
	(*srate) = input.frequency;
	(*nch) = input.channels;
}

OutputPlugin jack_op =
{
	NULL,
	NULL,
	"JACK Output Plugin 0.15",
	jack_init,
	jack_cleanup,
	jack_about,
	jack_configure,
	jack_get_volume,
	jack_set_volume,
	jack_open,
	jack_write,
	jack_close,
	jack_flush,
	jack_pause,
	jack_free,
	jack_playing,
	jack_get_output_time,
	jack_get_written_time,
	jack_tell_audio
};


OutputPlugin *get_oplugin_info(void)
{
	return &jack_op;
}
