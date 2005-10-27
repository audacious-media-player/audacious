#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libaudacious/configfile.h>
#include <audacious/util.h>
#include <libvisual/libvisual.h>
#include <glib/gi18n.h>

#include "config.h"

#include "lv_bmp_config.h"
#include "config_gui.h"

#define CONFIG_DEFAULT_ACTOR_PLUGIN "infinite"
#define CONFIG_DEFAULT_INPUT_PLUGIN "esd"
#define CONFIG_DEFAULT_MORPH_PLUGIN "alphablend"

static const Options default_options = { NULL, NULL, NULL, 320, 200, 30, 24, FALSE, FALSE, FALSE, TRUE, FALSE };
static Options options = { NULL, NULL, NULL, -1, -1, -1, -1, FALSE, FALSE, FALSE, TRUE, FALSE };
static ConfigWin *config_win = NULL;

static gboolean options_loaded = FALSE;
static gboolean fullscreen;
static gboolean gl_plugins_only;
static gboolean non_gl_plugins_only;
static gboolean all_plugins_enabled;
static gboolean random_morph;
static int fps;

static gchar *actor_plugin_buffer = NULL;

/* Table of GtkListItem's */
static GHashTable *actor_plugin_table = NULL;

/* List of gboolean's */
static GHashTable *actor_plugin_enable_table = NULL;

/* Lists of VisPluginRef's */
static GSList *actor_plugins_gl = NULL;
static GSList *actor_plugins_nongl = NULL;

static VisPluginRef *current_actor = NULL;

static char *morph_plugin = NULL;
static char *morph_plugin_buffer = NULL;
static GSList *morph_plugins_list = NULL;

static void sync_options (void);

static void config_win_load_actor_plugin_gl_list (void);
static void config_win_load_actor_plugin_nongl_list (void);

static int config_win_load_morph_plugin_list (void);

static int load_actor_plugin_list (void);
static int load_morph_plugin_list (void);
	
static void load_actor_plugin_enable_table (ConfigFile *f);

static void remove_boolean (gpointer key, gpointer value, gpointer data);

static void config_win_set_defaults (void);
static void config_win_connect_callbacks (void);
static void config_visual_initialize (void);

static gboolean read_config_file (ConfigFile *f);

static void dummy (GtkWidget *widget, gpointer data);

static void set_defaults (void);


Options *lv_bmp_config_open ()
{
	actor_plugin_buffer = g_malloc0 (OPTIONS_MAX_NAME_LEN);
	options.last_plugin = actor_plugin_buffer;
	morph_plugin_buffer = g_malloc0 (OPTIONS_MAX_NAME_LEN);
	options.icon_file = g_malloc0 (OPTIONS_MAX_ICON_PATH_LEN);

	config_visual_initialize ();

	srand (time(NULL));

	load_actor_plugin_list ();

	load_morph_plugin_list ();
	
	return &options;
}

int lv_bmp_config_close ()
{
	if (actor_plugin_buffer != NULL)
		g_free (actor_plugin_buffer);
	if (morph_plugin_buffer != NULL)
		g_free (morph_plugin_buffer);
	if (options.icon_file != NULL)
		g_free (options.icon_file);

	if (actor_plugin_table) {
		g_hash_table_destroy (actor_plugin_table);
		actor_plugin_table = NULL;
	}
	if (actor_plugin_enable_table) {
		g_hash_table_foreach (actor_plugin_enable_table, remove_boolean, NULL);
		g_hash_table_destroy (actor_plugin_enable_table);
		actor_plugin_enable_table = NULL;
	}

	if (actor_plugins_gl) {
		g_slist_free (actor_plugins_gl);
		actor_plugins_gl = NULL;
	}
	if (actor_plugins_nongl) {
		g_slist_free (actor_plugins_nongl);
		actor_plugins_nongl = NULL;
	}
	if (morph_plugins_list) {
		g_slist_free (morph_plugins_list);
		morph_plugins_list = NULL;
	}

	options_loaded = FALSE;

	return 0;
}

