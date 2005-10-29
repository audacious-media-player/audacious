
/*
   libmikmod for xmms

   Modified mikmod lib 2/1/99 for xmms by J. Nick Koston (BlueDraco)

   Current Version 0.9.9

   Changes: (1) 2/1/99 Inital Build / Release
   (2) 2/2/99 Psychotron fixed a small problem w/interfacing
   (3) 2/2/99 Shows Channels in spectrum analyzer
   (4) 2/2/99 Fixed the infinate loop problem, and the absured channel
   value problem, removed the mikmod_going stuff
   (5) 2/2/99 Added back in mikmod_going (it is needed)
   (6) 2/2/99 Repaired SEGV on playlist goes to next song, bad
   printf when the mod cannot be loaded and xmms is in repeat mode.
   Also made the absured channel display code work 
   (7) 2/5/99 Removed some old useless stuff
   (8) 2/5/99 Added about -- (doesn't work .. not in xmms yet)
   (9) 2/8/99 Added configure -- (about and configure work now w/a2)

 */

#include "glib/gi18n.h"
#include "mikmod-plugin.h"
#include "libaudacious/configfile.h"
#include "libaudacious/util.h"
#include "libaudacious/titlestring.h"
#include <gtk/gtk.h>

#include "mikmod.xpm"

static void init(void);
static int is_our_file(char *filename);
extern InputPlugin mikmod_ip;
static void play_file(char *filename);
static int get_time(void);
static void stop(void);
static void *play_loop(void *arg);
static void mod_pause(short p);
static void seek(int time);
static void aboutbox(void);
static void get_song_info(char *filename, char **title, int *length);
static void configure();
static void config_ok(GtkWidget * widget, gpointer data);

static pthread_t decode_thread;

MIKMODConfig mikmod_cfg;
extern gboolean mikmod_xmms_audio_error;

/* current module */
static MODULE *mf = NULL;

/* module parameters */
static gboolean cfg_extspd = 1;	/* Extended Speed enable */
static gboolean cfg_panning = 1;	/* DMP panning enable (8xx effects) */
static gboolean cfg_wrap = 0;	/* auto song-wrapping disable */
static gboolean cfg_loop = 0;	/* allow module to loop */
gboolean mikmod_going = 1;

static GtkWidget *Res_16, *Res_8, *Chan_ST, *Chan_MO;

static GtkWidget *Sample_44, *Sample_22, *Sample_11, *Curious_Check, *Surrond_Check, *Fadeout_Check, *Interp_Check;

static GtkObject *pansep_adj;

static GtkWidget *mikmod_conf_window = NULL, *about_window = NULL;

static void *play_loop(void *arg);
static void config_ok(GtkWidget * widget, gpointer data);


