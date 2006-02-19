/*
        xmms_inetctl - Network control port for XMMS 
        Copyright (C) 2002 Gerald Duprey <gerry@cdp1802.org>
	  based on xmms-mpcp by Vadim "Kryptolus" Berezniker <vadim@berezniker.com>
          in turn, based on work from xmms-goodnight

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/* The protocol looks much like the FTP protocol.  All commands and responses */
/* are line oriented text.  Commands are in upper case.  Responses have a     */
/* numeric code indentifying the response, followed by parameters. Each token */
/* is seperated by a space.  Parameters can be enclosed by single or double   */
/* quotes, especially important when the parameter has embedded spaces.  The  */
/* line must be terminated with a CR and LF (ASCII 0x0d, then 0x0a).  Extra   */
/* spaces between parameters are ignored.  Blank lines or lines begining with */
/* a # are ignored by the server.  Responses can be solicited (i.e. in reply  */
/* to a REQUEST command) or unsolicited (i.e. in response to the server       */
/* moving on to a new track).  There is no guarantee that the first response  */
/* the client receives after sending a command is in anyway related to that   */
/* command (i.e. you can't send a REQUEST CUR_TRACK and assume the next reply */
/* the server sends is the track info).  Stateless is the way to go here.     */

/* Commands:
 * USER name PASSWORD word       - Authenticate user
 * PLAY                          - Resume playing (after pause or stop)
 * PLAY_FILE path/filename.mp3   - Load and play the file.  There can be multiple
 *                                 files and the files can be MP3s and M3Us
 *                                 Any existing playlist is cleared first.
 * LOAD_FILE path/filename.m3u   - Load the file.  This is the same as PLAY_FILE
 *                                 except it won't automatically start playing
 *                                 after loading.
 * PAUSE                         - Pauses playing
 * RESUME                        - Result playing (same as PLAY)
 * NEXT_TRACK                    - Move on to next track
 * PREV_TRACK                    - Move back to previous track
 * STOP                          - Stops player
 * EXIT                          - Closes this connection
 * QUIT                          - Closes connection and shutdown XMMS
 * VOLUME n                      - Sets volume to n% (0-100, 0=Mute)
 * SHUFFLE on/off                - Sets shuffle mode on or off
 * REPEAT on/off                 - Sets play repeat mode on or off
 *
 * Per Client commands (these affect only your connection)
 * TIME_UPDATES on/off           - Sets progress in time messages are sent during play (default on)
 * FORMATTED_TIME on/off         - Sets whether time reports are formatted (h:mm:ss) or
 *                                 just raw seconds.  Default is on.
 * REQUEST STATUS                - Returns player status
 * REQUEST TRACK                 - Returns current track info via 011
 * REQUEST PLAYLIST              - Returns current playlist info via 012
 * REQUEST POSITION              - Returns current position in song via 013
 * REQUEST ALL                   - Causes all status (010-019) to be returned in order
 */

/* Responses:
 *
 * NOTE: Future versions will not change the data being sent, but MAY
 * add additional parameters on to the end of the status.  Don't assume
 * that the number of parameters will never change.  It won't get 
 * smaller, but it may grow.  As long as you can handle that (and
 * ignore it until your code is reved to support the new stuff), you'll
 * be fine.
 *
 * Times are always represented in hours:minutes:seconds.  Minutes
 * and seconds will always be two digitis.  Hours will be as
 * many digits as necessary, but always at least one (for 0).

 * 000 - Welcome/connection message
 *    000 hostname XMMS_NETCTL server (Version theversion) ready.
 *    000 homer.cdp1802.org XMMS_NETCTL server (Version V0.2.0) ready.
 * 001 - Authentication needed/failure indicator
 *    001 NOT AUTHENTICATED
 * 002 - Unrecognized command or parameters 
 *    002 UNKNOWN COMMAND
 * 003 - Too few/many arguments
 *    003 TOO FEW ARGUMENTS
 * 004 - Unrecognized Argument/invalid argument
 *    004 INVALID ARGUMENT
 *
 * 010 - Current player status
 *    010 state volume shuffle_mode repeat_mode
 *    010 PLAYING 100 SHUFFLE REPEAT
 * 011 - Current track info
 *    011 title filename length rate freq chans
 *    011 "They Might Be Giants - Twisting" "/usr/opt/mp3/tmbg/flood/twisting.mp3" 0:1:57 128 44 2
 * 012 - Current Playlist info
 *    012 curtrack totaltracks 
 *    012 5 104
 * 013 - Track playing status update
 *    013 timeintrack totaltracktime
 *    013 0:01:20 0:04:31
 */