int lv_bmp_config_load_prefs ()
{
	gchar *vstr;
	ConfigFile *f;
	gboolean errors;
	gboolean must_create_entry;
	gboolean must_update;
	GtkWidget *msg;

	if ((f = xmms_cfg_open_default_file ()) == NULL)
		return -1;

	errors = FALSE;
	must_create_entry = FALSE;
	must_update = FALSE;
	if (xmms_cfg_read_string (f, "libvisual_bmp", "version", &vstr)) {
		if (strcmp (vstr, VERSION) == 0) {
			errors = read_config_file (f);
			if (errors)
				visual_log (VISUAL_LOG_INFO, "There are errors on config file");
		}
		else
			must_update = TRUE;
		g_free (vstr);
	} else {
		must_create_entry = TRUE;
	}
	
	if (must_update || must_create_entry)
		set_defaults ();

	load_actor_plugin_enable_table (f);

	xmms_cfg_free (f);

	/*
	 * Set our local copies
	 */
	if (!visual_morph_valid_by_name (morph_plugin_buffer)) {
		msg = xmms_show_message (PACKAGE_NAME,
					_("The morph plugin specified on the config\n"
					"file is not a valid morph plugin.\n"
					"We will use "CONFIG_DEFAULT_MORPH_PLUGIN
					" morph plugin instead.\n"
					"If you want another one, please choose it\n"
					"on the configure dialog."),
					_("Accept"), TRUE, dummy, NULL);
		gtk_widget_show (msg);
		strcpy (morph_plugin_buffer, CONFIG_DEFAULT_MORPH_PLUGIN);
	}
	options.morph_plugin = morph_plugin_buffer;
	morph_plugin = morph_plugin_buffer;
	random_morph = options.random_morph;

	fullscreen = options.fullscreen;
	fps = options.fps;
        gl_plugins_only = options.gl_plugins_only;
        non_gl_plugins_only = options.non_gl_plugins_only;
        all_plugins_enabled = options.all_plugins_enabled;

	if (gl_plugins_only)
		visual_log (VISUAL_LOG_INFO, _("GL plugins only"));
	else if (non_gl_plugins_only)
		visual_log (VISUAL_LOG_INFO, _("non GL plugins only"));
	else if (all_plugins_enabled)
		visual_log (VISUAL_LOG_INFO, _("All plugins enabled"));
	else
		visual_log (VISUAL_LOG_WARNING, "Cannot determine which kind of plugin to show");

	if (errors) {
		visual_log (VISUAL_LOG_INFO, _("LibVisual BMP plugin: config file contain errors, fixing..."));
		lv_bmp_config_save_prefs ();
	} else if (must_update) {
		visual_log (VISUAL_LOG_INFO, _("LibVisual BMP plugin: config file is from old version, updating..."));
		lv_bmp_config_save_prefs ();
	} else if (must_create_entry) {
		visual_log (VISUAL_LOG_INFO, _("LibVisual BMP plugin: adding entry to config file..."));
		lv_bmp_config_save_prefs ();
	}

	options_loaded = TRUE;

	return 0;
}

static void save_actor_enable_state (gpointer data, gpointer user_data)
{
	VisPluginRef *actor;
	ConfigFile *f;
	gboolean *enable;

	actor = data;
	f = user_data;

	visual_log_return_if_fail (actor != NULL);
	visual_log_return_if_fail (actor->info != NULL);
	visual_log_return_if_fail (f != NULL);

	enable = g_hash_table_lookup (actor_plugin_enable_table, actor->info->plugname);
	if (!enable) {
		visual_log (VISUAL_LOG_DEBUG, "enable == NULL for %s", actor->info->plugname);
		return;
	}
	xmms_cfg_write_boolean (f, "libvisual_bmp", actor->info->plugname, *enable);
}

int lv_bmp_config_save_prefs ()
{
	ConfigFile *f;

	if((f = xmms_cfg_open_default_file ()) == NULL)
		f = xmms_cfg_new ();
	if (f == NULL)
		return -1;

	xmms_cfg_write_string (f, "libvisual_bmp", "version", VERSION);

	if (options.last_plugin != NULL && (strlen(options.last_plugin) > 0))
		xmms_cfg_write_string (f, "libvisual_bmp", "last_plugin", options.last_plugin);
	else
		xmms_cfg_write_string (f, "libvisual_bmp", "last_plugin", CONFIG_DEFAULT_ACTOR_PLUGIN);

	if (options.morph_plugin != NULL && (strlen(options.morph_plugin) > 0))
		xmms_cfg_write_string (f, "libvisual_bmp", "morph_plugin", options.morph_plugin);
	else
		xmms_cfg_write_string (f, "libvisual_bmp", "morph_plugin", CONFIG_DEFAULT_MORPH_PLUGIN);
	xmms_cfg_write_boolean (f, "libvisual_bmp", "random_morph", options.random_morph);
		
	if (options.icon_file != NULL && (strlen(options.icon_file) > 0))
		xmms_cfg_write_string (f, "libvisual_bmp", "icon", options.icon_file);

	xmms_cfg_write_int (f, "libvisual_bmp", "width", options.width);
	xmms_cfg_write_int (f, "libvisual_bmp", "height", options.height);
	xmms_cfg_write_int (f, "libvisual_bmp", "color_depth", options.depth);
	xmms_cfg_write_int (f, "libvisual_bmp", "fps", options.fps);
	xmms_cfg_write_boolean (f, "libvisual_bmp", "fullscreen", options.fullscreen);
	if (options.gl_plugins_only)
		xmms_cfg_write_string (f, "libvisual_bmp", "enabled_plugins", "gl_only");
	else if (options.non_gl_plugins_only)
		xmms_cfg_write_string (f, "libvisual_bmp", "enabled_plugins", "non_gl_only");
	else if (options.all_plugins_enabled)
		xmms_cfg_write_string (f, "libvisual_bmp", "enabled_plugins", "all");
	else
		g_warning ("Inconsistency on config module");

	visual_log_return_val_if_fail (actor_plugins_gl != NULL, -1);

	g_slist_foreach (actor_plugins_gl, save_actor_enable_state, f);
	g_slist_foreach (actor_plugins_nongl, save_actor_enable_state, f);

	xmms_cfg_write_default_file (f);
	xmms_cfg_free (f);

	return 0;
}

