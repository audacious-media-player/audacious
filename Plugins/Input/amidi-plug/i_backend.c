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
* 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
*/

#include "i_backend.h"


gboolean i_str_has_pref_and_suff( const gchar *str , gchar *pref , gchar *suff )
{
  if ( (g_str_has_prefix( str , pref )) &&
       (g_str_has_suffix( str , suff )) )
    return TRUE;
  else
    return FALSE;
}

GSList * i_backend_list_lookup( void )
{
  GDir * backend_directory;
  GSList * backend_list = NULL;

  backend_directory = g_dir_open( AMIDIPLUGDATADIR , 0 , NULL );
  if ( backend_directory != NULL )
  {
    const gchar * backend_directory_entry = g_dir_read_name( backend_directory );
    while ( backend_directory_entry != NULL )
    {
      /* simple filename checking */
      if ( i_str_has_pref_and_suff( backend_directory_entry , "ap-" , ".so" ) == TRUE )
      {
        GModule * module;
        gchar * (*getapmoduleinfo)( gchar ** , gchar ** , gchar ** , gint * );
        gchar * module_pathfilename = g_strjoin( "" , AMIDIPLUGDATADIR , "/" ,
                                                 backend_directory_entry , NULL );
        /* seems to be a backend for amidi-plug , try to load it */
        module = g_module_open( module_pathfilename , 0 );
        if ( module == NULL )
          g_warning( "Error loading module %s - %s\n" , module_pathfilename , g_module_error() );
        else
        {
          /* try to get the module name */
          if ( g_module_symbol( module , "backend_info_get" , (gpointer*)&getapmoduleinfo ) )
          {
            /* module name found, ok! add its name and filename to the list */
            amidiplug_sequencer_backend_name_t * mn = g_malloc(sizeof(amidiplug_sequencer_backend_name_t));
            /* name and desc dinamically allocated */
            getapmoduleinfo( &mn->name , &mn->longname , &mn->desc , &mn->ppos );
            mn->filename = g_strdup(module_pathfilename); /* dinamically allocated */
            DEBUGMSG( "Backend found and added in list, filename: %s and lname: %s\n" ,
                      mn->filename, mn->longname );
            backend_list = g_slist_append( backend_list , mn );
          }
          else
          {
            /* module name not found, this is not a backend for amidi-plug */
            g_warning( "File %s is not a backend for amidi-plug!\n" , module_pathfilename );
          }
          g_module_close( module );
        }
      }
      backend_directory_entry = g_dir_read_name( backend_directory );
    }
    g_dir_close( backend_directory );
  }
  else
    g_warning( "Unable to open the backend directory %s\n" , AMIDIPLUGDATADIR );

  return backend_list;
}


void i_backend_list_free( GSList * backend_list )
{
  while ( backend_list != NULL )
  {
    amidiplug_sequencer_backend_name_t * mn = backend_list->data;
    g_free( mn->desc );
    g_free( mn->name );
    g_free( mn->longname );
    g_free( mn->filename );
    g_free( mn );
    backend_list = backend_list->next;
  }
  return;
}


