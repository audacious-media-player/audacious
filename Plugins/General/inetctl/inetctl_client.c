/* inetctl_client.c - client connection handler */

/* All code in here is related only to the client processing */
/* done for a particular client session.  Overall client     */
/* management is done in inetctl.c                            */

/* NOTE: All code running in this module is in the process   */
/* space of the child servicing a particular connection.  It */
/* does not have access to data in inetctl.                   */

#include "inetctl.h"

/* Current client socket (for client handler) */
int clientSocket = -1;
pid_t clientPID = -1;

/* Buffers */
#define TEXT_BUFFER_SIZE 1024
#define READ_BUFFER_SIZE 32768
char textBuffer[TEXT_BUFFER_SIZE];
char readBuffer[READ_BUFFER_SIZE];
int readBufferLength = 0;
int readBufferPtr = 0;

/* Shuts us down.  We don't return */
void endThisClient() {
  if (clientSocket != -1) {
    close(clientSocket);
  }

  kill(clientPID, SIGKILL);
}

/* Read in next block of available bytes from client.  If there */
/* are no available bytes, block until there are.               */
int readBytesFromClient() {
  int errval, bytesRead;

  struct pollfd clientWait;
  clientWait.fd = clientSocket;
  clientWait.events = POLLIN;

  /* sleep until events */
  SYSCALL(errval = poll(&clientWait, 1, -1));
  if (errval <= 0) return errval;
    
  /* Attempt to read as many bytes as are available */
  SYSCALL(bytesRead = read(clientSocket, readBuffer, READ_BUFFER_SIZE));
  switch(bytesRead) {
  case 0:
    writeDebug("Client closed connection\n");
    endThisClient();
    break;

  case -1:
    writeDebug("Error when reading data from client - %s (%d)\n", strerror(errno), errno);
    return 0;
    break;

  default:
    writeDebug("Read %d bytes from client\n", bytesRead);
    break;
  }

  /* Set # of bytes available and last byte used */
  readBufferLength = bytesRead;
  readBufferPtr = 0;
  return bytesRead;
}

/* Read from client until we reach an EOL or an error.  If */
/* a line was read, return # of bytes (not including the   */
/* terminating null) in string.  If there is an error,     */
/* return -1.  bufferSize is the Max # of chars to write   */
/* into the passed client buffer                           */
int readStringFromClient(String clientBuffer, int bufferSize) {
  int charPtr = 0;

  for (;;) {
    /* See if we need to fetch more bytes from the client */
    if (readBufferPtr == readBufferLength) {
      if (readBytesFromClient() <= 0) return -1;
    }

    /* See if this byte should be copied */
    if (readBuffer[readBufferPtr] == '\015') {
      readBufferPtr++;
      continue;
    }

    /* See if this byte terminates the string */
    if (readBuffer[readBufferPtr] == '\012') {
      readBufferPtr++;
      clientBuffer[charPtr] = '\0';
      return charPtr;
    }

    /* Copy over the character */
    clientBuffer[charPtr++] = readBuffer[readBufferPtr++];
    if (charPtr == bufferSize) return -2;
  }

  /* Should never happen */
  return -3;
}

/* Writes the passed # of bytes to client.  if there is an */
/* error, FALSE is returned.                               */
Bool writeBytes(void *buffer, int byteCount) {
  int bytesWritten;

  /* Attempt to write passed data to client */
  SYSCALL(bytesWritten = write(clientSocket, buffer, byteCount));
  if (bytesWritten != byteCount) return FALSE;
  return TRUE;
}

/* Write the passed null-terminated string */
Bool writeText(String theFormat, ...) {
  int bytesToWrite;
  va_list theParms;

  /* Format the message */
  va_start(theParms, theFormat);
  bytesToWrite = vsnprintf(textBuffer, TEXT_BUFFER_SIZE, theFormat, theParms);
  va_end(theParms);

  /* See if there was an error */
  if (bytesToWrite <= 0) return FALSE;

  /* Format and print message */
  return writeBytes(textBuffer, bytesToWrite);
}

/* Send a response code/message to the client */
Bool sendResponse(String theResponseCode, String theResponse, ...) {
  int bytesToWrite;
  va_list theParms;
 
  /* Insure the response code is valid */
  if (theResponseCode == NULL) return FALSE;
  if (strlen(theResponseCode) != 3) return FALSE;

  /* Install the response code */
  strcpy(textBuffer, theResponseCode);
  strcat(textBuffer, " ");

  /* Format the message */
  va_start(theParms, theResponse);
  bytesToWrite = vsnprintf(&textBuffer[4], TEXT_BUFFER_SIZE - 6, theResponse, theParms);
  va_end(theParms);

  /* See if there was an error */
  if (bytesToWrite <= 0) return FALSE;

  /* Tack on standard line terminators */
  strcat(textBuffer, "\r\n");

  /* Format and print message */
  return writeBytes(textBuffer, bytesToWrite + 6);
}  

/* Write initial client welcome message */
Bool sendWelcome() {
  char hostName[256];

  /* Get host name */
  if (gethostname(hostName, 256) != 0) 
    strcpy(hostName, "unknown_host");

  /* Write header out */
  return sendResponse("000", "%s XMMS_INETCTL server (Version %s) ready.", hostName, INETCTLVERSION);
}

/* Read in client requests and dispatch as possible */
void processClientRequests(pid_t theClientPID, int theClientSocket) {
  int errval = 0;
  char commandBuffer[1024];
  int bytesRead;
  struct pollfd clientWait;

  /* Install specifics for this client */
  clientPID = theClientPID;
  clientSocket = theClientSocket;

  /* Setup for polling */
  clientWait.fd = clientSocket;
  clientWait.events = POLLIN;
  
  /* Welcome the client */
  if (!sendWelcome()) {
    writeError("Client appears to be dead - stopping client handler\n");
    endThisClient();
  }

  /* Send current status */
  if (!sendNewClientStatus(theClientSocket)) {
    writeError("Client appears to be dead - stopping client handler\n");
    endThisClient();
  }

  for(;;) {
    /* Wait for something to come in from the client */
    if ((errval = poll(&clientWait, 1, 300)) < 0) {
      writeDebug("Error polling client %s (%d) - terminating\n", strerror(errno), errno);
      break;
    }

    /* If this is a timeout, update stats on the clients connection */
    if (errval == 0) {
      updatePlayerStatus(clientSocket);
      continue;
    }

    /* Read a string from the client */
    if ((bytesRead = readStringFromClient(commandBuffer, 1024)) == -1) {
      writeError("Error trying to read from client - closing connection\n");
      return;
    }

    /* Just in case we are watching */
    writeDebug("Read command from client [%s]\n", commandBuffer);

    /* Attempt to dispatch this command */
    dispatchCommand(commandBuffer);
  }
}