void lv_bmp_config_toggle_fullscreen (void)
{
	fullscreen = !fullscreen;
	options.fullscreen = !options.fullscreen;
	if (config_win != NULL)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->checkbutton_fullscreen),
							fullscreen);
}

const char *lv_bmp_config_get_prev_actor (void)
{
	const gchar *prev_plugin;
	gboolean *plugin_enabled;
	int round;

	round = 0;
	prev_plugin = options.last_plugin;
	do {
		prev_plugin = visual_actor_get_prev_by_name (prev_plugin);
		if (!prev_plugin) {
			round++;
			continue;
		}
		plugin_enabled = g_hash_table_lookup (actor_plugin_enable_table, prev_plugin);
		if (!plugin_enabled)
			continue;
		if (*plugin_enabled)
			return prev_plugin;
	} while (round < 2);
	
	return NULL;
}

const char *lv_bmp_config_get_next_actor (void)
{
	const gchar *next_plugin;
	gboolean *plugin_enabled;
	int round;

	round = 0;
	next_plugin = options.last_plugin;
	do {
		next_plugin = visual_actor_get_next_by_name (next_plugin);
		if (!next_plugin) {
			round++;
			continue;
		}
		plugin_enabled = g_hash_table_lookup (actor_plugin_enable_table, next_plugin);
		if (!plugin_enabled)
			continue;
		if (*plugin_enabled)
			return next_plugin;
	} while (round < 2);
	
	return NULL;
}

void lv_bmp_config_set_current_actor (const char *name)
{
	visual_log_return_if_fail (name != NULL);

	options.last_plugin = name;
}

const char *lv_bmp_config_morph_plugin (void)
{
	GSList *l;
	int i, pos;
	
	visual_log_return_val_if_fail (g_slist_length (morph_plugins_list) > 0, NULL);

	if (random_morph) {
		pos = (rand () % (g_slist_length (morph_plugins_list)));
		l = morph_plugins_list;
		for (i = 0; i < pos; i++)
			l = g_slist_next(l);
		return ((char*)l->data);
	} else {
		return options.morph_plugin;
	}
}

void lv_bmp_config_window ()
{
#if 0
	if (config_win != NULL) {
  		gtk_widget_grab_default (config_win->button_cancel);
		gtk_widget_show (config_win->window_main);
		return;
	}

	config_visual_initialize ();

	if (!options_loaded) {
		lv_bmp_config_open ();
		lv_bmp_config_load_prefs ();
	}

	config_win = lv_bmp_config_gui_new ();

	if (options_loaded) {
		gtk_spin_button_set_value (GTK_SPIN_BUTTON(config_win->spinbutton_fps), options.fps);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->checkbutton_fullscreen),
						options.fullscreen);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->radiobutton_onlygl),
						options.gl_plugins_only);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->radiobutton_onlynongl),
						options.non_gl_plugins_only);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->radiobutton_all_plugins),
						options.all_plugins_enabled);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->checkbutton_morph_random),
						options.random_morph);
	
        } else {
		config_win_set_defaults ();
        }

	config_win_connect_callbacks ();

	gtk_widget_grab_default (config_win->button_cancel);

	if (options.all_plugins_enabled || options.non_gl_plugins_only)
		config_win_load_actor_plugin_nongl_list ();
	if (options.all_plugins_enabled || options.gl_plugins_only)
		config_win_load_actor_plugin_gl_list ();

	config_win_load_morph_plugin_list ();

	gtk_widget_show (config_win->window_main);
#endif
}

static void on_checkbutton_fullscreen_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	fullscreen = !fullscreen;
}

static void on_radiobutton_opengl_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	gl_plugins_only = !gl_plugins_only;

	gtk_list_clear_items (GTK_LIST(config_win->list_vis_plugins), 0, -1);
	config_win_load_actor_plugin_gl_list ();
}

static void on_radiobutton_non_opengl_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	non_gl_plugins_only = !non_gl_plugins_only;

	gtk_list_clear_items (GTK_LIST(config_win->list_vis_plugins), 0, -1);
	config_win_load_actor_plugin_nongl_list ();
}

static void on_radiobutton_all_plugins_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	all_plugins_enabled = !all_plugins_enabled;

	gtk_list_clear_items (GTK_LIST(config_win->list_vis_plugins), 0, -1);
	config_win_load_actor_plugin_gl_list ();
	config_win_load_actor_plugin_nongl_list ();
}

static void on_spinbutton_fps_changed (GtkEditable *editable, gpointer user_data)
{
	gchar *buffer;

	buffer = gtk_editable_get_chars (editable, (gint) 0, (gint) -1);
	fps = atoi (buffer);
	g_free (buffer);
}

static void on_button_ok_clicked (GtkButton *button, gpointer user_data)
{
	sync_options ();
	lv_bmp_config_save_prefs ();
	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET(button)));
}