/* Include commons */
#include "inetctl.h"

/* Define client managment structures */
struct _clientDef {
  pid_t clientPID;
  int clientSocket;
};
typedef struct _clientDef clientDef;
typedef struct _clientDef * clientDefPtr;

/* Port we wait for connections on */
int listenPort = 1586;
int listenSocket = -1;

/* Status flags */
Bool debugMode = FALSE;
Bool inetctlEnabled = TRUE;
pid_t serverPID = -1;

/* Plugin definition structure */
GeneralPlugin inetctl = {
  NULL,
  NULL,
  -1,
  "iNetControl " INETCTLVERSION,
  inetctl_init,
  inetctl_about,
  inetctl_config,
  inetctl_cleanup,
};

/* Define client tracking */
#define CLIENT_MAX 64
int clientCount = 0;
clientDef clientInfo[CLIENT_MAX];

/* Define formatting buffer */
#define FORMAT_BUFFER_MAX 1024
char formatBuffer[FORMAT_BUFFER_MAX];

/* Forward declarations */
Bool removeClient(int);
Bool acceptNewClients(int);
Bool configureServer();

/* Shutdown the server */
void shutdownServer() {
  int clientIndex;

  writeDebug("Shutting down server\n");

  /* Shutdown all children first */
  for (clientIndex = clientCount - 1; clientIndex >= 0; clientIndex--) {
    removeClient(clientInfo[clientIndex].clientPID);
  }

  /* Close socket */
  if (listenSocket != -1) {
    close(listenSocket);
    listenSocket = -1;
  }

  /* Now we stop ourselves/server */
  if (serverPID != -1) {
    writeDebug("Killing server process %d\n", getpid());
    kill(getpid(), SIGKILL);
    serverPID = -1;
  }
}

/* Remove an existing client from the client list */
Bool removeClient(int theClientPID) {
  int clientIndex;
  int purgeIndex;

  for (clientIndex = 0; clientIndex < clientCount; clientIndex++) {
    /* See if this is our client */
    if (clientInfo[clientIndex].clientPID == theClientPID) {
      /* Close the connection */
      close(clientInfo[clientIndex].clientSocket);
      
      /* Kill the client */
      kill(clientInfo[clientIndex].clientPID, SIGKILL);

      /* Shift all other clients down */
      for (purgeIndex = clientIndex; purgeIndex < (clientCount - 1); purgeIndex++) 
	clientInfo[purgeIndex] = clientInfo[purgeIndex + 1];

      /* Down the client count & we ar done */
      clientCount--;
      writeDebug("Removed client %d, %d clients remaining\n", theClientPID, clientCount);
      return TRUE;
    }
  }

  /* If we got here, there was no match */
  return FALSE;
}

/* Add the passed client to the list */
Bool addClient(pid_t theClientPID, int theClientSocket) {
  /* Make sure there is room */
  if (clientCount == CLIENT_MAX) return FALSE;

  /* Add in info */
  clientInfo[clientCount].clientPID = theClientPID;
  clientInfo[clientCount].clientSocket = theClientSocket;
  clientCount++;

  writeDebug("Spawned child %d to handle client, now %d clients\n", theClientPID, clientCount);
  
  return TRUE;
}

