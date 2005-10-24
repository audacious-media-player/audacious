#ifndef CONFIGDB_H
#define CONFIGDB_H

#include <glib.h>


typedef struct _ConfigDb ConfigDb;


G_BEGIN_DECLS

    ConfigDb *bmp_cfg_db_open();
    void bmp_cfg_db_close(ConfigDb *db);

    gboolean bmp_cfg_db_get_string(ConfigDb *db,
                                   const gchar *section,
                                   const gchar *key,
                                   gchar **value);
    gboolean bmp_cfg_db_get_int(ConfigDb *db,
                                const gchar *section,
                                const gchar *key,
                                gint *value);
    gboolean bmp_cfg_db_get_bool(ConfigDb *db,
                                 const gchar *section,
                                 const gchar *key,
                                 gboolean *value);
    gboolean bmp_cfg_db_get_float(ConfigDb *db,
                                  const gchar *section,
                                  const gchar *key,
                                  gfloat *value);
    gboolean bmp_cfg_db_get_double(ConfigDb *db,
                                   const gchar *section,
                                   const gchar *key,
                                   gdouble *value);

    void bmp_cfg_db_set_string(ConfigDb *db,
                               const gchar *section,
                               const gchar *key,
                               gchar *value);
    void bmp_cfg_db_set_int(ConfigDb *db,
                            const gchar *section,
                            const gchar *key,
                            gint value);
    void bmp_cfg_db_set_bool(ConfigDb *db,
                             const gchar *section,
                             const gchar *key,
                             gboolean value);
    void bmp_cfg_db_set_float(ConfigDb *db,
                              const gchar *section,
                              const gchar *key,
                              gfloat value);
    void bmp_cfg_db_set_double(ConfigDb *db,
                               const gchar *section,
                               const gchar *key,
                               gdouble value);

    void bmp_cfg_db_unset_key(ConfigDb *db,
                              const gchar *section,
                              const gchar *key);

G_END_DECLS

#endif // CONFIGDB_H

