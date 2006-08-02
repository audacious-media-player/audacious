/*
*
* Author: Giacomo Lozito <james@develia.org>, (C) 2005-2006
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*
*/


#include "i_pcfg.h"


pcfg_t * i_pcfg_new( void )
{
  pcfg_t * configfile = g_key_file_new();
  return configfile;
}


pcfg_t * i_pcfg_new_from_file( gchar * pathfilename )
{
  pcfg_t * configfile = g_key_file_new();
  if ( g_key_file_load_from_file( configfile , pathfilename , G_KEY_FILE_NONE , NULL ) == FALSE )
  {
    g_key_file_free( configfile );
    configfile = NULL;
  }
  return configfile;
}


void i_pcfg_free( pcfg_t * configfile )
{
  g_key_file_free( configfile );
  return;
}


gboolean i_pcfg_write_to_file( pcfg_t * configfile , gchar * pathfilename )
{
  GError * error = NULL;
  gchar *configfile_string = g_key_file_to_data( configfile , NULL , &error );
  if ( error != NULL )
  {
    g_clear_error( &error );
    return FALSE;
  }
  else
  {
    if ( g_file_set_contents( pathfilename , configfile_string , -1 , NULL ) == FALSE )
    {
      g_free( configfile_string );
      return FALSE;
    }
    else
    {
      g_free( configfile_string );
      return TRUE;
    }
  }
}


gboolean i_pcfg_read_string( pcfg_t * configfile ,
                             const gchar * group ,
                             const gchar * key ,
                             gchar ** value ,
                             gchar * default_value )
{
  GError * error = NULL;
  *value = g_key_file_get_string( configfile , group , key , &error );
  if ( error != NULL )
  {
    if ( default_value != NULL )
      *value = g_strdup( default_value );
    g_clear_error( &error );
    return FALSE;
  }
  return TRUE;
}


gboolean i_pcfg_read_integer( pcfg_t * configfile ,
                              const gchar * group ,
                              const gchar * key ,
                              gint * value ,
                              gint default_value )
{
  GError * error = NULL;
  *value = g_key_file_get_integer( configfile , group , key , &error );
  if ( error != NULL )
  {
    *value = default_value;
    g_clear_error( &error );
    return FALSE;
  }
  return TRUE;
}


gboolean i_pcfg_read_boolean( pcfg_t * configfile ,
                              const gchar * group ,
                              const gchar * key ,
                              gboolean * value ,
                              gboolean default_value )
{
  GError * error = NULL;
  *value = g_key_file_get_boolean( configfile , group , key , &error );
  if ( error != NULL )
  {
    *value = default_value;
    g_clear_error( &error );
    return FALSE;
  }
  return TRUE;
}


gboolean i_pcfg_write_string( pcfg_t * configfile ,
                              const gchar * group ,
                              const gchar * key ,
                              gchar * value )
{
  g_key_file_set_string( configfile , group , key , value );
  return TRUE;
}


gboolean i_pcfg_write_integer( pcfg_t * configfile ,
                               const gchar * group ,
                               const gchar * key ,
                               gint value )
{
  g_key_file_set_integer( configfile , group , key , value );
  return TRUE;
}


gboolean i_pcfg_write_boolean( pcfg_t * configfile ,
                               const gchar * group ,
                               const gchar * key ,
                               gboolean value )
{
  g_key_file_set_boolean( configfile , group , key , value );
  return TRUE;
}