/* Handle termination signal */
void termHandler(int sigNum) {
  shutdownServer();
  return;
}

/* Handle children ending/dying */
void mournChildren(int sigNum) {
  int childPID, childStatus;

  /* Check for any child processes that have "silently" died */
  while ((childPID = waitpid(-1, &childStatus, WNOHANG))) {
    /* No children means just get the heck out (quietly) */
    if ((childPID < 0) && (errno == 10)) break;

    /* Handle an error */
    if (childPID < 0) {
      writeError("Error while mourning children, %s (%d)\n", strerror(errno), errno);
    } else {
      /* We are done */
      removeClient(childPID);
      writeDebug("Recognized that child %d died\n", childPID);
    }
  }

  /* Resinstall handler */
  signal(SIGCHLD, mournChildren);
}

/* Write a message when in debug mode */
void writeDebug(String theFormat, ...) {
  va_list theParms;

  /* Skip out if not debugging */
  if (!debugMode) return;

  /* Format and print message */
  va_start(theParms, theFormat);
  fprintf(stderr, "inetctl DEBUG [%6d]: ", getpid());
  vfprintf(stderr, theFormat, theParms);
  va_end(theParms);
}

/* Write an error message */
void writeError(String theFormat, ...) {
  va_list theParms;

  /* Format and print message */
  va_start(theParms, theFormat);
  fprintf(stderr, "inetctl ERROR [%6d]: ", getpid());
  vfprintf(stderr, theFormat, theParms);
  va_end(theParms);
}

GeneralPlugin * get_gplugin_info () {
  writeDebug("XMMS Asked for inetctl info\n");
  return (&inetctl);
}

void inetctl_cleanup() {
  int serverStatus = -1;

  writeDebug("Cleanup module started\n");

  /* Clear enabled indicator */
  inetctlEnabled = FALSE;

  /* Shutdown the server and free resources */
  if (serverPID != -1) {
    writeDebug("Shuttingdown inetctl server\n");
    kill(serverPID, SIGTERM);
    waitpid(serverPID, &serverStatus, 0);
  }

  writeDebug("Module cleanup complete\n");
}

/* Initialize this module */
void inetctl_init() {
  int errval;
  int reuseFlag = 1;
  struct sockaddr_in server;
  struct protoent * tcpInfo;

  writeDebug("Begining module initialization\n");

  /* lookup tcp protocol info */
  tcpInfo = getprotobyname("tcp");
  if (tcpInfo == NULL) {
    writeError("Unable lookup info on TCP protocol\n");
    exit(1);
  }

  /* Read the configuration for this module */
  read_config();

  /* create the main socket */
  listenSocket = socket(PF_INET, SOCK_STREAM, tcpInfo->p_proto);
  setsockopt(listenSocket,  SOL_SOCKET, SO_REUSEADDR, &reuseFlag, sizeof(reuseFlag));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(listenPort);
  
  /* bind the socket */
  if ((errval = bind(listenSocket, (struct sockaddr *) &server, sizeof(server))) == -1) {
    writeError("Attempt to bind to port %d failed - %s (%d)\n",
	       listenPort, strerror(errno), errno);
    close(listenSocket);
    listenSocket = -1;
    return;
  }

  /* Initiate active listening on our port */
  if ((errval = listen(listenSocket, 10)) == -1) {
    writeError("Attempt to listen on port %d failed - %s (%d)\n", 
	       listenPort, strerror(errno), errno); 
    close(listenSocket);
    listenSocket = -1;
    exit(1);
  }

  /* Install a child death handler */
  signal(SIGCHLD, mournChildren);

  /* Fork off server */
  switch(serverPID = fork()) {
  case -1:
    writeError("Attempt to fork failed - %s (%d)\n", strerror(errno), errno);
    close(listenSocket);
    listenSocket = -1;
    exit(1);
    break;

  case 0:
    /* Configure the server */
    configureServer();

    /* Process client connections */
    writeDebug("Now ready to accept client connections\n");
    acceptNewClients(listenSocket);
    shutdownServer();
    break;

  default:
    writeDebug("Server installed as PID %d\n", serverPID);
    close(listenSocket);
    listenSocket = -1;
    break;
  }

  /* Flag ourselves (re)initialized */
  inetctlEnabled = TRUE;

  writeDebug("Intialization complete\n");
}

