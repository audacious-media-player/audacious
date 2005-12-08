#include <gtk/gtk.h>


void
xs_cfg_samplerate_menu_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_oversample_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_emu_sidplay1_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_emu_sidplay2_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_emu_filters_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_filter_reset                    (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_filter_sync_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_filter2_reset                   (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_filter2_sync_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_mintime_enable_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_mintime_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
xs_cfg_maxtime_enable_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_maxtime_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
xs_cfg_sld_enable_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_sld_dbbrowse                    (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_stil_enable_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_stil_browse                     (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_hvsc_browse                     (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_ftitle_override_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_subauto_enable_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_subauto_min_only_toggled        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
xs_cfg_ok                              (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_cancel                          (GtkButton       *button,
                                        gpointer         user_data);

gboolean
xs_fileinfo_delete                     (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
xs_subctrl_prevsong                    (GtkButton       *button,
                                        gpointer         user_data);

void
xs_subctrl_nextsong                    (GtkButton       *button,
                                        gpointer         user_data);

void
xs_fileinfo_ok                         (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_sldb_fs_ok                      (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_sldb_fs_cancel                  (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_stil_fs_ok                      (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_stil_fs_cancel                  (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_hvsc_fs_ok                      (GtkButton       *button,
                                        gpointer         user_data);

void
xs_cfg_hvsc_fs_cancel                  (GtkButton       *button,
                                        gpointer         user_data);