static void aboutbox()
{
	GtkWidget *dialog_vbox1;
	GtkWidget *hbox1;
	GtkWidget *label1;
	GtkWidget *dialog_action_area1;
	GtkWidget *about_exit;
	GtkWidget *pixmapwid;
	GdkPixmap *pixmap;
	GdkBitmap *mask;

	if (!about_window)
	{
		about_window = gtk_dialog_new();
		gtk_object_set_data(GTK_OBJECT(about_window), "about_window", about_window);
		gtk_window_set_title(GTK_WINDOW(about_window), _("About mikmod plugin"));
		gtk_window_set_policy(GTK_WINDOW(about_window), FALSE, FALSE, FALSE);
		gtk_window_set_position(GTK_WINDOW(about_window), GTK_WIN_POS_MOUSE);
		gtk_signal_connect(GTK_OBJECT(about_window), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &about_window);
		gtk_container_border_width(GTK_CONTAINER(about_window), 10);

		dialog_vbox1 = GTK_DIALOG(about_window)->vbox;
		gtk_object_set_data(GTK_OBJECT(about_window), "dialog_vbox1", dialog_vbox1);
		gtk_widget_show(dialog_vbox1);
		gtk_container_border_width(GTK_CONTAINER(dialog_vbox1), 5);

		hbox1 = gtk_hbox_new(FALSE, 0);
		gtk_object_set_data(GTK_OBJECT(about_window), "hbox1", hbox1);
		gtk_widget_show(hbox1);
		gtk_box_pack_start(GTK_BOX(dialog_vbox1), hbox1, TRUE, TRUE, 0);
		gtk_container_border_width(GTK_CONTAINER(hbox1), 5);
		gtk_widget_realize(about_window);

		pixmap = gdk_pixmap_create_from_xpm_d(about_window->window, &mask, NULL, mikmod_xpm);
		pixmapwid = gtk_pixmap_new(pixmap, mask);

		gtk_widget_show(pixmapwid);
		gtk_box_pack_start(GTK_BOX(hbox1), pixmapwid, TRUE, TRUE, 0);

		label1 = gtk_label_new(_("Mikmod Plugin\nhttp://www.multimania.com/miodrag/mikmod/\nPorted to xmms by J. Nick Koston"));
		gtk_object_set_data(GTK_OBJECT(about_window), "label1", label1);
		gtk_widget_show(label1);
		gtk_box_pack_start(GTK_BOX(hbox1), label1, TRUE, TRUE, 0);

		dialog_action_area1 = GTK_DIALOG(about_window)->action_area;
		gtk_object_set_data(GTK_OBJECT(about_window), "dialog_action_area1", dialog_action_area1);
		gtk_widget_show(dialog_action_area1);
		gtk_container_border_width(GTK_CONTAINER(dialog_action_area1), 10);

		about_exit = gtk_button_new_with_label(_("Ok"));
		gtk_signal_connect_object(GTK_OBJECT(about_exit), "clicked",
					  GTK_SIGNAL_FUNC
					  (gtk_widget_destroy),
					  GTK_OBJECT(about_window));

		gtk_object_set_data(GTK_OBJECT(about_window), "about_exit", about_exit);
		gtk_widget_show(about_exit);
		gtk_box_pack_start(GTK_BOX(dialog_action_area1), about_exit, TRUE, TRUE, 0);

		gtk_widget_show(about_window);
	}
	else
	{
		gdk_window_raise(about_window->window);
	}

}

static void stop(void)
{
	if (mikmod_going)
	{
		mikmod_going = 0;
		pthread_join(decode_thread, NULL);
	}
}

static void seek(int time)
{
	/*
	   We need to seek in pattrens somehow
	   can't seek by time only by X pattrens on way or the other

	   Player_NextPosition ();
	   Player_PrevPosition ();

	 */
	return;
}

static void mod_pause(short p)
{
	mikmod_ip.output->pause(p);
}

static int get_time(void)
{
	if (mikmod_xmms_audio_error)
		return -2;
	if (!mikmod_going)
		return -1;
	if(!Player_Active() && !mikmod_ip.output->buffer_playing())
		return -1;
	return mikmod_ip.output->output_time();
}

InputPlugin *get_iplugin_info(void)
{
	mikmod_ip.description = g_strdup_printf(_("MikMod Player %s"), VERSION);
	return &mikmod_ip;
}

