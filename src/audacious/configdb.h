#ifndef CONFIGDB_H
#define CONFIGDB_H

#include <glib.h>
#include <libmcs/mcs.h>

G_BEGIN_DECLS

    /**
     * cfg_db_open:
     *
     * Opens the configuration database.
     *
     * Return value: A configuration database handle.
     **/
    mcs_handle_t *cfg_db_open();

    /**
     * cfg_db_close:
     * @db: A configuration database handle.
     *
     * Closes the configuration database.
     **/
    void cfg_db_close(mcs_handle_t *db);

    /**
     * cfg_db_get_string:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to look up.
     * @value: Pointer to a buffer to put the data in.
     *
     * Searches the configuration database for a value.
     *
     * Return value: TRUE if successful, FALSE otherwise.
     **/
    gboolean cfg_db_get_string(mcs_handle_t *db,
                                   const gchar *section,
                                   const gchar *key,
                                   gchar **value);

    /**
     * cfg_db_get_int:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to look up.
     * @value: Pointer to an integer to put the data in.
     *
     * Searches the configuration database for a value.
     *
     * Return value: TRUE if successful, FALSE otherwise.
     **/
    gboolean cfg_db_get_int(mcs_handle_t *db,
                                const gchar *section,
                                const gchar *key,
                                gint *value);

    /**
     * cfg_db_get_bool:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to look up.
     * @value: Pointer to a boolean to put the data in.
     *
     * Searches the configuration database for a value.
     *
     * Return value: TRUE if successful, FALSE otherwise.
     **/
    gboolean cfg_db_get_bool(mcs_handle_t *db,
                                 const gchar *section,
                                 const gchar *key,
                                 gboolean *value);

    /**
     * cfg_db_get_float:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to look up.
     * @value: Pointer to a floating point integer to put the data in.
     *
     * Searches the configuration database for a value.
     *
     * Return value: TRUE if successful, FALSE otherwise.
     **/
    gboolean cfg_db_get_float(mcs_handle_t *db,
                                  const gchar *section,
                                  const gchar *key,
                                  gfloat *value);

    /**
     * cfg_db_get_double:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to look up.
     * @value: Pointer to a double-precision floating point integer to put the data in.
     *
     * Searches the configuration database for a value.
     *
     * Return value: TRUE if successful, FALSE otherwise.
     **/
    gboolean cfg_db_get_double(mcs_handle_t *db,
                                   const gchar *section,
                                   const gchar *key,
                                   gdouble *value);

    /**
     * cfg_db_set_string:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     * @value: Pointer to a buffer containing the data.
     *
     * Sets a value in the configuration database.
     **/
    void cfg_db_set_string(mcs_handle_t *db,
                               const gchar *section,
                               const gchar *key,
                               const gchar *value);

    /**
     * cfg_db_set_int:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     * @value: Pointer to an integer containing the data.
     *
     * Sets a value in the configuration database.
     **/
    void cfg_db_set_int(mcs_handle_t *db,
                            const gchar *section,
                            const gchar *key,
                            gint value);

    /**
     * cfg_db_set_bool:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     * @value: Pointer to a boolean containing the data.
     *
     * Sets a value in the configuration database.
     **/
    void cfg_db_set_bool(mcs_handle_t *db,
                             const gchar *section,
                             const gchar *key,
                             gboolean value);

    /**
     * cfg_db_set_float:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     * @value: Pointer to a floating point integer containing the data.
     *
     * Sets a value in the configuration database.
     **/
    void cfg_db_set_float(mcs_handle_t *db,
                              const gchar *section,
                              const gchar *key,
                              gfloat value);

    /**
     * cfg_db_set_double:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     * @value: Pointer to a double precision floating point integer containing the data.
     *
     * Sets a value in the configuration database.
     **/
    void cfg_db_set_double(mcs_handle_t *db,
                               const gchar *section,
                               const gchar *key,
                               gdouble value);

    /**
     * cfg_db_unset_key:
     * @db: A configuration database handle.
     * @section: The section of the configuration database to search.
     * @key: The name of the field in the configuration database to set.
     *
     * Removes a value from the configuration database.
     **/
    void cfg_db_unset_key(mcs_handle_t *db,
                              const gchar *section,
                              const gchar *key);

G_END_DECLS

#endif /* CONFIGDB_H */