static void on_button_apply_clicked (GtkButton *button, gpointer user_data)
{
	sync_options ();
	lv_bmp_config_save_prefs ();
}


static void on_button_cancel_clicked (GtkButton *button, gpointer user_data)
{
	/*
	 * Restore original values
	 */
	if (options_loaded) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->checkbutton_fullscreen),
						options.fullscreen);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON(config_win->spinbutton_fps), options.fps);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->radiobutton_onlygl),
						options.gl_plugins_only);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->radiobutton_onlynongl),
						options.non_gl_plugins_only);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->radiobutton_all_plugins),
						options.all_plugins_enabled);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->checkbutton_morph_random),
						options.random_morph);
	
	} else {
		config_win_set_defaults ();
	}

	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET(button)));
}

static void sync_options ()
{
	options.fullscreen = fullscreen;
	options.fps = fps;
        options.gl_plugins_only = gl_plugins_only;
        options.non_gl_plugins_only = non_gl_plugins_only;
        options.all_plugins_enabled = all_plugins_enabled;
	options.morph_plugin = morph_plugin;
	options.random_morph = random_morph;
}

static void on_checkbutton_vis_plugin_toggled (GtkToggleButton *togglebutton, gpointer user_data);

static void on_actor_plugin_selected (GtkListItem *item, VisPluginRef *actor)
{
	gboolean *enabled;

	visual_log_return_if_fail (actor != NULL);
	visual_log_return_if_fail (actor->info != NULL);

	current_actor = actor;
	enabled = g_hash_table_lookup (actor_plugin_enable_table, actor->info->plugname);
	visual_log_return_if_fail (enabled != NULL);

	gtk_signal_disconnect_by_func (GTK_OBJECT (config_win->checkbutton_vis_plugin),
					on_checkbutton_vis_plugin_toggled, NULL);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->checkbutton_vis_plugin), *enabled);	

	gtk_signal_connect (GTK_OBJECT (config_win->checkbutton_vis_plugin), "toggled",
                      GTK_SIGNAL_FUNC (on_checkbutton_vis_plugin_toggled),
                      NULL);
}

static void on_checkbutton_vis_plugin_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	GtkWidget *item;
	GList *items = NULL;
	gint pos;
	gchar *name;
	gchar *plugname;
	gboolean *enable;

	if (!current_actor)
		return;

	plugname = current_actor->info->plugname;
	if (gtk_toggle_button_get_active (togglebutton)) {
		/* We are enabling the selected actor */
		item = g_hash_table_lookup (actor_plugin_table, plugname);
		g_hash_table_remove (actor_plugin_table, plugname);

		/* Drop the item from the list, after save his position */
		pos = gtk_list_child_position (GTK_LIST(config_win->list_vis_plugins), item);
		items = g_list_append (items, item);
		gtk_list_remove_items (GTK_LIST(config_win->list_vis_plugins), items);
		g_list_free (items);

		/* Create a new item marked as enabled */
		name = g_strconcat (current_actor->info->name, _(" (enabled)"), 0);
		item = gtk_list_item_new_with_label (name);
		g_free (name);
		/*gtk_list_select_item (GTK_LIST(config_win->list_vis_plugins), pos);*/
		gtk_widget_show (item);
		gtk_signal_connect (GTK_OBJECT(item), "select",
			GTK_SIGNAL_FUNC(on_actor_plugin_selected),
			(gpointer) current_actor);

		/* Insert the new item */
		items = NULL;
		items = g_list_append (items, item);
		gtk_list_insert_items (GTK_LIST(config_win->list_vis_plugins), items, pos);
		
		g_hash_table_insert (actor_plugin_table, plugname, item);

		/* Mark it as enabled */
		enable = g_hash_table_lookup (actor_plugin_enable_table, plugname);
		visual_log_return_if_fail (enable != NULL);
		*enable = TRUE;
	} else {
		item = g_hash_table_lookup (actor_plugin_table, plugname);
		g_hash_table_remove (actor_plugin_table, plugname);

		/* Drop the item from the list, after save his position */
		pos = gtk_list_child_position (GTK_LIST(config_win->list_vis_plugins), item);
		items = g_list_append (items, item);
		gtk_list_remove_items (GTK_LIST(config_win->list_vis_plugins), items);
		g_list_free (items);

		/* Create a new item marked as enabled */
		item = gtk_list_item_new_with_label (current_actor->info->name);
		/*gtk_list_select_item (GTK_LIST(config_win->list_vis_plugins), pos);*/
		gtk_widget_show (item);
		gtk_signal_connect (GTK_OBJECT(item), "select",
			GTK_SIGNAL_FUNC(on_actor_plugin_selected),
			(gpointer) current_actor);

		/* Insert the new item */
		items = NULL;
		items = g_list_append (items, item);
		gtk_list_insert_items (GTK_LIST(config_win->list_vis_plugins), items, pos);
		
		g_hash_table_insert (actor_plugin_table, plugname, item);

		/* Mark it as disabled */
		enable = g_hash_table_lookup (actor_plugin_enable_table, plugname);
		visual_log_return_if_fail (enable != NULL);
		*enable = FALSE;
	}
}

