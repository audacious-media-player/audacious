/* inetctl_actions.c - This is where commands are interpreted and acted on */

/* For simplciity of installation, the actual command dispatch table */
/* is defined at the end of the source file.  This is done because other */
/* wise we'd have to have lots of forward references.                    */

/* I really wanted the COMMAND macro to not only define the function, but */
/* also add the function to the command dispatch table, but I couldn't    */
/* find a way to build up a list that would later be expanded.            */

/* As a result, a new command requires two definitions.  The COMMMAND def */
/* defines the name of the command and the function header for the code   */
/* executed for that command.  The REGISTER macro, located at the end of  */
/* the file, adds the command to the dispatch table.  The names in the    */
/* COMMAND macros must match the names in the REGISTER macros or you'll   */
/* either get undefined symbols or functions that are never invoked.      */

#include "inetctl.h"

/* Exported from inetctl_client.c */
extern pid_t clientPID;
extern int clientSocket;

#define COMMAND(NAME) Bool cmd_##NAME(int argc, String argv[])
#define REGISTER(NAME, PUBLIC) { #NAME, cmd_##NAME, PUBLIC }

COMMAND(DEBUG) {
  Bool debugFlag = TRUE;

  /* If there is a parameter, process it */
  if (argc >= 2) {
    upString(argv[1]);
    if (!strcmp(argv[1], "OFF")) debugFlag = FALSE;
  }

  debugMode = debugFlag;
  return TRUE;
}

COMMAND(USER) {
  return TRUE;
}

COMMAND(PLAY) {
  xmms_remote_play(inetctl.xmms_session);  
  return TRUE;
}

COMMAND(PLAY_FILE) {
  int argIndex;
  GList *theList = NULL;

  /* Add all files to the list */
  for (argIndex = 1; argIndex < argc; argIndex++) {
    theList = g_list_append(theList, argv[argIndex]);
  }

  /* Insure there is at least one file */
  if (theList == NULL) {
    sendResponse("003", "TOO FEW ARGUMENTS");
    return FALSE;
  }

  /* Clear the current list and add these file(s) to it */
  xmms_remote_stop(inetctl.xmms_session);
  xmms_remote_playlist_clear(inetctl.xmms_session);
  xmms_remote_playlist_add(inetctl.xmms_session, theList);
  xmms_remote_play(inetctl.xmms_session);

  /* And we are off to the races */
  return TRUE;
}

COMMAND(LOAD_FILE) {
  int argIndex;
  GList *theList = NULL;

  /* Add all files to the list */
  for (argIndex = 1; argIndex < argc; argIndex++) {
    theList = g_list_append(theList, argv[argIndex]);
  }

  /* Insure there is at least one file */
  if (theList == NULL) {
    sendResponse("003", "TOO FEW ARGUMENTS");
    return FALSE;
  }

  /* Clear the current list and add these file(s) to it */
  xmms_remote_stop(inetctl.xmms_session);
  xmms_remote_playlist_clear(inetctl.xmms_session);
  xmms_remote_playlist_add(inetctl.xmms_session, theList);

  /* And we are off to the races */
  return TRUE;
}

COMMAND(CLEAR_PLAYLIST) {
  int trackIndex;

  /* Stop the player (if playing) */
  xmms_remote_stop(inetctl.xmms_session);

  /* Clear the current list and add these file(s) to it */
  for (trackIndex = lastPlayerStatus.totalTracksInPlaylist - 1; trackIndex >= 0; trackIndex--) {
    xmms_remote_playlist_delete(inetctl.xmms_session, trackIndex);
  }

  return TRUE;
} 

COMMAND(APPEND_FILE) {
  int argIndex;
  GList *theList = NULL;

  /* Add all files to the list */
  for (argIndex = 1; argIndex < argc; argIndex++) {
    theList = g_list_append(theList, argv[argIndex]);
  }

  /* Insure there is at least one file */
  if (theList == NULL) {
    sendResponse("003", "TOO FEW ARGUMENTS");
    return FALSE;
  }

  /* Append this to the current list */
  xmms_remote_playlist_add(inetctl.xmms_session, theList);
  return TRUE;
}

COMMAND(REMOVE_TRACK) {
  int theTrackNum;
  String theEndChar;

  /* Convert integer to number */
  theTrackNum = strtol(argv[1], &theEndChar, 10);
  if ((*theEndChar != '\0') 
      || (theEndChar == argv[1]) 
      || (theTrackNum < 0) 
      || (theTrackNum >= lastPlayerStatus.totalTracksInPlaylist)) {
    sendResponse("004", "INVALID ARGUMENT");
    return FALSE;
  }

  xmms_remote_playlist_delete(inetctl.xmms_session, theTrackNum);
  return TRUE;
}

