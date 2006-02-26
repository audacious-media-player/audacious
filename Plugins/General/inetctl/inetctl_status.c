/* inetctl_status.c - status tracking and reporting module */

#include "inetctl.h"

/* Static "previous state" of player */
playerStatus lastPlayerStatus = {
	.infoValid = FALSE,
	.curTrackInPlaylist = 0,
	.totalTracksInPlaylist = 0,
	.curTrackLength = 0,
	.curTrackTime = 0,
	.trackTitle = {0, },
	.trackFile = {0, },
	.volume = 0,
	.balance = 0,
	.playing = FALSE,
	.paused = FALSE,
	.repeat = FALSE,
	.shuffle = FALSE
};

/* Option/controls */
Bool formattedTime = TRUE;
Bool timeStatusUpdates = TRUE;
Bool playlistUpdates = FALSE;

/* Status buffer, used for formatting status messages */
#define STATUS_BUFF_MAX 1024
char statusBuffer[STATUS_BUFF_MAX];

/* Fill the passed player status structure with the players current status */
Bool getPlayerStatus(playerStatusPtr theStatus) {
  /* Playlist data */
  theStatus->curTrackInPlaylist = xmms_remote_get_playlist_pos(inetctl.xmms_session);
  theStatus->totalTracksInPlaylist = xmms_remote_get_playlist_length(inetctl.xmms_session);

  /* Track data */
  theStatus->curTrackLength = xmms_remote_get_playlist_time(inetctl.xmms_session, theStatus->curTrackInPlaylist);
  theStatus->curTrackTime = xmms_remote_get_output_time(inetctl.xmms_session);
  strcpy(theStatus->trackTitle, xmms_remote_get_playlist_title(inetctl.xmms_session, theStatus->curTrackInPlaylist));
  strcpy(theStatus->trackFile, xmms_remote_get_playlist_file(inetctl.xmms_session, theStatus->curTrackInPlaylist));

  /* Player data */
  theStatus->playing = xmms_remote_is_playing(inetctl.xmms_session);
  theStatus->paused = xmms_remote_is_paused(inetctl.xmms_session);
  theStatus->shuffle = xmms_remote_is_shuffle(inetctl.xmms_session);
  theStatus->repeat = xmms_remote_is_repeat(inetctl.xmms_session);
  theStatus->volume = xmms_remote_get_main_volume(inetctl.xmms_session);
  theStatus->balance = xmms_remote_get_balance(inetctl.xmms_session);

  /* Mark valid and we are done */
  theStatus->infoValid = TRUE;
  return TRUE;
}

/* Set status of formatted times */
void setFormattedTime(Bool doFormattedTime) {
  formattedTime = doFormattedTime;
}

/* Return TRUE if doing formatted time */
Bool isFormattedTime() {
  return formattedTime;
}

/* Set status of time/progress updates */
void setTimeUpdates(Bool doTimeUpdates) {
  timeStatusUpdates = doTimeUpdates;
}

/* Set status of playlist updates */
void setPlaylistUpdates(Bool doPlaylistUpdates) {
  playlistUpdates = doPlaylistUpdates;
}

/* Write a quoted string out the the passed buffer */
void writeQuotedString(String destBuffer, String theText) {
  strcat(destBuffer, "\"");
  strcat(destBuffer, theText);
  strcat(destBuffer, "\"");
}

/* Format a number into a string */
void writeNumberString(String destBuffer, int theNumber) {
  sprintf(destBuffer, "%d", theNumber);
}

/* Format a passed # of seconds into a time */
void writeTimeString(String destBuffer, int theTime) {
  int hours, minutes, seconds;

  /* Handle unformatted times */
  if (!formattedTime) {
    writeNumberString(destBuffer, theTime);
    return;
  }

  /* Decode into "english" time */
  theTime /= 1000;
  hours = theTime / 3600;
  minutes = (theTime % 3600) / 60;
  seconds = theTime % 60;

  /* Format hours, if any */
  sprintf(destBuffer, "%d:%02d:%02d", hours, minutes, seconds);
}

/* Initialize status tracking */
Bool initStatusTracking() {
  return getPlayerStatus(&lastPlayerStatus);
}

/* Send track progress status */
Bool sendTrackStatus(playerStatusPtr theStatus) {
  /* Default the status, if needed */
  if (theStatus == NULL) theStatus = &lastPlayerStatus;

  /* Init the response buffer */
  statusBuffer[0] = '\0';

  /* Format the info */
  writeTimeString(statusBuffer, theStatus->curTrackTime);
  strcat(statusBuffer, " ");
  writeTimeString(&statusBuffer[strlen(statusBuffer)], theStatus->curTrackLength);

  /* Send Status to client */
  return sendResponse("013", statusBuffer);
}