static void on_button_vis_plugin_about_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *msgwin;
	gchar *msg;

	if (!current_actor)
		return;

	visual_log_return_if_fail (current_actor->info != NULL);

	msg = g_strconcat (current_actor->info->name, "\n",
				_("Version: "), current_actor->info->version, "\n",
				current_actor->info->about, "\n",
				_("Author: "), current_actor->info->author, "\n\n",
				current_actor->info->help, 0);
	msgwin = xmms_show_message (PACKAGE_NAME, msg, _("Accept"), TRUE, dummy, NULL);
	gtk_widget_show (msgwin);
	g_free (msg);
}

/* FIXME Libvisual API must have something for doing this! */
static int is_gl_actor (VisPluginRef *actor)
{
	VisPluginData *plugin;
	VisActorPlugin *actplugin;

	visual_log_return_val_if_fail (actor != NULL, -1);
	visual_log_return_val_if_fail (actor->info->plugin != NULL, -1);

	plugin = visual_plugin_load (actor);
	actplugin = plugin->info->plugin;
	if (actplugin->depth & VISUAL_VIDEO_DEPTH_GL) {
		visual_plugin_unload (plugin);
		return TRUE;
	} else {
		visual_plugin_unload (plugin);
		return FALSE;
	}
}
	
static void actor_plugin_add (VisPluginRef *actor)
{
	visual_log_return_if_fail (actor != NULL);

	if (is_gl_actor (actor))
		actor_plugins_gl = g_slist_append (actor_plugins_gl, actor);
	else
		actor_plugins_nongl = g_slist_append (actor_plugins_nongl, actor);
}

/*
 * This function initializes the actor_plugin_(gl/nongl)_items lists.
 */
static int load_actor_plugin_list ()
{
	VisList *list;
	VisListEntry *item;
	VisPluginRef *actor;
	GtkWidget *msg;

	/* We want to load the lists just once */
	visual_log_return_val_if_fail (actor_plugins_gl == NULL, -1);
	visual_log_return_val_if_fail (actor_plugins_nongl == NULL, -1);

	list = visual_actor_get_list ();
	if (!list) {
		visual_log (VISUAL_LOG_WARNING, _("The list of actor plugins is empty."));
		return -1;
	}
	
	item = NULL;
	/* FIXME update to visual_list_is_empty() when ready */
	if (!(actor = (VisPluginRef*) visual_list_next (list, &item))) {
		msg = xmms_show_message (_(PACKAGE_NAME " error"),
					_("There are no actor plugins installed.\n"
					PACKAGE_NAME " cannot be initialized.\n"
					"Please visit http://libvisual.sf.net to\n"
					"to get some nice plugins."),
					_("Accept"), TRUE, dummy, NULL);
		return -1;
	}

	item = NULL;
	while ((actor = (VisPluginRef*) visual_list_next (list, &item)))
		actor_plugin_add (actor);

	return 0;
}

static guint hash_function (gconstpointer key)
{
	const char *skey;
	guint hash_value = 0;
	int i;

	if (!key)
		return 0;
	
	skey = key;
	for (i = 0; i < strlen (skey); i++)
		hash_value = (hash_value << 4) + (hash_value ^ (guint) skey[i]);

	return hash_value;
}

static gint hash_compare (gconstpointer s1, gconstpointer s2)
{
	return (!strcmp ((char*) s1, (char*) s2));
}

static void load_actor_enable_state (gpointer data, gpointer user_data)
{
	ConfigFile *config_file;
	VisPluginRef *actor;
	gboolean enabled, *b;

	actor = data;
	config_file = user_data;

	visual_log_return_if_fail (actor != NULL);
	visual_log_return_if_fail (actor->info != NULL);
	visual_log_return_if_fail (config_file != NULL);

	if (!xmms_cfg_read_boolean (config_file, "libvisual_bmp", actor->info->plugname, &enabled))
		enabled = TRUE;

	b = g_malloc (sizeof(gboolean));
	*b = enabled;
	g_hash_table_insert (actor_plugin_enable_table, actor->info->plugname, b);
}

static void load_actor_plugin_enable_table (ConfigFile *f)
{
	visual_log_return_if_fail (actor_plugins_nongl != NULL);
	visual_log_return_if_fail (actor_plugins_gl != NULL);

	if (!actor_plugin_enable_table)
		actor_plugin_enable_table = g_hash_table_new (hash_function, hash_compare);

	g_slist_foreach (actor_plugins_nongl, load_actor_enable_state, f);
	g_slist_foreach (actor_plugins_gl, load_actor_enable_state, f);
}

static void remove_boolean (gpointer key, gpointer value, gpointer data)
{
	g_free (value);
}

