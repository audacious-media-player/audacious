#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "config.h"

#include "config_gui.h"

#if 0
static gchar *check_file_exists (const gchar *directory, const gchar *filename);
static GtkWidget *create_pixmap (GtkWidget *widget, const gchar *filename);
static GtkWidget *create_dummy_pixmap (GtkWidget *widget);
/*static GtkWidget* lookup_widget (GtkWidget *widget, const gchar *widget_name);*/
static void add_pixmap_directory (const gchar *directory);

ConfigWin *lv_xmms_config_gui_new (void)
{
  ConfigWin *config_gui;

  GtkWidget *window_main;
  GtkWidget *vbox_main;
  GtkWidget *frame_vis_plugin;
  GtkWidget *vbox3;
  GtkWidget *scrolledwindow_vis_plugins;
  GtkWidget *viewport1;
  GtkWidget *list_vis_plugins;
  GtkWidget *hbox_vis_plugin_controls;
  GtkWidget *hbox_vis_plugin_buttons;
  GtkWidget *button_vis_plugin_conf;
  GtkWidget *button_vis_plugin_about;
  GtkWidget *checkbutton_vis_plugin;
  GtkWidget *checkbutton_fullscreen;
  GSList *buttongroup_plugins_group = NULL;
  GtkWidget *radiobutton_all_plugins;
  GtkWidget *radiobutton_onlynongl;
  GtkWidget *radiobutton_onlygl;
  GtkWidget *hbox_fps;
  GtkWidget *label_fps;
  GtkObject *spinbutton_fps_adj;
  GtkWidget *spinbutton_fps;
  GtkWidget *frame_morph_plugin;
  GtkWidget *vbox_morph_plugin;
  GtkWidget *optionmenu_morph_plugin;
  GtkWidget *optionmenu_morph_plugin_menu;
  GtkWidget *hbox_morph_plugin_controls;
  GtkWidget *hbox_morph_plugin_buttons;
  GtkWidget *button_morph_plugin_conf;
  GtkWidget *button_morph_plugin_about;
  GtkWidget *checkbutton_morph_random;
  GtkWidget *hbox_main_buttons;
  GtkWidget *button_ok;
  GtkWidget *button_apply;
  GtkWidget *button_cancel;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  add_pixmap_directory (PACKAGE_DATADIR);
  
  window_main = gtk_window_new (GTK_WINDOW_DIALOG);
  gtk_object_set_data (GTK_OBJECT (window_main), "window_main", window_main);
  gtk_window_set_title (GTK_WINDOW (window_main), _("LibVisual XMMS Plugin"));
  gtk_window_set_position (GTK_WINDOW (window_main), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size (GTK_WINDOW (window_main), -1, 450);

  vbox_main = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_main);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "vbox_main", vbox_main,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox_main);
  gtk_container_add (GTK_CONTAINER (window_main), vbox_main);
  gtk_container_set_border_width (GTK_CONTAINER (vbox_main), 6);

  frame_vis_plugin = gtk_frame_new (_("Visualization Plugins"));
  gtk_widget_ref (frame_vis_plugin);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "frame_vis_plugin", frame_vis_plugin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame_vis_plugin);
  gtk_box_pack_start (GTK_BOX (vbox_main), frame_vis_plugin, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame_vis_plugin), 2);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox3);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "vbox3", vbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox3);
  gtk_container_add (GTK_CONTAINER (frame_vis_plugin), vbox3);

  scrolledwindow_vis_plugins = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow_vis_plugins);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "scrolledwindow_vis_plugins", scrolledwindow_vis_plugins,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow_vis_plugins);
  gtk_box_pack_start (GTK_BOX (vbox3), scrolledwindow_vis_plugins, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow_vis_plugins), 2);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_vis_plugins), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  viewport1 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport1);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "viewport1", viewport1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow_vis_plugins), viewport1);

  list_vis_plugins = gtk_list_new ();
  gtk_widget_ref (list_vis_plugins);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "list_vis_plugins", list_vis_plugins,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (list_vis_plugins);
  gtk_container_add (GTK_CONTAINER (viewport1), list_vis_plugins);
  gtk_list_set_selection_mode (GTK_LIST (list_vis_plugins), GTK_SELECTION_SINGLE);

  hbox_vis_plugin_controls = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_vis_plugin_controls);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "hbox_vis_plugin_controls", hbox_vis_plugin_controls,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_vis_plugin_controls);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox_vis_plugin_controls, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox_vis_plugin_controls), 2);

  hbox_vis_plugin_buttons = gtk_hbox_new (TRUE, 0);
  gtk_widget_ref (hbox_vis_plugin_buttons);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "hbox_vis_plugin_buttons", hbox_vis_plugin_buttons,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_vis_plugin_buttons);
  gtk_box_pack_start (GTK_BOX (hbox_vis_plugin_controls), hbox_vis_plugin_buttons, FALSE, TRUE, 0);

  button_vis_plugin_conf = gtk_button_new_with_label (_("Configure"));
  gtk_widget_ref (button_vis_plugin_conf);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "button_vis_plugin_conf", button_vis_plugin_conf,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_vis_plugin_conf);
  gtk_box_pack_start (GTK_BOX (hbox_vis_plugin_buttons), button_vis_plugin_conf, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button_vis_plugin_conf), 2);

  button_vis_plugin_about = gtk_button_new_with_label (_("About"));
  gtk_widget_ref (button_vis_plugin_about);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "button_vis_plugin_about", button_vis_plugin_about,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_vis_plugin_about);
  gtk_box_pack_start (GTK_BOX (hbox_vis_plugin_buttons), button_vis_plugin_about, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button_vis_plugin_about), 2);

  checkbutton_vis_plugin = gtk_check_button_new_with_label (_("Enable/Disable"));
  gtk_widget_ref (checkbutton_vis_plugin);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "checkbutton_vis_plugin", checkbutton_vis_plugin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (checkbutton_vis_plugin);
  gtk_box_pack_end (GTK_BOX (hbox_vis_plugin_controls), checkbutton_vis_plugin, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (checkbutton_vis_plugin), 2);

  checkbutton_fullscreen = gtk_check_button_new_with_label (_("Fullscreen"));
  gtk_widget_ref (checkbutton_fullscreen);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "checkbutton_fullscreen", checkbutton_fullscreen,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (checkbutton_fullscreen);
  gtk_box_pack_start (GTK_BOX (vbox3), checkbutton_fullscreen, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, checkbutton_fullscreen, _("You can toggle between normal and fullscreen mode pressing key TAB or F11"), NULL);

  radiobutton_all_plugins = gtk_radio_button_new_with_label (buttongroup_plugins_group, _("All plugins"));
  buttongroup_plugins_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_all_plugins));
  gtk_widget_ref (radiobutton_all_plugins);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "radiobutton_all_plugins", radiobutton_all_plugins,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_all_plugins);
  gtk_box_pack_start (GTK_BOX (vbox3), radiobutton_all_plugins, FALSE, FALSE, 0);

  radiobutton_onlynongl = gtk_radio_button_new_with_label (buttongroup_plugins_group, _("Only non GL plugins"));
  buttongroup_plugins_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_onlynongl));
  gtk_widget_ref (radiobutton_onlynongl);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "radiobutton_onlynongl", radiobutton_onlynongl,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_onlynongl);
  gtk_box_pack_start (GTK_BOX (vbox3), radiobutton_onlynongl, FALSE, FALSE, 0);

  radiobutton_onlygl = gtk_radio_button_new_with_label (buttongroup_plugins_group, _("Only GL plugins"));
  buttongroup_plugins_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radiobutton_onlygl));
  gtk_widget_ref (radiobutton_onlygl);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "radiobutton_onlygl", radiobutton_onlygl,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (radiobutton_onlygl);
  gtk_box_pack_start (GTK_BOX (vbox3), radiobutton_onlygl, FALSE, FALSE, 0);

  hbox_fps = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_fps);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "hbox_fps", hbox_fps,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_fps);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox_fps, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox_fps), 2);

  label_fps = gtk_label_new (_("Maximum Frames Per Second:"));
  gtk_widget_ref (label_fps);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "label_fps", label_fps,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_fps);
  gtk_box_pack_start (GTK_BOX (hbox_fps), label_fps, FALSE, FALSE, 6);
  gtk_label_set_justify (GTK_LABEL (label_fps), GTK_JUSTIFY_LEFT);

  spinbutton_fps_adj = gtk_adjustment_new (30, 10, 100, 1, 10, 10);
  spinbutton_fps = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton_fps_adj), 1, 0);
  gtk_widget_ref (spinbutton_fps);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "spinbutton_fps", spinbutton_fps,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (spinbutton_fps);
  gtk_box_pack_start (GTK_BOX (hbox_fps), spinbutton_fps, FALSE, FALSE, 0);

  frame_morph_plugin = gtk_frame_new (_("Morph Plugin"));
  gtk_widget_ref (frame_morph_plugin);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "frame_morph_plugin", frame_morph_plugin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame_morph_plugin);
  gtk_box_pack_start (GTK_BOX (vbox_main), frame_morph_plugin, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame_morph_plugin), 2);

  vbox_morph_plugin = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox_morph_plugin);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "vbox_morph_plugin", vbox_morph_plugin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox_morph_plugin);
  gtk_container_add (GTK_CONTAINER (frame_morph_plugin), vbox_morph_plugin);

  optionmenu_morph_plugin = gtk_option_menu_new ();
  gtk_widget_ref (optionmenu_morph_plugin);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "optionmenu_morph_plugin", optionmenu_morph_plugin,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (optionmenu_morph_plugin);
  gtk_box_pack_start (GTK_BOX (vbox_morph_plugin), optionmenu_morph_plugin, FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, optionmenu_morph_plugin, _("Select the kind of morph that will be applied when switching from one visualization plugin to another "), NULL);
  optionmenu_morph_plugin_menu = gtk_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu_morph_plugin), optionmenu_morph_plugin_menu);

  hbox_morph_plugin_controls = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox_morph_plugin_controls);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "hbox_morph_plugin_controls", hbox_morph_plugin_controls,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_morph_plugin_controls);
  gtk_box_pack_start (GTK_BOX (vbox_morph_plugin), hbox_morph_plugin_controls, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox_morph_plugin_controls), 2);

  hbox_morph_plugin_buttons = gtk_hbox_new (TRUE, 0);
  gtk_widget_ref (hbox_morph_plugin_buttons);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "hbox_morph_plugin_buttons", hbox_morph_plugin_buttons,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_morph_plugin_buttons);
  gtk_box_pack_start (GTK_BOX (hbox_morph_plugin_controls), hbox_morph_plugin_buttons, FALSE, FALSE, 0);

  button_morph_plugin_conf = gtk_button_new_with_label (_("Configure"));
  gtk_widget_ref (button_morph_plugin_conf);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "button_morph_plugin_conf", button_morph_plugin_conf,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_morph_plugin_conf);
  gtk_box_pack_start (GTK_BOX (hbox_morph_plugin_buttons), button_morph_plugin_conf, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button_morph_plugin_conf), 2);

  button_morph_plugin_about = gtk_button_new_with_label (_("About"));
  gtk_widget_ref (button_morph_plugin_about);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "button_morph_plugin_about", button_morph_plugin_about,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_morph_plugin_about);
  gtk_box_pack_start (GTK_BOX (hbox_morph_plugin_buttons), button_morph_plugin_about, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (button_morph_plugin_about), 2);

  checkbutton_morph_random = gtk_check_button_new_with_label (_("Select one morph plugin randomly"));
  gtk_widget_ref (checkbutton_morph_random);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "checkbutton_morph_random", checkbutton_morph_random,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (checkbutton_morph_random);
  gtk_box_pack_start (GTK_BOX (vbox_morph_plugin), checkbutton_morph_random, FALSE, FALSE, 0);

  hbox_main_buttons = gtk_hbox_new (TRUE, 0);
  gtk_widget_ref (hbox_main_buttons);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "hbox_main_buttons", hbox_main_buttons,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox_main_buttons);
  gtk_box_pack_start (GTK_BOX (vbox_main), hbox_main_buttons, FALSE, FALSE, 6);

  button_ok = gtk_button_new_with_label (_("Accept"));
  gtk_widget_ref (button_ok);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "button_ok", button_ok,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_ok);
  gtk_box_pack_start (GTK_BOX (hbox_main_buttons), button_ok, FALSE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (button_ok, GTK_CAN_DEFAULT);

  button_apply = gtk_button_new_with_label (_("Apply"));
  gtk_widget_ref (button_apply);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "button_apply", button_apply,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_apply);
  gtk_box_pack_start (GTK_BOX (hbox_main_buttons), button_apply, FALSE, TRUE, 6);
  GTK_WIDGET_SET_FLAGS (button_apply, GTK_CAN_DEFAULT);

  button_cancel = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_ref (button_cancel);
  gtk_object_set_data_full (GTK_OBJECT (window_main), "button_cancel", button_cancel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button_cancel);
  gtk_box_pack_start (GTK_BOX (hbox_main_buttons), button_cancel, FALSE, TRUE, 6);
  GTK_WIDGET_SET_FLAGS (button_cancel, GTK_CAN_DEFAULT);

  gtk_object_set_data (GTK_OBJECT (window_main), "tooltips", tooltips);

  config_gui = g_new0 (ConfigWin, 1);

  config_gui->window_main = window_main;

  config_gui->list_vis_plugins = list_vis_plugins;
  config_gui->button_vis_plugin_conf = button_vis_plugin_conf;
  config_gui->button_vis_plugin_about = button_vis_plugin_about;
  config_gui->checkbutton_vis_plugin = checkbutton_vis_plugin;

  config_gui->checkbutton_fullscreen = checkbutton_fullscreen;
  config_gui->radiobutton_onlygl = radiobutton_onlygl;
  config_gui->radiobutton_onlynongl = radiobutton_onlynongl;
  config_gui->radiobutton_all_plugins = radiobutton_all_plugins;
  config_gui->spinbutton_fps = spinbutton_fps;

  config_gui->optionmenu_morph_plugin = optionmenu_morph_plugin;
  config_gui->optionmenu_morph_plugin_group = NULL;
  config_gui->button_morph_plugin_conf = button_morph_plugin_conf;
  config_gui->button_morph_plugin_about = button_morph_plugin_about;
  config_gui->checkbutton_morph_random = checkbutton_morph_random;

  config_gui->button_ok = button_ok;
  config_gui->button_apply = button_apply;
  config_gui->button_cancel = button_cancel;

  return config_gui;
}