COMMAND(PAUSE) {
  if (xmms_remote_is_playing(inetctl.xmms_session) 
      && !xmms_remote_is_paused(inetctl.xmms_session)) {
    xmms_remote_pause(inetctl.xmms_session);
  }

  return TRUE;
}

COMMAND(RESUME) {
  if (xmms_remote_is_playing(inetctl.xmms_session) 
      && xmms_remote_is_paused(inetctl.xmms_session)) {
    xmms_remote_pause(inetctl.xmms_session);
  }

  return TRUE;
}

COMMAND(STOP) {
  xmms_remote_stop(inetctl.xmms_session);
  return TRUE;
}

COMMAND(NEXT_TRACK) {
  int theRepeatCount = 1, theCount = 0;
  String theEndChar;

  /* Convert integer to number */
  if (argc == 2) {
    theRepeatCount = strtol(argv[1], &theEndChar, 10);
    if ((*theEndChar != '\0') 
        || (theEndChar == argv[1]) 
        || (theRepeatCount < 1)) {
      sendResponse("004", "INVALID ARGUMENT");
      return FALSE;
    }
  }

  for (theCount = 0; theCount < theRepeatCount; theCount++) 
    xmms_remote_playlist_next(inetctl.xmms_session);

  return TRUE;
}

COMMAND(PREV_TRACK) {
  int theRepeatCount = 1, theCount = 0;
  String theEndChar;

  /* Convert integer to number */
  if (argc == 2) {
    theRepeatCount = strtol(argv[1], &theEndChar, 10);
    if ((*theEndChar != '\0') 
        || (theEndChar == argv[1]) 
        || (theRepeatCount < 1)) {
      sendResponse("004", "INVALID ARGUMENT");
      return FALSE;
    }
  }

  for (theCount = 0; theCount < theRepeatCount; theCount++) 
    xmms_remote_playlist_prev(inetctl.xmms_session);

  return TRUE;
}

COMMAND(JUMP_TO_TRACK) {
  int theTrackNum;
  String theEndChar;

  /* Convert integer to number */
  theTrackNum = strtol(argv[1], &theEndChar, 10);
  if ((*theEndChar != '\0') 
      || (theEndChar == argv[1]) 
      || (theTrackNum < 0) 
      || (theTrackNum >= lastPlayerStatus.totalTracksInPlaylist)) {
    sendResponse("004", "INVALID ARGUMENT");
    return FALSE;
  }

  xmms_remote_set_playlist_pos(inetctl.xmms_session, theTrackNum);
  return TRUE;
}

COMMAND(JUMP_TO_TIME) {
  int theTimeIndex;
  String theEndChar;

  /* Convert integer to number */
  theTimeIndex = strtol(argv[1], &theEndChar, 10);
  if ((*theEndChar != '\0') 
      || (theEndChar == argv[1]) 
      || (theTimeIndex < 0) 
      || (theTimeIndex >= lastPlayerStatus.curTrackLength)) {
    sendResponse("004", "INVALID ARGUMENT");
    return FALSE;
  }

  if (isFormattedTime())
    xmms_remote_jump_to_time(inetctl.xmms_session, theTimeIndex * 1000);
  else
    xmms_remote_jump_to_time(inetctl.xmms_session, theTimeIndex);

  return TRUE;
}

COMMAND(EXIT) {
  endThisClient();
  return FALSE;
}

COMMAND(QUIT) {
  xmms_remote_quit(inetctl.xmms_session);
  kill(serverPID, SIGTERM);
  return FALSE;
}

COMMAND(REPEAT) {
  Bool repeatMode = TRUE;

  /* If there is a parameter, process it */
  if (argc >= 2) {
    upString(argv[1]);
    if (!strcmp(argv[1], "OFF")) repeatMode = FALSE;
  }

  /* See if we need to change repeat mode */
  if (xmms_remote_is_repeat(inetctl.xmms_session) == repeatMode) 
    return TRUE;

  /* Toggle repeat mode */
  xmms_remote_toggle_repeat(inetctl.xmms_session);
  return TRUE;
}

COMMAND(SHUFFLE) {
  Bool shuffleMode = TRUE;

  /* If there is a parameter, process it */
  if (argc >= 2) {
    upString(argv[1]);
    if (!strcmp(argv[1], "OFF")) shuffleMode = FALSE;
  }

  /* See if we need to change shuffle mode */
  if (xmms_remote_is_shuffle(inetctl.xmms_session) == shuffleMode) 
    return TRUE;

  /* Toggle shuffle mode */
  xmms_remote_toggle_shuffle(inetctl.xmms_session);
  return TRUE;
}

