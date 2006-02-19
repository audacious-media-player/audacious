/* inetctl_command.c - Command parser and dispatcher */

/* Include commons */
#include "inetctl.h"

/* Private dynamic parse buffer */
String parseBuffer = NULL;
int parseBufferSize = 0;

/* Is the user authenticated? */
String userName = NULL;
Bool userAuthenticated = FALSE;

/* Private parameter store */
#define MAX_PARMS 64
String cmdParms[MAX_PARMS];
int cmdParmCount = -1;

/* upString will convert a string to upper case */
void upString(String target) {
  int charPtr = 0;

  /* Convert to upper case */
  while (target[charPtr] != '\0') {
    /* See if we should convert */
    if ((target[charPtr] >= 'a') && (target[charPtr] <= 'z'))
      target[charPtr] -= ('a' - 'A');
    charPtr++;
  }
}

/* trimString will trim leading/trailing spaces/tabs/new-lines */
/* from a passed string.                                       */
void trimString(String theText) {
  int srcPtr = 0, destPtr = 0;

  /* Skip empty strings */
  if (theText[0] == '\0') return;

  /* Strip trailing characters */
  destPtr = strlen(theText) - 1;
  while (isspace(theText[destPtr])) theText[destPtr--] = '\0';

  /* If the front part of string is okay, we're done */
  if (!isspace(theText[0])) return;

  /* Process each character */
  destPtr = 0;
  while (theText[srcPtr] != '\0') {
    /* See if we should skip this */
    if ((destPtr == 0) && isspace(theText[srcPtr]))
      srcPtr++;
    else
      theText[destPtr++] = theText[srcPtr++];
  }

  /* Set terminator in place */
  theText[destPtr] = '\0';
}

/* Abrev will return TRUE if the passed string is a valid abreviation */
/* of the passed string.  You pass the string to test as 'source', the*/
/* full command name as 'command' and the minimum # of chars that can */
/* be matches as 'minchars'.  If 'minchars' is passed as 0, then there*/
/* can be no abreviation.  The command must match exactly. If the     */
/* command is longer than the min # of chars, all chars after that    */
/* are checked as well.  We stop when we hit the end of the line or   */
/* the chars don't match.                                             */
Bool abrevCheck(String source, String command, int minchars) {
  int maxchar = strlen(source);
  int maxmatch = strlen(command);
  int charptr;

  /* Set the minimum # of chars if none specified */
  if (!minchars) minchars = maxmatch;

  /* Make sure we have at least as many characters as we need */
  /* and no more than we expected to handle                   */
  if ((maxchar < minchars) || (maxchar > maxmatch)) return FALSE;

  /* Test each character */
  for (charptr = 0; charptr < maxchar; charptr++)
    /* If we fail to match, exit in shame */
    if (source[charptr] != command[charptr]) return FALSE;

  /* All our chars matched */
  return TRUE;
}