static void init(void)
{

	ConfigFile *cfg;

	md_device = 0;		/* standard device: autodetect */
	md_reverb = 0;		/* Reverb (max 15) */

	mikmod_cfg.mixing_freq = SAMPLE_FREQ_44;
	mikmod_cfg.volumefadeout = 0;
	mikmod_cfg.surround = 0;
	mikmod_cfg.force8bit = 0;
	mikmod_cfg.hidden_patterns = 0;
	mikmod_cfg.force_mono = 0;
	mikmod_cfg.interpolation = TRUE;
	mikmod_cfg.def_pansep = 64;

	if ((cfg = xmms_cfg_open_default_file()) != NULL)
	{
		xmms_cfg_read_int(cfg, "MIKMOD", "mixing_freq", &mikmod_cfg.mixing_freq);
		xmms_cfg_read_int(cfg, "MIKMOD", "volumefadeout", &mikmod_cfg.volumefadeout);
		xmms_cfg_read_int(cfg, "MIKMOD", "surround", &mikmod_cfg.surround);
		xmms_cfg_read_int(cfg, "MIKMOD", "force8bit", &mikmod_cfg.force8bit);
		xmms_cfg_read_int(cfg, "MIKMOD", "hidden_patterns", &mikmod_cfg.hidden_patterns);
		xmms_cfg_read_int(cfg, "MIKMOD", "force_mono", &mikmod_cfg.force_mono);
		xmms_cfg_read_int(cfg, "MIKMOD", "interpolation", &mikmod_cfg.interpolation);

		xmms_cfg_read_int(cfg, "MIKMOD", "default_panning", &mikmod_cfg.def_pansep);
		xmms_cfg_free(cfg);
	}
	MikMod_RegisterAllLoaders();
	MikMod_RegisterDriver(&drv_xmms);
}

static int is_our_file(char *filename)
{
	char *ext, *tmp;

	tmp = strrchr(filename, '/');
	ext = strrchr(filename, '.');
	
	if (ext)
	{
		if (!strcasecmp(ext, ".669"))
			return 1;
		if (!strcasecmp(ext, ".amf"))
			return 1;
		if (!strcasecmp(ext, ".dsm"))
			return 1;
		if (!strcasecmp(ext, ".far"))
			return 1;
		if (!strcasecmp(ext, ".it"))
			return 1;
		if (!strcasecmp(ext, ".m15"))
			return 1;
		if (!strcasecmp(ext, ".med"))
			return 1;
		if (!strcasecmp(ext, ".mod"))
			return 1;
		if (!strcasecmp(ext, ".mtm"))
			return 1;
		if (!strcasecmp(ext, ".s3m"))
			return 1;
		if (!strcasecmp(ext, ".stm"))
			return 1;
		if (!strcasecmp(ext, ".ult"))
			return 1;
		if (!strcasecmp(ext, ".xm"))
			return 1;
		if (!strcasecmp(ext, ".imf"))
			return 1;
		if (!strcasecmp(ext, ".gdm"))
			return 1;
		if (!strcasecmp(ext, ".stx"))
			return 1;
	}

	if (tmp)
	{
		if(!strncasecmp("/mod.", tmp, 5))
			return 1;
	}

	return 0;
}

static gchar *get_title(gchar *filename)
{
	TitleInput *input;
	gchar *temp, *ext, *title;

	if((temp = Player_LoadTitle(filename)) != NULL)
		title = g_strdup(temp);
	else
	{
		XMMS_NEW_TITLEINPUT(input);

		temp = g_strdup(filename);
		ext = strrchr(temp, '.');
		if (ext)
			*ext = '\0';
		input->file_name = g_basename(temp);
		input->file_ext = ext ? ext+1 : NULL;
		input->file_path = temp;

		title = xmms_get_titlestring(xmms_get_gentitle_format(),
					     input);
		if ( title == NULL )
			title = g_strdup(input->file_name);

		g_free(temp);
		g_free(input);
	}

	return title;
}

static void get_song_info(char *filename, char **title, int *length)
{
	(*title) = get_title(filename);
	(*length) = -1;
}