/* Send playlist info */
Bool sendPlaylistStatus(playerStatusPtr theStatus) {
  /* Default the status, if needed */
  if (theStatus == NULL) theStatus = &lastPlayerStatus;

  /* Init the response buffer */
  statusBuffer[0] = '\0';

  /* Format the info */
  writeNumberString(statusBuffer, theStatus->curTrackInPlaylist);
  strcat(statusBuffer, " ");
  writeNumberString(&statusBuffer[strlen(statusBuffer)], theStatus->totalTracksInPlaylist);

  /* Send Status to client */
  return sendResponse("012", statusBuffer);
}

/* Send current playlist info */
Bool sendPlaylistTracks(playerStatusPtr theStatus) {
  int playListIndex;

  /* Default the status, if needed */
  if (theStatus == NULL) theStatus = &lastPlayerStatus;

  /* Send playlist header */
  strcpy(statusBuffer, "START ");
  writeNumberString(&statusBuffer[strlen(statusBuffer)], theStatus->totalTracksInPlaylist);
  strcat(statusBuffer, " ");
  writeQuotedString(&statusBuffer[strlen(statusBuffer)], "Unknown");  /* Title */
  strcat(statusBuffer, " ");\
  writeQuotedString(&statusBuffer[strlen(statusBuffer)], "Unknown");  /* File path/name */
  if (!sendResponse("014", statusBuffer)) return FALSE;

  /* Iterate over all tracks in the playlist */
  for (playListIndex = 0; playListIndex < theStatus->totalTracksInPlaylist; playListIndex++) {
    /* Format this tracks info */
    writeNumberString(statusBuffer, playListIndex);
    strcat(statusBuffer, " ");
    writeQuotedString(&statusBuffer[strlen(statusBuffer)], xmms_remote_get_playlist_title(inetctl.xmms_session, playListIndex));
    strcat(statusBuffer, " ");
    writeQuotedString(&statusBuffer[strlen(statusBuffer)], xmms_remote_get_playlist_file(inetctl.xmms_session, playListIndex));
    strcat(statusBuffer, " ");
    writeTimeString(&statusBuffer[strlen(statusBuffer)], xmms_remote_get_playlist_time(inetctl.xmms_session, playListIndex));
    if (!sendResponse("014", statusBuffer)) return FALSE;
  }

  /* Write the terminator */
  strcpy(statusBuffer, "END ");
  writeNumberString(&statusBuffer[strlen(statusBuffer)], theStatus->totalTracksInPlaylist);
  return sendResponse("014", statusBuffer);
}

/* Send info on current track */
Bool sendTrackInfo(playerStatusPtr theStatus) {
  int bitRate, sampleFreq, numChannels;

  /* Default the status, if needed */
  if (theStatus == NULL) theStatus = &lastPlayerStatus;

  /* Init the response buffer */
  statusBuffer[0] = '\0';

  /* Format track data */
  writeQuotedString(statusBuffer, theStatus->trackTitle);
  strcat(statusBuffer, " ");
  writeQuotedString(&statusBuffer[strlen(statusBuffer)], theStatus->trackFile);
  strcat(statusBuffer, " ");
  writeTimeString(&statusBuffer[strlen(statusBuffer)], theStatus->curTrackLength);

  /* Format track stats */
  xmms_remote_get_info(inetctl.xmms_session, &bitRate, &sampleFreq, &numChannels);
  strcat(statusBuffer, " ");
  writeNumberString(&statusBuffer[strlen(statusBuffer)], bitRate);
  strcat(statusBuffer, " ");
  writeNumberString(&statusBuffer[strlen(statusBuffer)], sampleFreq);
  strcat(statusBuffer, " ");
  writeNumberString(&statusBuffer[strlen(statusBuffer)], numChannels);

  /* Send info to client */
  return sendResponse("011", statusBuffer);
}

