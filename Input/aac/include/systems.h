/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May		wmay@cisco.com
 */

#ifndef __SYSTEMS_H__
#define __SYSTEMS_H__

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>

#define OPEN_RDWR O_RDWR
#define OPEN_CREAT O_CREAT 
#define OPEN_RDONLY O_RDONLY

#define closesocket close
#define IOSBINARY ios::bin
#define MAX_UINT64 -1LLU
#define LLD "%lld"
#define LLU "%llu"
#define LLX "%llx"
#define M_LLU 1000LLU
#define C_LLU 100LLU
#define I_LLU 1LLU
#ifdef HAVE_FPOS_T_POS
#define FPOS_TO_VAR(fpos, typed, var) (var) = (typed)((fpos).__pos)
#define VAR_TO_FPOS(fpos, var) (fpos).__pos = (var)
#else
#define FPOS_TO_VAR(fpos, typed, var) (var) = (typed)(fpos)
#define VAR_TO_FPOS(fpos, var) (fpos) = (var)
#endif

#define FOPEN_READ_BINARY "r"
#define FOPEN_WRITE_BINARY "w"

#include <stdarg.h>
typedef void (*error_msg_func_t)(int loglevel,
				 const char *lib,
				 const char *fmt,
				 va_list ap);
typedef void (*lib_message_func_t)(int loglevel,
				   const char *lib,
				   const char *fmt,
				   ...);
#ifndef HAVE_IN_PORT_T
typedef uint16_t in_port_t;
#endif

#ifndef HAVE_SOCKLEN_T
typedef unsigned int socklen_t;
#endif

#ifdef sun
#include <limits.h>
#define u_int8_t uint8_t
#define u_int16_t uint16_t
#define u_int32_t uint32_t
#define u_int64_t uint64_t
#define __STRING(expr) #expr
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif

#define MALLOC_STRUCTURE(a) ((a *)malloc(sizeof(a)))

#define CHECK_AND_FREE(a) if ((a) != NULL) { free((a)); (a) = NULL;}

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif /* define unix */