static void new_actor_item (gpointer data, gpointer user_data)
{
	GList *items;
	GtkWidget *item/*, *olditem*/;
	VisPluginRef *actor;
	gchar *name;
	const gchar *plugname;
	gboolean *enabled;

	actor = data;
	items = *(GList**)user_data;

	visual_log_return_if_fail (actor != NULL);
	visual_log_return_if_fail (actor->info != NULL);

	plugname = actor->info->plugname;
	enabled = g_hash_table_lookup (actor_plugin_enable_table, plugname);
	visual_log_return_if_fail (enabled != NULL);

	/* Create the new item */
	if (*enabled) {
		name = g_strconcat (actor->info->name, _(" (enabled)"), 0);
		item = gtk_list_item_new_with_label (name);
		g_free (name);
	} else {
		item = gtk_list_item_new_with_label (actor->info->name);
	}

	gtk_widget_show (item);
	gtk_signal_connect (GTK_OBJECT(item), "select",
		GTK_SIGNAL_FUNC(on_actor_plugin_selected),
		(gpointer) actor);
	items = g_list_append (items, item);

	/*olditem = g_hash_table_lookup (actor_plugin_table, plugname);
	if (olditem)
		gtk_widget_destroy (olditem);*/

	g_hash_table_remove (actor_plugin_table, plugname);
	g_hash_table_insert (actor_plugin_table, plugname, item);

	*(GList**)user_data = items;
}

static void config_win_load_actor_plugin_gl_list ()
{
	GList *items;

	if (!actor_plugin_table)
		actor_plugin_table = g_hash_table_new (hash_function, hash_compare);

	items = NULL;
	g_slist_foreach (actor_plugins_gl, new_actor_item, &items);
	gtk_list_append_items (GTK_LIST(config_win->list_vis_plugins), items);
}

static void config_win_load_actor_plugin_nongl_list ()
{
	GList *items;

	if (!actor_plugin_table)
		actor_plugin_table = g_hash_table_new (hash_function, hash_compare);

	items = NULL;
	g_slist_foreach (actor_plugins_nongl, new_actor_item, &items);
	gtk_list_append_items (GTK_LIST(config_win->list_vis_plugins), items);
}

static void on_morph_plugin_activate (GtkMenuItem *menuitem, char *name)
{
	visual_log_return_if_fail (name != NULL);

	morph_plugin = name;
}

static int config_win_load_morph_plugin_list ()
{
	VisList *list;
	VisListEntry *item;
	VisPluginRef *morph;
	GtkWidget *msg;
	GtkWidget *menu;
	GtkWidget *menuitem;
	GSList *group;
	gint index;

	/* FIXME use load_morph_plugin_list() */
	list = visual_morph_get_list ();
	if (!list) {
		visual_log (VISUAL_LOG_WARNING, _("The list of morph plugins is empty."));
		return -1;
	}
	
	item = NULL;
	/* FIXME update to visual_list_is_empty() when ready */
	if (!(morph = (VisPluginRef*) visual_list_next (list, &item))) {
		msg = xmms_show_message (PACKAGE_NAME,
					_("There are no morph plugins, so switching\n"
					"between visualization plugins will be do it\n"
					"without any morphing."),
					_("Accept"), TRUE, dummy, NULL);
		return -1;
	}
	index = 0;
	item = NULL;
	while ((morph = (VisPluginRef*) visual_list_next (list, &item))) {
		if (!(morph->info)) {
			visual_log (VISUAL_LOG_WARNING, _("There is no info for this plugin"));
			continue;
		}
		group = config_win->optionmenu_morph_plugin_group;
		menu = gtk_option_menu_get_menu (GTK_OPTION_MENU(config_win->optionmenu_morph_plugin));
		menuitem = gtk_radio_menu_item_new_with_label (group, morph->info->plugname);
		group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM(menuitem));
		gtk_menu_append (GTK_MENU(menu), menuitem);
		config_win->optionmenu_morph_plugin_group = group;

		gtk_signal_connect (GTK_OBJECT(menuitem), "activate",
					GTK_SIGNAL_FUNC(on_morph_plugin_activate),
					(gpointer) morph->info->plugname);

		gtk_widget_show (menuitem);

		if (!strcmp (morph->info->plugname, options.morph_plugin)) {
			gtk_menu_item_activate (GTK_MENU_ITEM(menuitem));
			gtk_menu_set_active (GTK_MENU(menu), index);
			gtk_option_menu_set_history (GTK_OPTION_MENU(config_win->optionmenu_morph_plugin), index);
		}
		index++;
	}

	return 0;
}

static int load_morph_plugin_list ()
{
	VisList *list;
	VisListEntry *item;
	VisPluginRef *morph;
	GtkWidget *msg;

	list = visual_morph_get_list ();
	if (!list) {
		visual_log (VISUAL_LOG_WARNING, _("The list of morph plugins is empty."));
		return -1;
	}
	
	item = NULL;
	/* FIXME update to visual_list_is_empty() when ready */
	if (!(morph = (VisPluginRef*) visual_list_next (list, &item))) {
		msg = xmms_show_message (PACKAGE_NAME,
					_("There are no morph plugins, so switching\n"
					"between visualization plugins will be do it\n"
					"without any morphing."),
					_("Accept"), TRUE, dummy, NULL);
		return -1;
	}
	item = NULL;
	while ((morph = (VisPluginRef*) visual_list_next (list, &item))) {
		if (!(morph->info)) {
			visual_log (VISUAL_LOG_WARNING, _("There is no info for this plugin"));
			continue;
		}
		morph_plugins_list = g_slist_append (morph_plugins_list, morph->info->plugname);
	}

	return 0;
}