/* Do one-shot server initialization */
Bool configureServer() {
  /* Get our PID */
  serverPID = getpid();

  /* Install us as the process group leader */
  setpgrp();

  /* Catch requests for us to shut down */  
  signal(SIGTERM, termHandler);

  /* Initialize status tracking */
  if (!initStatusTracking()) return FALSE;

  /* Server ready to rock and roll */
  return TRUE;
}

/* Check if the recently connected client, described by clientInfo, */
/* is valid (i.e. matches any security we have installed).  If so,  */
/* return TRUE.  Otherwise, return FALSE.                           */
Bool isValidClient(struct sockaddr_in * clientInfo) {
  /* For now, we are disabling authentication */
  userAuthenticated = TRUE;
  userName = "TEST";

  /* IP Security and such would go here */
  return TRUE;
}

/* Client connection handlers inherit lots of stuff we */
/* not need or want from the server.  Here we drop that*/
/* which isn't needed for a client                     */
void closeInheritedResources() {
  int clientIndex;

  /* Close servers listen socket, if open */
  if (listenSocket != -1) close(listenSocket);
  listenSocket = -1;

  /* Close any clients too */
  for (clientIndex = 0; clientIndex < clientCount; clientIndex++) {
    close(clientInfo[clientIndex].clientSocket);
    clientInfo[clientIndex].clientSocket = -1;
    clientInfo[clientIndex].clientPID = -1;
  }
  clientCount = 0;
}

/* Process incoming client connections.  Each incoming connection */
/* gets it own process to handle the connection.                  */
Bool acceptNewClients(int mainSocket) {
  int errval;
  pid_t clientPID;
  int clientSocket;
  socklen_t len;
  struct pollfd connWait;
  struct sockaddr_in client;

  /* Initialize polling wait structure */
  connWait.fd = mainSocket;
  connWait.events = POLLIN;

  /* We repeatedly process incomming connections */
  while(inetctlEnabled) {
    /* Wait for a new connection */
    SYSCALL(errval = poll(&connWait, 1, -1));
    if (errval == -1) {
      writeError("Poll waiting for new connections failed - %s (%d)\n",
		 strerror(errno), errno);
      return FALSE;
    }

    /* See if we got a timeout */
    if (errval == 0) {
      updatePlayerStatus(-1);
      continue;
    }

    /* Accept a new connection */
    if ((clientSocket = accept(mainSocket, (struct sockaddr *) &client, &len)) == -1) {
      writeError("Unable to accept new client connection - %s (%d)\n",
		 strerror(errno), errno);
      continue;
    }

    /* Do security checks on this connection */
    if (!isValidClient(&client)) {
      writeDebug("Closing client connection - failed security check\n");
      close(clientSocket);
      clientSocket = -1;
      continue;
    }

    /* Fork off a child to handle this client connection */
    switch(clientPID = fork()) {
    case -1:
      writeError("Attempt to form new client connection failed - %s (%d)\n",
		 strerror(errno), errno);
      return FALSE;
      break;

    case 0:
      /* Close off inherited resources */
      closeInheritedResources();

      /* Handle this clients traffic */
      processClientRequests(getpid(), clientSocket);
      removeClient(clientPID);
      exit(0);
      break;

    default:
      /* We've handed that client off to the child - close ours */
      addClient(clientPID, clientSocket);
      break;
    }
  }

  /* Processing completed */
  return TRUE;
}