static void play_file(char *filename)
{
	int cfg_maxchn = 128;
	int uservolume = 128;
	int channelcnt = 1;
	int format = FMT_U8;
	FILE *f;
	
	if(!(f = fopen(filename,"rb")))
	{
		mikmod_going = 0;
		return;
	}
	fclose(f);

	mikmod_xmms_audio_error = FALSE;
	mikmod_going = 1;

	switch (mikmod_cfg.mixing_freq)
	{
		case SAMPLE_FREQ_22:
			md_mixfreq = 22050;	/* 1:2 mixing freq */
			break;
		case SAMPLE_FREQ_11:
			md_mixfreq = 11025;	/* 1:4 mixing freq */
			break;
		default:
			md_mixfreq = 44100;	/* standard mixing freq */
			break;
	}

	md_mode = DMODE_SOFT_MUSIC;
	if ((mikmod_cfg.surround == 1))
	{
		md_mode |= DMODE_SURROUND;
	}
	if ((mikmod_cfg.force8bit == 0))
	{
		format = FMT_S16_NE;
		md_mode |= DMODE_16BITS;
	}
	if ((mikmod_cfg.force_mono == 0))
	{
		channelcnt = DMODE_STEREO;
		md_mode |= DMODE_STEREO;
	}
	if ((mikmod_cfg.interpolation == 1))
	{
		md_mode |= DMODE_INTERP;
	}
	md_pansep = mikmod_cfg.def_pansep;
	
#if (LIBMIKMOD_VERSION > 0x30106)
	MikMod_Init("");
#else
	MikMod_Init();
#endif
	if (!(mf = Player_Load(filename, cfg_maxchn, mikmod_cfg.hidden_patterns)))
	{
		mikmod_ip.set_info_text(_("Couldn't load mod"));
		mikmod_going = 0;
		return;
	}
	mf->extspd = cfg_extspd;
	mf->panflag = cfg_panning;
	mf->wrap = cfg_wrap;
	mf->loop = cfg_loop;
	mf->fadeout = mikmod_cfg.volumefadeout;

	Player_Start(mf);
	
	if (mf->volume > uservolume)
		Player_SetVolume(uservolume);

/* mods are in pattrens .. you need to be able to seek
   back the forth from pattrens */

	mikmod_ip.set_info(mf->songname, -1, ((mf->bpm * 1000)), md_mixfreq, channelcnt);
	pthread_create(&decode_thread, NULL, play_loop, NULL);
	return;

}

static void *play_loop(void *arg)
{
	while (mikmod_going)
	{
		if (Player_Active())
			drv_xmms.Update();
		else
		{
			xmms_usleep(10000);
		}

	}
	Player_Stop();		/* stop playing */
	Player_Free(mf);	/* and free the module */

	mikmod_going = 0;
	MikMod_Exit();
	pthread_exit(NULL);
}