static void on_button_morph_plugin_about_clicked (GtkButton *button, gpointer data)
{
	VisList *list;
	VisListEntry *item;
	VisPluginRef *morph;
	gchar *msg;
	GtkWidget *msgwin;
		
	list = visual_morph_get_list ();
	if (!list) {
		visual_log (VISUAL_LOG_WARNING, _("The list of input plugins is empty."));
		return;
	}

	item = NULL;
	while ((morph = (VisPluginRef*) visual_list_next (list, &item))) {
		if (!(morph->info)) {
			visual_log (VISUAL_LOG_WARNING, _("There is no info for this plugin"));
			continue;
		}
		if (strcmp (morph->info->plugname, options.morph_plugin) == 0) {
			msg = g_strconcat (morph->info->name, "\n",
					_("Version: "), morph->info->version, "\n",
					morph->info->about, "\n",
					_("Author: "), morph->info->author, "\n\n",
					morph->info->help, 0);
			msgwin = xmms_show_message (PACKAGE_NAME, msg,
					_("Accept"), TRUE, dummy, NULL);
			gtk_widget_show (msgwin);
			g_free (msg);
			break;
		}
	}
}

static void on_checkbutton_morph_random_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	random_morph = !random_morph;
}

/*
 * This function set the default values on configure dialog for all options,
 * except the selected vis and morph plugins.
 */
static void config_win_set_defaults (void)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->checkbutton_fullscreen),
					default_options.fullscreen);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(config_win->spinbutton_fps), default_options.fps);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->radiobutton_onlygl),
					default_options.gl_plugins_only);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->radiobutton_onlynongl),
					default_options.non_gl_plugins_only);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->radiobutton_all_plugins),
					default_options.all_plugins_enabled);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (config_win->checkbutton_morph_random),
					default_options.random_morph);
}

static void config_win_connect_callbacks (void)
{
	gtk_signal_connect (GTK_OBJECT (config_win->checkbutton_vis_plugin), "toggled",
                      GTK_SIGNAL_FUNC (on_checkbutton_vis_plugin_toggled),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->checkbutton_fullscreen), "toggled",
                      GTK_SIGNAL_FUNC (on_checkbutton_fullscreen_toggled),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->radiobutton_onlygl), "toggled",
                      GTK_SIGNAL_FUNC (on_radiobutton_opengl_toggled),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->radiobutton_onlynongl), "toggled",
                      GTK_SIGNAL_FUNC (on_radiobutton_non_opengl_toggled),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->radiobutton_all_plugins), "toggled",
                      GTK_SIGNAL_FUNC (on_radiobutton_all_plugins_toggled),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->spinbutton_fps), "changed",
                      GTK_SIGNAL_FUNC (on_spinbutton_fps_changed),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->button_ok), "clicked",
                      GTK_SIGNAL_FUNC (on_button_ok_clicked),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->button_apply), "clicked",
                      GTK_SIGNAL_FUNC (on_button_apply_clicked),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->button_cancel), "clicked",
                      GTK_SIGNAL_FUNC (on_button_cancel_clicked),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->button_vis_plugin_about), "clicked",
                      GTK_SIGNAL_FUNC (on_button_vis_plugin_about_clicked),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->button_morph_plugin_about), "clicked",
                      GTK_SIGNAL_FUNC (on_button_morph_plugin_about_clicked),
                      NULL);
	gtk_signal_connect (GTK_OBJECT (config_win->checkbutton_morph_random), "toggled",
                      GTK_SIGNAL_FUNC (on_checkbutton_morph_random_toggled),
                      NULL);
}

static void set_defaults (void)
{
	strcpy (actor_plugin_buffer, CONFIG_DEFAULT_ACTOR_PLUGIN);
	options.last_plugin = actor_plugin_buffer;
	strcpy (morph_plugin_buffer, CONFIG_DEFAULT_MORPH_PLUGIN);
	options.morph_plugin = morph_plugin_buffer;

	options.width = default_options.width;
	options.height = default_options.height;
	options.depth = default_options.depth;
	options.fps = default_options.fps;
	options.fullscreen = default_options.fullscreen;
	options.gl_plugins_only = default_options.gl_plugins_only;
	options.non_gl_plugins_only = default_options.non_gl_plugins_only;
	options.all_plugins_enabled = default_options.all_plugins_enabled;
	options.random_morph = default_options.random_morph;
}

