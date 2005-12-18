#ifndef __LV_CONFIG_GUI_H__
#define __LV_CONFIG_GUI_H__

#include <gtk/gtk.h>

typedef struct {
	GtkWidget *window_main;

	GtkWidget *list_vis_plugins;
	GtkWidget *button_vis_plugin_conf;
	GtkWidget *button_vis_plugin_about;

	GtkWidget *checkbutton_vis_plugin;
	GtkWidget *checkbutton_fullscreen;
	GtkWidget *radiobutton_onlygl;
	GtkWidget *radiobutton_onlynongl;
	GtkWidget *radiobutton_all_plugins;
	GtkWidget *spinbutton_fps;

	GtkWidget *optionmenu_morph_plugin;
	GSList *optionmenu_morph_plugin_group;
	GtkWidget *button_morph_plugin_conf;
	GtkWidget *button_morph_plugin_about;
	GtkWidget *checkbutton_morph_random;
	
	GtkWidget *button_ok;
	GtkWidget *button_apply;
	GtkWidget *button_cancel;
} ConfigWin;

ConfigWin *lv_bmp_config_gui_new (void);

#endif /* __LV_CONFIG_GUI_H__ */