/* This is a dummy pixmap we use when a pixmap can't be found. */
static char *dummy_pixmap_xpm[] = {
/* columns rows colors chars-per-pixel */
"1 1 1 1",
"  c None",
/* pixels */
" "
};

static GtkWidget *create_dummy_pixmap (GtkWidget *widget)
{
  GdkColormap *colormap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;

  colormap = gtk_widget_get_colormap (widget);
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, &mask,
                                                     NULL, dummy_pixmap_xpm);
  if (gdkpixmap == NULL)
    g_error (_("Couldn't create replacement pixmap."));
  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gdk_pixmap_unref (gdkpixmap);
  gdk_bitmap_unref (mask);
  return pixmap;
}

static GList *pixmaps_directories = NULL;

static void add_pixmap_directory (const gchar *directory)
{
  pixmaps_directories = g_list_prepend (pixmaps_directories,
                                        g_strdup (directory));
}

static GtkWidget *create_pixmap (GtkWidget *widget, const gchar *filename)
{
  gchar *found_filename = NULL;
  GdkColormap *colormap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;
  GList *elem;

  if (!filename || !filename[0])
      return create_dummy_pixmap (widget);

  /* We first try any pixmaps directories set by the application. */
  elem = pixmaps_directories;
  while (elem)
    {
      found_filename = check_file_exists ((gchar*)elem->data, filename);
      if (found_filename)
        break;
      elem = elem->next;
    }

  /* If we haven't found the pixmap, try the source directory. */
  if (!found_filename)
    {
      found_filename = check_file_exists ("../pixmaps", filename);
    }

  if (!found_filename)
    {
      g_warning (_("Couldn't find pixmap file: %s"), filename);
      return create_dummy_pixmap (widget);
    }

  colormap = gtk_widget_get_colormap (widget);
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm (NULL, colormap, &mask,
                                                   NULL, found_filename);
  if (gdkpixmap == NULL)
    {
      g_warning (_("Error loading pixmap file: %s"), found_filename);
      g_free (found_filename);
      return create_dummy_pixmap (widget);
    }
  g_free (found_filename);
  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gdk_pixmap_unref (gdkpixmap);
  gdk_bitmap_unref (mask);
  return pixmap;
}

static gchar *check_file_exists (const gchar *directory, const gchar *filename)
{
  gchar *full_filename;
  struct stat s;
  gint status;

  full_filename = (gchar*) g_malloc (strlen (directory) + 1
                                     + strlen (filename) + 1);
  strcpy (full_filename, directory);
  strcat (full_filename, G_DIR_SEPARATOR_S);
  strcat (full_filename, filename);

  status = stat (full_filename, &s);
  if (status == 0 && S_ISREG (s.st_mode))
    return full_filename;
  g_free (full_filename);
  return NULL;
}

/*static GtkWidget* lookup_widget (GtkWidget *widget, const gchar *widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = widget->parent;
      if (parent == NULL)
        break;
      widget = parent;
    }

  found_widget = (GtkWidget*) gtk_object_get_data (GTK_OBJECT (widget),
                                                   widget_name);
  if (!found_widget)
    g_warning ("Widget not found: %s", widget_name);
  return found_widget;
}*/

#endif