static void configure()
{
	GtkWidget *notebook1, *vbox, *vbox1, *hbox1, *Resolution_Frame, *vbox4;
	GSList *resolution_group = NULL, *vbox5_group = NULL;
	GtkWidget *Channels_Frame, *vbox5, *Downsample_Frame, *vbox3;
	GSList *sample_group = NULL;
	GtkWidget *vbox6, *Quality_Label, *Options_Label;
	GtkWidget *pansep_label, *pansep_hscale;
	GtkWidget *bbox, *ok, *cancel;

	if (!mikmod_conf_window)
	{
		mikmod_conf_window = gtk_window_new(GTK_WINDOW_DIALOG);
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "mikmod_conf_window", mikmod_conf_window);
		gtk_window_set_title(GTK_WINDOW(mikmod_conf_window), _("MikMod Configuration"));
		gtk_window_set_policy(GTK_WINDOW(mikmod_conf_window), FALSE, FALSE, FALSE);
		gtk_window_set_position(GTK_WINDOW(mikmod_conf_window), GTK_WIN_POS_MOUSE);
		gtk_signal_connect(GTK_OBJECT(mikmod_conf_window), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &mikmod_conf_window);
		gtk_container_border_width(GTK_CONTAINER(mikmod_conf_window), 10);

		vbox = gtk_vbox_new(FALSE, 10);
		gtk_container_add(GTK_CONTAINER(mikmod_conf_window), vbox);

		notebook1 = gtk_notebook_new();
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "notebook1", notebook1);
		gtk_widget_show(notebook1);
		gtk_box_pack_start(GTK_BOX(vbox), notebook1, TRUE, TRUE, 0);
		gtk_container_border_width(GTK_CONTAINER(notebook1), 3);

		vbox1 = gtk_vbox_new(FALSE, 0);
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "vbox1", vbox1);
		gtk_widget_show(vbox1);

		hbox1 = gtk_hbox_new(FALSE, 0);
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "hbox1", hbox1);
		gtk_widget_show(hbox1);
		gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);

		Resolution_Frame = gtk_frame_new(_("Resolution:"));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Resolution_Frame", Resolution_Frame);
		gtk_widget_show(Resolution_Frame);
		gtk_box_pack_start(GTK_BOX(hbox1), Resolution_Frame, TRUE, TRUE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(Resolution_Frame), 5);

		vbox4 = gtk_vbox_new(FALSE, 0);
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "vbox4", vbox4);
		gtk_widget_show(vbox4);
		gtk_container_add(GTK_CONTAINER(Resolution_Frame), vbox4);

		Res_16 = gtk_radio_button_new_with_label(resolution_group, _("16 bit"));
		resolution_group = gtk_radio_button_group(GTK_RADIO_BUTTON(Res_16));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Res_16", Res_16);
		gtk_widget_show(Res_16);
		gtk_box_pack_start(GTK_BOX(vbox4), Res_16, TRUE, TRUE, 0);
		if (mikmod_cfg.force8bit == 0)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Res_16), TRUE);

		Res_8 = gtk_radio_button_new_with_label(resolution_group, _("8 bit"));
		resolution_group = gtk_radio_button_group(GTK_RADIO_BUTTON(Res_8));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Res_8", Res_8);
		gtk_widget_show(Res_8);
		gtk_box_pack_start(GTK_BOX(vbox4), Res_8, TRUE, TRUE, 0);
		if (mikmod_cfg.force8bit == 1)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Res_8), TRUE);

		Channels_Frame = gtk_frame_new(_("Channels:"));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Channels_Frame", Channels_Frame);
		gtk_widget_show(Channels_Frame);
		gtk_box_pack_start(GTK_BOX(hbox1), Channels_Frame, TRUE, TRUE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(Channels_Frame), 5);

		vbox5 = gtk_vbox_new(FALSE, 0);
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "vbox5", vbox5);
		gtk_widget_show(vbox5);
		gtk_container_add(GTK_CONTAINER(Channels_Frame), vbox5);

		Chan_ST = gtk_radio_button_new_with_label(vbox5_group, _("Stereo"));
		vbox5_group = gtk_radio_button_group(GTK_RADIO_BUTTON(Chan_ST));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Chan_ST", Chan_ST);
		gtk_widget_show(Chan_ST);
		gtk_box_pack_start(GTK_BOX(vbox5), Chan_ST, TRUE, TRUE, 0);
		if (mikmod_cfg.force_mono == 0)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Chan_ST), TRUE);

		Chan_MO = gtk_radio_button_new_with_label(vbox5_group, _("Mono"));
		vbox5_group = gtk_radio_button_group(GTK_RADIO_BUTTON(Chan_MO));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Chan_MO", Chan_MO);
		gtk_widget_show(Chan_MO);
		gtk_box_pack_start(GTK_BOX(vbox5), Chan_MO, TRUE, TRUE, 0);
		if (mikmod_cfg.force_mono == 1)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Chan_MO), TRUE);

		Downsample_Frame = gtk_frame_new(_("Downsample:"));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Downsample_Frame", Downsample_Frame);
		gtk_widget_show(Downsample_Frame);
		gtk_box_pack_start(GTK_BOX(vbox1), Downsample_Frame, TRUE, TRUE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(Downsample_Frame), 5);

		vbox3 = gtk_vbox_new(FALSE, 0);
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "vbox3", vbox3);
		gtk_widget_show(vbox3);
		gtk_container_add(GTK_CONTAINER(Downsample_Frame), vbox3);

		Sample_44 = gtk_radio_button_new_with_label(sample_group, _("1:1 (44 kHz)"));
		sample_group = gtk_radio_button_group(GTK_RADIO_BUTTON(Sample_44));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Sample_44", Sample_44);
		gtk_widget_show(Sample_44);
		gtk_box_pack_start(GTK_BOX(vbox3), Sample_44, TRUE, TRUE, 0);

		if (mikmod_cfg.mixing_freq == SAMPLE_FREQ_44)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Sample_44), TRUE);

		Sample_22 = gtk_radio_button_new_with_label(sample_group, _("1:2 (22 kHz)"));
		sample_group = gtk_radio_button_group(GTK_RADIO_BUTTON(Sample_22));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Sample_22", Sample_22);
		gtk_widget_show(Sample_22);
		gtk_box_pack_start(GTK_BOX(vbox3), Sample_22, TRUE, TRUE, 0);
		if (mikmod_cfg.mixing_freq == SAMPLE_FREQ_22)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Sample_22), TRUE);

		Sample_11 = gtk_radio_button_new_with_label(sample_group, _("1:4 (11 kHz)"));
		sample_group = gtk_radio_button_group(GTK_RADIO_BUTTON(Sample_11));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Sample_11", Sample_11);
		gtk_widget_show(Sample_11);
		gtk_box_pack_start(GTK_BOX(vbox3), Sample_11, TRUE, TRUE, 0);
		if (mikmod_cfg.mixing_freq == SAMPLE_FREQ_11)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Sample_11), TRUE);

		vbox6 = gtk_vbox_new(FALSE, 0);
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "vbox6", vbox6);
		gtk_widget_show(vbox6);

		Curious_Check = gtk_check_button_new_with_label(_("Look for hidden patterns in modules "));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Curious_Check", Curious_Check);
		gtk_widget_show(Curious_Check);
		gtk_box_pack_start(GTK_BOX(vbox6), Curious_Check, TRUE, TRUE, 0);
		if (mikmod_cfg.hidden_patterns == 1)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Curious_Check), TRUE);

		Surrond_Check = gtk_check_button_new_with_label(_("Use surround mixing"));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Surround_Check", Surrond_Check);
		gtk_widget_show(Surrond_Check);
		gtk_box_pack_start(GTK_BOX(vbox6), Surrond_Check, TRUE, TRUE, 0);
		if (mikmod_cfg.surround == 1)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Surrond_Check), TRUE);

		Fadeout_Check = gtk_check_button_new_with_label(_("Force volume fade at the end of the module"));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Fadeout_Check", Fadeout_Check);
		gtk_widget_show(Fadeout_Check);
		gtk_box_pack_start(GTK_BOX(vbox6), Fadeout_Check, TRUE, TRUE, 0);
		if (mikmod_cfg.volumefadeout == 1)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Fadeout_Check), TRUE);

		Interp_Check = gtk_check_button_new_with_label(_("Use interpolation"));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Interp_Check", Interp_Check);
		gtk_widget_show(Interp_Check);
		gtk_box_pack_start(GTK_BOX(vbox6), Interp_Check, TRUE, TRUE, 0);
		if (mikmod_cfg.interpolation == 1)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Interp_Check), TRUE);

		pansep_label = gtk_label_new(_("Default panning separation"));
		gtk_widget_show(pansep_label);
		gtk_box_pack_start(GTK_BOX(vbox6), pansep_label, TRUE, TRUE, 0);

		pansep_adj = gtk_adjustment_new(mikmod_cfg.def_pansep, 0.0, 129.0, 1.0, 8.0, 1.0);
		pansep_hscale = gtk_hscale_new(GTK_ADJUSTMENT(pansep_adj));
		gtk_scale_set_digits(GTK_SCALE(pansep_hscale), 0);
		gtk_scale_set_draw_value(GTK_SCALE(pansep_hscale), TRUE);
		gtk_scale_set_value_pos(GTK_SCALE(pansep_hscale), GTK_POS_BOTTOM);
		gtk_widget_show(pansep_hscale);
		gtk_box_pack_start(GTK_BOX(vbox6), pansep_hscale, TRUE, TRUE, 0);

		Quality_Label = gtk_label_new(_("Quality"));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Quality_Label", Quality_Label);
		gtk_widget_show(Quality_Label);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook1), vbox1, Quality_Label);

		Options_Label = gtk_label_new(_("Options"));
		gtk_object_set_data(GTK_OBJECT(mikmod_conf_window), "Options_Label", Options_Label);
		gtk_widget_show(Options_Label);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook1), vbox6, Options_Label);

		bbox = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
		gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
		gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

		ok = gtk_button_new_with_label(_("Ok"));
		gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(config_ok), NULL);
		GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
		gtk_widget_show(ok);
		gtk_widget_grab_default(ok);

		cancel = gtk_button_new_with_label(_("Cancel"));
		gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(mikmod_conf_window));
		GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
		gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);
		gtk_widget_show(cancel);

		gtk_widget_show(bbox);

		gtk_widget_show(vbox);
		gtk_widget_show(mikmod_conf_window);
	}
	else
	{
		gdk_window_raise(mikmod_conf_window->window);
	}

}