/* ParseArgs will take a command and break it into a number of    */
/* arguments/tokens.  'command' is the command to process.  'argv'*/
/* is the returned token list array.  argc will be returned with  */
/* the # of args parsed.  'maxargs' is the max # of args that we  */
/* can deal with.  If there are too many arguments or quoting is  */
/* goofed up or the command line is screwed up in any way, FALSE  */
/* is returned.  Otherwise, TRUE.                                 */
Bool parseArgs(String command, String argv[], int *argc, int maxargs) {
  int  cmdLength = strlen(command);
  char quoteChar = '\0';
  Bool quoteMode = FALSE;
  Bool inParm = FALSE;
  Bool quoteNextChar = FALSE;
  String destChar;
  int  charPtr;

  /* Make sure we have enough room for a copy */
  if (parseBuffer == NULL) {
    parseBufferSize = cmdLength + 1;
    parseBuffer = (String) malloc(parseBufferSize);
    writeDebug("Allocated %d bytes for parse buffer @ %p\n", parseBufferSize, parseBuffer);
  } else if (cmdLength + 1 > parseBufferSize) {
    parseBufferSize = cmdLength + 1;
    parseBuffer = realloc(parseBuffer, parseBufferSize);
    writeDebug("Expanded parse buffer to %d bytes @ %p\n", parseBufferSize, parseBuffer);
  }

  /* Init for new parsing */
  *argc = 0;
  destChar = parseBuffer;

  /* Run through each char, until we hit EOL */
  for (charPtr = 0; command[charPtr] != '\0'; charPtr++) {
    /* See if we are dealing with a quoted character */
    if (!quoteNextChar) {
      /* Copy everything in quote mode (or notice we're done quoting) */
      if (quoteMode) {
	/* Check for End of Quote mode */
	if (command[charPtr] == quoteChar) {
	  *destChar++ = '\0';
	  quoteMode = FALSE;
	  inParm = FALSE;
	  continue;
	}

	/* Handle character quotes */
	if (command[charPtr] == '\\') {
	  quoteNextChar = TRUE;
	  continue;
	}

	/* Anything else is copied verbatim */
	*destChar++ = command[charPtr];
	continue;
      }
      
      /* See if we have a 'start of quote' flag */
      if (((command[charPtr] == '"') || (command[charPtr] == '\'')) && (!inParm)) {
	/* Set quote mode */
	quoteChar = command[charPtr];
	quoteMode = TRUE;

	/* Start a new parm */
	if (*argc == maxargs) return FALSE;
	argv[*argc] = destChar;
	(*argc)++;
	inParm = TRUE;
	
	continue;
      }
      
      /* Handle spaces as terminators and/or junk */
      if (isspace(command[charPtr])) {
	/* If we were in a parm, then end the parm here */
	if (inParm) {
	  *destChar++ = '\0';
	  inParm = FALSE;
	}

	continue;
      }
    }

    /* If we weren't in a parm on a regular char, start one */
    if (!inParm) {
      inParm = TRUE;

      /* Start a new parm */
      if (*argc == maxargs) return FALSE;
      argv[*argc] = destChar;
      (*argc)++;
    }
    
    /* Handle the character quoting character */
    if ((command[charPtr] == '\\' && !quoteNextChar)) {
      quoteNextChar = TRUE;
      continue;
    }

    /* Copy a character over */
    *destChar++ = command[charPtr];
    quoteNextChar = FALSE;
  }

  /* If we were in a parm, wrap things up */
  if (inParm) {
    *destChar++ = '\0';
    inParm = FALSE;
  }

  /* At EOL, make sure we are not in quote mode */
  return (!quoteMode);
}

/* Take the passed command and parse it.  Once parsed,     */
/* attempt to dispatch it to the appropriate handler.      */
/* If the command is processed correctly, TRUE is returned */
/* and if there is an error (command not recognized, error */
/* executing the command, etc).                            */
Bool dispatchCommand(String theCommand) {
  int commandIndex;

  /* Make sure we're not doomed */
  if (theCommand == NULL) return FALSE;

  /* Trim the string & exit on empty string or comment */
  trimString(theCommand);
  if (theCommand[0] == '\0') return TRUE;
  if (theCommand[0] == '#') return TRUE;
  writeDebug("Begining dispatch on command [%s]\n", theCommand);

  /* Parse the command into parts */
  if (!parseArgs(theCommand, cmdParms, &cmdParmCount, MAX_PARMS)) {
    sendResponse("002", "COMMAND FORMAT ERROR");
    return FALSE;
  }

  /* Make sure the command isn't missing */
  if (cmdParmCount == 0) {
    sendResponse("002", "MISSING COMMAND");
    return FALSE;
  }

  /* Upcase the command and make sure we have one */
  trimString(cmdParms[0]);
  upString(cmdParms[0]);
  if (cmdParms[0][0] == '\0') {
    sendResponse("002", "MISSING COMMAND");
    return FALSE;
  }

  /* Try to find the command */
  for (commandIndex = 0; commandTable[commandIndex].commandFunc != NULL; commandIndex++) {
    /* If command doesn't match, skip to next */
    if (strcmp(cmdParms[0], commandTable[commandIndex].commandName)) continue;

    /* If they are not authenticated and this command isn't always allowed, stop it */
    if (!userAuthenticated && !commandTable[commandIndex].alwaysAllowed) {
      writeDebug("Attempt to execute %d before authentication - rejected\n", cmdParms[0]);
      sendResponse("001", "NOT AUTHENTICATED");
      return FALSE;
    }

    /* Dispatch to this command */
    writeDebug("Matched command %s - dispatching\n", commandTable[commandIndex].commandName);
    if (commandTable[commandIndex].commandFunc(cmdParmCount, cmdParms)) {
      sendResponse("005", theCommand);
      return TRUE;
    } else
      return FALSE;
  }

  /* If we get here, there was no match */
  sendResponse("002", "UNKNOWN COMMAND");
  return FALSE;
}
