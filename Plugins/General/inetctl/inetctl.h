/* inetctl.h - common includes for inetctl */

#include <gtk/gtk.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <asm/errno.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <glib.h>
#include <string.h>
#include <ctype.h>
#include "libaudacious/beepctrl.h"
#include "libaudacious/configdb.h"
#include "audacious/plugin.h"

#include "config.h"

#define INETCTLVERSION "1.2"
#define INETCTLVERSION_MAJOR	1
#define INETCTLVERSION_MINOR	2
#define INETCTLVERSION_MICRO	0

#define CONFIGFILE "/.xmms/inetctl"

#define SYSCALL(call) while(((int) (call) == -1) && (errno == EINTR))

typedef char * String;

typedef int Bool;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* Port we wait for connections on */
extern int listenPort;
extern pid_t serverPID;

/* Status flags */
extern Bool debugMode;
extern Bool inetctlEnabled;

/* User flags */
extern Bool userAuthenticated;
extern String userName;

/* Command dispatch table */
typedef Bool (* _cmdFunc)(int, String[]);
struct _cmdTable {
  char commandName[32];
  _cmdFunc commandFunc;
  Bool alwaysAllowed;
};
typedef struct _cmdTable cmdTable;
typedef struct _cmdTable * cmdTablePtr;

extern cmdTable commandTable[];

/* Player status info */
#define PLAYER_TEXT_MAX 512
struct _playerStatDef {
  Bool infoValid;

  /* Playlist info */
  gint curTrackInPlaylist;
  gint totalTracksInPlaylist;

  /* Track Info */
  gint curTrackLength;
  gint curTrackTime;
  char trackTitle[PLAYER_TEXT_MAX];
  char trackFile[PLAYER_TEXT_MAX];

  /* Player info */
  gint volume, balance;
  gboolean playing, paused, repeat, shuffle;
};
typedef struct _playerStatDef playerStatus;
typedef struct _playerStatDef * playerStatusPtr;

extern GeneralPlugin inetctl;

/* Public methods invoked by xmms (from inetctl.c) */
extern void inetctl_init();
extern void inetctl_about();
extern void inetctl_config();
extern void inetctl_cleanup();

/* Support functions in inetctl.c */
extern void writeDebug(String, ...);
extern void writeError(String, ...);

/* Command parse in inetctl_command.c */
extern void upString(String);
extern void trimString(String);
extern Bool dispatchCommand(String);
extern void endThisClient();

/* Configuration support in inetctl_config.c */
extern void read_config();

/* Client support in inetctl_client.c */
extern Bool sendResponse(String, String, ...);
extern void processClientRequests(pid_t, int);

/* Status support in inetctl_status.c */
extern playerStatus lastPlayerStatus;
extern Bool initStatusTracking();
extern Bool updatePlayerStatus(int);
extern Bool sendNewClientStatus(int);
extern void setFormattedTime(Bool);
extern Bool isFormattedTime();
extern void setTimeUpdates(Bool);
extern void setPlaylistUpdates(Bool);
extern Bool freshenClientStatus();
extern Bool sendPlayerStatus(playerStatusPtr);
extern Bool sendPlaylistStatus(playerStatusPtr);
extern Bool sendPlaylistTracks(playerStatusPtr);
extern Bool sendTrackInfo(playerStatusPtr);
extern Bool sendTrackStatus(playerStatusPtr);