gint i_backend_load( gchar * module_name )
{
  gchar * module_pathfilename = g_strjoin( "" , AMIDIPLUGDATADIR , "/ap-" , module_name , ".so" , NULL );
  DEBUGMSG( "loading backend '%s'\n" , module_pathfilename );
  backend.gmodule = g_module_open( module_pathfilename , 0 );

  if ( backend.gmodule != NULL )
  {
    gchar * (*getapmoduleinfo)( gchar ** , gchar ** , gchar ** , gint * );
    gboolean (*checkautonomousaudio)( void );
    g_module_symbol( backend.gmodule , "backend_init" , (gpointer *)&backend.init );
    g_module_symbol( backend.gmodule , "backend_cleanup" , (gpointer *)&backend.cleanup );
    g_module_symbol( backend.gmodule , "audio_info_get" , (gpointer*)&backend.audio_info_get );
    g_module_symbol( backend.gmodule , "audio_volume_get" , (gpointer *)&backend.audio_volume_get );
    g_module_symbol( backend.gmodule , "audio_volume_set" , (gpointer *)&backend.audio_volume_set );
    g_module_symbol( backend.gmodule , "sequencer_start" , (gpointer *)&backend.seq_start );
    g_module_symbol( backend.gmodule , "sequencer_stop" , (gpointer *)&backend.seq_stop );
    g_module_symbol( backend.gmodule , "sequencer_on" , (gpointer *)&backend.seq_on );
    g_module_symbol( backend.gmodule , "sequencer_off" , (gpointer *)&backend.seq_off );
    g_module_symbol( backend.gmodule , "sequencer_queue_tempo" , (gpointer *)&backend.seq_queue_tempo );
    g_module_symbol( backend.gmodule , "sequencer_queue_start" , (gpointer *)&backend.seq_queue_start );
    g_module_symbol( backend.gmodule , "sequencer_event_init" , (gpointer *)&backend.seq_event_init );
    g_module_symbol( backend.gmodule , "sequencer_event_noteon" , (gpointer *)&backend.seq_event_noteon );
    g_module_symbol( backend.gmodule , "sequencer_event_noteoff" , (gpointer *)&backend.seq_event_noteoff );
    g_module_symbol( backend.gmodule , "sequencer_event_keypress" , (gpointer *)&backend.seq_event_keypress );
    g_module_symbol( backend.gmodule , "sequencer_event_controller" , (gpointer *)&backend.seq_event_controller );
    g_module_symbol( backend.gmodule , "sequencer_event_pgmchange" , (gpointer *)&backend.seq_event_pgmchange );
    g_module_symbol( backend.gmodule , "sequencer_event_chanpress" , (gpointer *)&backend.seq_event_chanpress );
    g_module_symbol( backend.gmodule , "sequencer_event_pitchbend" , (gpointer *)&backend.seq_event_pitchbend );
    g_module_symbol( backend.gmodule , "sequencer_event_sysex" , (gpointer *)&backend.seq_event_sysex );
    g_module_symbol( backend.gmodule , "sequencer_event_tempo" , (gpointer *)&backend.seq_event_tempo );
    g_module_symbol( backend.gmodule , "sequencer_event_other" , (gpointer *)&backend.seq_event_other );
    g_module_symbol( backend.gmodule , "sequencer_output" , (gpointer *)&backend.seq_output );
    g_module_symbol( backend.gmodule , "sequencer_output_shut" , (gpointer *)&backend.seq_output_shut );
    g_module_symbol( backend.gmodule , "sequencer_get_port_count" , (gpointer *)&backend.seq_get_port_count );
    g_module_symbol( backend.gmodule , "backend_info_get" , (gpointer*)&getapmoduleinfo );
    g_module_symbol( backend.gmodule , "audio_check_autonomous" , (gpointer*)&checkautonomousaudio );
    getapmoduleinfo( &backend.name , NULL , NULL , NULL );
    backend.autonomous_audio = checkautonomousaudio();
    DEBUGMSG( "backend %s (name '%s') successfully loaded\n" , module_pathfilename , backend.name );
    backend.init();
    g_free( module_pathfilename );
    return 1;
  }
  else
  {
    g_warning( "unable to load backend '%s'\n" , module_pathfilename );
    g_free( module_pathfilename );
    return 0;
  }
}


gint i_backend_unload( void )
{
  if ( backend.gmodule != NULL )
  {
    DEBUGMSG( "unloading backend '%s'\n" , backend.name );
    backend.cleanup();
    g_module_close( backend.gmodule );
    DEBUGMSG( "backend '%s' unloaded\n" , backend.name );
    g_free( backend.name );
    backend.gmodule = NULL;
    return 1;
  }
  else
  {
    g_warning( "attempting to unload backend, but no backend is loaded\n" );
    return 0;
  }
}