/* Format a player status message and dispatch it to the passed */
/* client.  If the passed client is -1, then all clients will   */
/* receive it.  If the passed status pointer is NULL, the last  */
/* known players stats are used.                                */
Bool sendPlayerStatus(playerStatusPtr theStatus) {
  /* Default the status, if needed */
  if (theStatus == NULL) theStatus = &lastPlayerStatus;

  /* Init the response buffer */
  statusBuffer[0] = '\0';

  /* Put in player state */
  if (theStatus->playing) {
    if (theStatus->paused)
      strcat(statusBuffer, "PAUSED");
    else
      strcat(statusBuffer, "PLAYING");
  } else
    strcat(statusBuffer, "STOPPED");

  /* Put in volume */
  strcat(statusBuffer, " ");
  writeNumberString(&statusBuffer[strlen(statusBuffer)], theStatus->volume);

  /* Put in shuffle mode flag */
  if (theStatus->shuffle) 
    strcat(statusBuffer, " SHUFFLE");
  else
    strcat(statusBuffer, " NORMAL");

  /* Put in repeat mode flag */
  if (theStatus->repeat)
    strcat(statusBuffer, " REPEAT");
  else
    strcat(statusBuffer, " SINGLE");

  /* Send Status to client */
  return sendResponse("010", statusBuffer);
}

/* Send new client status messages */
Bool sendNewClientStatus(int clientSocket) {
  /* Set initial status */
  initStatusTracking();

  /* Now send each status */
  if (!sendPlayerStatus(NULL)) return FALSE;
  if (!sendPlaylistStatus(NULL)) return FALSE;
  if (!sendTrackInfo(NULL)) return FALSE;
  if (!sendTrackStatus(NULL)) return FALSE;

  /* And we are done */
  return TRUE;
}

/* Used by the clients to update their idea of the */
/* players status                                  */
Bool freshenClientStatus() {
  return !getPlayerStatus(&lastPlayerStatus);
}

/* Check the player status.  If anything has changed since we */
/* last checked it's status, we issue an appropriate status   */
/* update to all connected clients                            */
Bool updatePlayerStatus(int clientSocket) {
  playerStatus curStatus;
  Bool playerIsStopped = FALSE;
  Bool statusChange = FALSE;
  Bool playListChanged = FALSE;
  Bool trackChanged = FALSE;

  /* Get player status */
  if (!getPlayerStatus(&curStatus)) return FALSE;

  /* See if the player is stopped */
  playerIsStopped = !curStatus.playing;

  /* See if we should create a 010 - player status change */
  if ((curStatus.playing != lastPlayerStatus.playing)
      || (curStatus.paused != lastPlayerStatus.paused)
      || (curStatus.volume != lastPlayerStatus.volume)
      || (curStatus.balance != lastPlayerStatus.balance)
      || (curStatus.repeat != lastPlayerStatus.repeat)
      || (curStatus.shuffle != lastPlayerStatus.shuffle)) {
    statusChange = TRUE;
    writeDebug("Change in player status (010)\n");
    sendPlayerStatus(&curStatus);
  }

  /* See if we should send 012 - playlist status change */
  if (((curStatus.curTrackInPlaylist != lastPlayerStatus.curTrackInPlaylist)
    || (curStatus.totalTracksInPlaylist != lastPlayerStatus.totalTracksInPlaylist))) {
    statusChange = TRUE;
    playListChanged = TRUE;
    writeDebug("Change in playlist status (012)\n");
    sendPlaylistStatus(&curStatus);

    /* If actual playlist changed (vs just our position), send that */
    if ((curStatus.totalTracksInPlaylist != lastPlayerStatus.totalTracksInPlaylist) && playlistUpdates) 
      sendPlaylistTracks(&curStatus);
  }

  /* See if we should send a 011 - track info change */
  if (!playerIsStopped 
      && (playListChanged
	  || (curStatus.curTrackLength != lastPlayerStatus.curTrackLength)
	  || strcmp(curStatus.trackTitle, lastPlayerStatus.trackTitle)
	  || strcmp(curStatus.trackFile, lastPlayerStatus.trackFile))) {
    statusChange = TRUE;
    trackChanged = TRUE;
    writeDebug("Change in track info (011)\n");
    sendTrackInfo(&curStatus);
  }

  /* See if we should send a 013 - track status updates */
  if (!playerIsStopped
      && timeStatusUpdates
      && (playListChanged
	  || trackChanged
	  || (curStatus.curTrackLength != lastPlayerStatus.curTrackLength)
	  || (curStatus.curTrackTime != lastPlayerStatus.curTrackTime))) {
    statusChange = TRUE;
    writeDebug("Change in track status (013)\n");
    sendTrackStatus(&curStatus);
  }

  /* If there was a change, copy it over */
  if (statusChange) {
    memcpy(&lastPlayerStatus, &curStatus, sizeof(playerStatus));
    writeDebug("Copied off current status for later\n");
  }

  /* And we are done */
  return TRUE;
}