static void config_ok(GtkWidget * widget, gpointer data)
{
	ConfigFile *cfg;
	gchar *filename;

	if (GTK_TOGGLE_BUTTON(Res_8)->active)
		mikmod_cfg.force8bit = 1;
	else
		mikmod_cfg.force8bit = 0;

	if (GTK_TOGGLE_BUTTON(Chan_MO)->active)
		mikmod_cfg.force_mono = 1;
	else
		mikmod_cfg.force_mono = 0;

	if (GTK_TOGGLE_BUTTON(Sample_22)->active)
		mikmod_cfg.mixing_freq = SAMPLE_FREQ_22;
	else if (GTK_TOGGLE_BUTTON(Sample_11)->active)
		mikmod_cfg.mixing_freq = SAMPLE_FREQ_11;
	else
		mikmod_cfg.mixing_freq = SAMPLE_FREQ_44;


	mikmod_cfg.hidden_patterns = GTK_TOGGLE_BUTTON(Curious_Check)->active;
	mikmod_cfg.surround = GTK_TOGGLE_BUTTON(Surrond_Check)->active;
	mikmod_cfg.volumefadeout = GTK_TOGGLE_BUTTON(Fadeout_Check)->active;
	mikmod_cfg.interpolation = GTK_TOGGLE_BUTTON(Interp_Check)->active;

	mikmod_cfg.def_pansep = (guchar)GTK_ADJUSTMENT(pansep_adj)->value;
	md_pansep = mikmod_cfg.def_pansep;

	filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
	cfg = xmms_cfg_open_file(filename);
	if (!cfg)
		cfg = xmms_cfg_new();

	xmms_cfg_write_int(cfg, "MIKMOD", "mixing_freq", mikmod_cfg.mixing_freq);
	xmms_cfg_write_int(cfg, "MIKMOD", "volumefadeout", mikmod_cfg.volumefadeout);
	xmms_cfg_write_int(cfg, "MIKMOD", "surround", mikmod_cfg.surround);
	xmms_cfg_write_int(cfg, "MIKMOD", "force8bit", mikmod_cfg.force8bit);
	xmms_cfg_write_int(cfg, "MIKMOD", "hidden_patterns", mikmod_cfg.hidden_patterns);
	xmms_cfg_write_int(cfg, "MIKMOD", "force_mono", mikmod_cfg.force_mono);
	xmms_cfg_write_int(cfg, "MIKMOD", "interpolation", mikmod_cfg.interpolation);
	xmms_cfg_write_int(cfg, "MIKMOD", "panning_separation", mikmod_cfg.def_pansep);

	xmms_cfg_write_file(cfg, filename);
	xmms_cfg_free(cfg);
	g_free(filename);
	gtk_widget_destroy(mikmod_conf_window);
}

InputPlugin mikmod_ip =
{
	NULL,
	NULL,
	NULL, /* Description */
	init,
	aboutbox,
	configure,
	is_our_file,
	NULL,
	play_file,
	stop,
	mod_pause,
	seek,
	NULL,
	get_time,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	get_song_info,
	NULL,			/* file_info_box */
	NULL
};