static void config_visual_initialize ()
{
	int argc;
	char **argv;
	GtkWidget *msg;

	if (!visual_is_initialized ()) {
	        argv = g_malloc (sizeof(char*));
	        argv[0] = g_strdup ("BMP plugin");
        	argc = 1;
		if (visual_init (&argc, &argv) < 0) {
			msg = xmms_show_message (PACKAGE_NAME,
					_("We cannot initialize Libvisual library.\n"
					"Libvisual is necessary for this plugin to work."),
					_("Accept"), TRUE, dummy, NULL);
			gtk_widget_show (msg);
        		g_free (argv[0]);
		        g_free (argv);
			return;
		}
        	g_free (argv[0]);
	        g_free (argv);
	}
}

static void dummy (GtkWidget *widget, gpointer data)
{
}

static gboolean read_config_file (ConfigFile *f)
{
	gchar *enabled_plugins;
	gboolean errors = FALSE;

	if (!xmms_cfg_read_string (f, "libvisual_bmp", "last_plugin", &actor_plugin_buffer)
 		|| (strlen (actor_plugin_buffer) <= 0)) {
		visual_log (VISUAL_LOG_DEBUG, "Error on last_plugin option");
		strcpy (actor_plugin_buffer, CONFIG_DEFAULT_ACTOR_PLUGIN);
		errors = TRUE;
	}
	options.last_plugin = actor_plugin_buffer;
	if (!xmms_cfg_read_string (f, "libvisual_bmp", "morph_plugin", &morph_plugin_buffer)
		|| (strlen (morph_plugin_buffer) <= 0)) {
		visual_log (VISUAL_LOG_DEBUG, "Error on morph_plugin option");
		strcpy (morph_plugin_buffer, CONFIG_DEFAULT_MORPH_PLUGIN);
		errors = TRUE;
	}
	morph_plugin = morph_plugin_buffer;
	options.morph_plugin = morph_plugin;
	if (!xmms_cfg_read_boolean (f, "libvisual_bmp", "random_morph", &options.random_morph)) {
		visual_log (VISUAL_LOG_DEBUG, "Error on random_morph option");
		options.random_morph = default_options.random_morph;
		errors = TRUE;
	}
	if (!xmms_cfg_read_string (f, "libvisual_bmp", "icon", &options.icon_file)
		|| (strlen (options.icon_file) <= 0)) {
		visual_log (VISUAL_LOG_DEBUG, "Error on icon option");
		errors = TRUE;
	}
	if (!xmms_cfg_read_int (f, "libvisual_bmp", "width", &options.width) || options.width <= 0) {
		visual_log (VISUAL_LOG_DEBUG, "Error on width option");
		options.width = default_options.width;
		errors = TRUE;
	}
	if (!xmms_cfg_read_int (f, "libvisual_bmp", "height", &options.height)	|| options.height <= 0) {
		visual_log (VISUAL_LOG_DEBUG, "Error on height option");
		options.height = default_options.height;
		errors = TRUE;
	}
	if (!xmms_cfg_read_int (f, "libvisual_bmp", "fps", &options.fps) || options.fps <= 0) {
		visual_log (VISUAL_LOG_DEBUG, "Error on fps option");
		options.fps = default_options.fps;
		errors = TRUE;
	}
	if (!xmms_cfg_read_int (f, "libvisual_bmp", "color_depth", &options.depth) || options.depth <= 0) {
		visual_log (VISUAL_LOG_DEBUG, "Error on color_depth option");
		options.depth = default_options.depth;
		errors = TRUE;
	}
	if (!xmms_cfg_read_boolean (f, "libvisual_bmp", "fullscreen", &options.fullscreen)) {
		visual_log (VISUAL_LOG_DEBUG, "Error on fullscreen option");
		options.fullscreen = default_options.fullscreen;
		errors = TRUE;
	}
	enabled_plugins = g_malloc0 (OPTIONS_MAX_NAME_LEN);
	if (!xmms_cfg_read_string (f, "libvisual_bmp", "enabled_plugins", &enabled_plugins)
		|| (strlen (enabled_plugins) <= 0)) {
		visual_log (VISUAL_LOG_DEBUG, "Error on enabled_plugins option: %s", enabled_plugins);
		options.gl_plugins_only = default_options.gl_plugins_only;
		options.non_gl_plugins_only = default_options.non_gl_plugins_only;
		options.all_plugins_enabled = default_options.all_plugins_enabled;
		errors = TRUE;
	} else {
		options.gl_plugins_only = FALSE;
		options.non_gl_plugins_only = FALSE;
		options.all_plugins_enabled = FALSE;
		if (strcmp (enabled_plugins, "gl_only") == 0)
			options.gl_plugins_only = TRUE;
		else if (strcmp (enabled_plugins, "non_gl_only") == 0)
			options.non_gl_plugins_only = TRUE;
		else if (strcmp (enabled_plugins, "all") == 0)
			options.all_plugins_enabled = TRUE;
		else {
			visual_log (VISUAL_LOG_WARNING, _("Invalid value for 'enabled_plugins' option"));
			options.gl_plugins_only = default_options.gl_plugins_only;
			options.non_gl_plugins_only = default_options.non_gl_plugins_only;
			options.all_plugins_enabled = default_options.all_plugins_enabled;
			errors = TRUE;	
		}
	}
	g_free (enabled_plugins);

	return errors;
}
