#ifndef __LV_BMP_CONFIG__
#define __LV_BMP_CONFIG__

#include <glib.h>

#define OPTIONS_MAX_NAME_LEN 256
#define OPTIONS_MAX_ICON_PATH_LEN 256

/**
 * User options information.
 * 
 * Just one of all_plugins_enabled, gl_plugins_only or non_gl_plugins_only
 * is enabled at a given time.
 */
typedef struct {

	const gchar *last_plugin;	/**< Name of the last plugin runned,
					  with length < OPTIONS_MAX_NAME_LEN. */

	gchar *morph_plugin; /**< */
	
	gchar *icon_file;	/**< Absolute path of the icon file,
				  with length < OPTIONS_MAX_ICON_PATH_LEN. */
	int width;		/**< Width in pixels. */
	int height;		/**< Height in pixels. */
	int fps;		/**< Maximum frames per second. */
	int depth;		/**< Color depth. */
	gboolean fullscreen;	/**< Say if we are in fullscreen or not. */

	gboolean gl_plugins_only;	/**< Only Gl plugins must be showed */
	gboolean non_gl_plugins_only;	/**< Only non GL plugins must be showed */
	gboolean all_plugins_enabled;	/**< All plugins must be showed */
	gboolean random_morph;		/**< Morph plugin will be selected randomly on
					  every switch. */

} Options;

void lv_bmp_config_window (void);

Options *lv_bmp_config_open (void);
int lv_bmp_config_close (void);

int lv_bmp_config_load_prefs (void);
int lv_bmp_config_save_prefs (void);

void lv_bmp_config_toggle_fullscreen (void);

const char *lv_bmp_config_get_next_actor (void);
const char *lv_bmp_config_get_prev_actor (void);
void lv_bmp_config_set_current_actor (const char *name);

const char *lv_bmp_config_morph_plugin (void);

#endif /* __LV_BMP_CONFIG__ */