COMMAND(VOLUME) {
  int newVolume;
  String endChar;

  /* There must be an additional parameter */
  if (argc != 2) {
    sendResponse("003", "TOO FEW ARGUMENTS");
    return FALSE;
  }

  /* Convert to volume */
  trimString(argv[1]);
  newVolume = strtol(argv[1], &endChar, 10);
  if ((endChar == argv[1]) || (*endChar != '\0') || (newVolume < 0) || (newVolume > 100)) {
    sendResponse("004", "INVALID ARGUMENT");
    return FALSE;
  }

  /* Install new volume */
  xmms_remote_set_main_volume(inetctl.xmms_session, newVolume);
  return TRUE;
}

COMMAND(HIDE_PLAYER) {
  /* Set Players Visibility */
  xmms_remote_pl_win_toggle(inetctl.xmms_session, FALSE);
  xmms_remote_eq_win_toggle(inetctl.xmms_session, FALSE);
  xmms_remote_main_win_toggle(inetctl.xmms_session, FALSE);
  return TRUE;
}

COMMAND(TIME_UPDATES) {
  Bool timeUpdates = TRUE;

  /* If there is a parameter, process it */
  if (argc >= 2) {
    upString(argv[1]);
    if (!strcmp(argv[1], "OFF")) timeUpdates = FALSE;
  }

  /* See if we need to change flag */
  setTimeUpdates(timeUpdates);
  return TRUE;
}

COMMAND(PLAYLIST_UPDATES) {
  Bool playlistUpdates = TRUE;

  /* If there is a parameter, process it */
  if (argc >= 2) {
    upString(argv[1]);
    if (!strcmp(argv[1], "OFF")) playlistUpdates = FALSE;
  }

  /* See if we need to change flag */
  setPlaylistUpdates(playlistUpdates);
  return TRUE;
}


COMMAND(FORMATTED_TIME) {
  Bool formattedTime = TRUE;

  /* If there is a parameter, process it */
  if (argc >= 2) {
    upString(argv[1]);
    if (!strcmp(argv[1], "OFF")) formattedTime = FALSE;
  }

  /* See if we need to change flag */
  writeDebug("Setting formatted time to %d\n", formattedTime);
  setFormattedTime(formattedTime);
  return TRUE;
}

COMMAND(REQUEST) {
  /* There must be an additional parameter */
  if (argc != 2) {
    sendResponse("003", "TOO FEW ARGUMENTS");
    return FALSE;
  }

  /* Upcase our option */
  upString(argv[1]);

  /* Freshen the clients stats */
  freshenClientStatus();

  /* Dispatch it */
  if (!strcmp(argv[1], "STATUS")) 
    return sendPlayerStatus(NULL);
  else if (!strcmp(argv[1], "TRACK"))
    return sendTrackInfo(NULL);
  else if (!strcmp(argv[1], "PLAYLIST"))
    return sendPlaylistStatus(NULL);
  else if (!strcmp(argv[1], "POSITION"))
    return sendTrackStatus(NULL);
  else if (!strcmp(argv[1], "ALL")) {
    sendPlayerStatus(NULL);
    sendTrackInfo(NULL);
    sendPlaylistStatus(NULL);
    sendTrackStatus(NULL);
  } else if (!strcmp(argv[1], "PLAYLIST_TRACKS"))
    return sendPlaylistTracks(NULL);
  else {
    sendResponse("004", "UNRECOGNIZED ARGUMENT");
    return FALSE;
  }

  return TRUE;
}

/* Build command table */
cmdTable commandTable[] = {
  REGISTER(DEBUG, FALSE),
  REGISTER(USER, TRUE),
  REGISTER(PLAY, FALSE),
  REGISTER(PLAY_FILE, FALSE),
  REGISTER(LOAD_FILE, FALSE),
  REGISTER(APPEND_FILE, FALSE),
  REGISTER(REMOVE_TRACK, FALSE),
  REGISTER(CLEAR_PLAYLIST, FALSE),
  REGISTER(PAUSE, FALSE),
  REGISTER(RESUME, FALSE),
  REGISTER(NEXT_TRACK, FALSE),
  REGISTER(PREV_TRACK, FALSE),
  REGISTER(JUMP_TO_TRACK, FALSE),
  REGISTER(JUMP_TO_TIME, FALSE),
  REGISTER(STOP, FALSE),
  REGISTER(VOLUME, FALSE),
  REGISTER(SHUFFLE, FALSE),
  REGISTER(REPEAT, FALSE),
  REGISTER(HIDE_PLAYER, FALSE),
  REGISTER(TIME_UPDATES, FALSE),
  REGISTER(FORMATTED_TIME, FALSE),
  REGISTER(PLAYLIST_UPDATES, FALSE),
  REGISTER(REQUEST, FALSE),
  REGISTER(EXIT, TRUE),
  REGISTER(QUIT, FALSE),
  { "", NULL, FALSE }  /* This must be the LAST line of the table */
};


