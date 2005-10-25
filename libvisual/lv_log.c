/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <signal.h>

#include "lv_common.h"
#include "lv_error.h"
#include "lv_log.h"


static VisLogVerboseness verboseness = VISUAL_LOG_VERBOSENESS_MEDIUM;

static void default_info_handler (const char *msg, const char *funcname, void *privdata);
static void default_warning_handler (const char *msg, const char *funcname, void *privdata);
static void default_critical_handler (const char *msg, const char *funcname, void *privdata);
static void default_error_handler (const char *msg, const char *funcname, void *privdata);

static struct _message_handlers {
	VisLogMessageHandlerFunc	 info_handler;
	VisLogMessageHandlerFunc	 warning_handler;
	VisLogMessageHandlerFunc	 critical_handler;
	VisLogMessageHandlerFunc	 error_handler;

	void				*info_priv;
	void				*warning_priv;
	void				*critical_priv;
	void				*error_priv;
} message_handlers;


/**
 * @defgroup VisLog VisLog
 * @{
 */

/**
 * Set the library it's verbosity level.
 *
 * @param v The verbose level as a VisLogVerboseness enumerate value.
 */
void visual_log_set_verboseness (VisLogVerboseness v)
{
	verboseness = v;
}

/**
 * Get the current library it's verbosity level.
 *
 * @return The verboseness level as a VisLogVerboseness enumerate value.
 */
VisLogVerboseness visual_log_get_verboseness ()
{
	return verboseness;
}

/**
 * Set the callback function that handles info messages.
 *
 * @param handler The custom message handler callback.
 * @param priv Optional private data to pass on to the handler.
 */
void visual_log_set_info_handler (VisLogMessageHandlerFunc handler, void *priv)
{
	visual_log_return_if_fail (handler != NULL);

	message_handlers.info_handler = handler;

	message_handlers.info_priv = priv;
}

/**
 * Set the callback function that handles warning messages.
 *
 * @param handler The custom message handler callback.
 * @param priv Optional private data to pass on to the handler.
 */
void visual_log_set_warning_handler (VisLogMessageHandlerFunc handler, void *priv)
{
	visual_log_return_if_fail (handler != NULL);

	message_handlers.warning_handler = handler;

	message_handlers.warning_priv = priv;
}

/**
 * Set the callback function that handles critical messages.
 *
 * @param handler The custom message handler callback.
 * @param priv Optional private data to pass on to the handler.
 */
void visual_log_set_critical_handler (VisLogMessageHandlerFunc handler, void *priv)
{
	visual_log_return_if_fail (handler != NULL);

	message_handlers.critical_handler = handler;

	message_handlers.critical_priv = priv;
}

/**
 * Set the callback function that handles error messages. After handling the message with
 * this function, libvisual will abort the program. This behavior cannot be
 * changed.
 *
 * @param handler The custom message handler callback.
 * @param priv Optional private data to pass on to the handler.
 */
void visual_log_set_error_handler (VisLogMessageHandlerFunc handler, void *priv)
{
	visual_log_return_if_fail (handler != NULL);

	message_handlers.error_handler = handler;

	message_handlers.error_priv = priv;
}

/**
 * Set callback the function that handles all the messages.
 *
 * @param handler The custom message handler callback.
 * @param priv Optional private data to pass on to the handler.
 */
void visual_log_set_all_messages_handler (VisLogMessageHandlerFunc handler, void *priv)
{
	visual_log_return_if_fail (handler != NULL);

	message_handlers.info_handler = handler;
	message_handlers.warning_handler = handler;
	message_handlers.critical_handler = handler;
	message_handlers.error_handler = handler;

	message_handlers.info_priv = priv;
	message_handlers.warning_priv = priv;
	message_handlers.critical_priv = priv;
	message_handlers.error_priv = priv;
}
	
/**
 * Private library call used by the visual_log define to display debug,
 * warning and error messages.
 *
 * @see visual_log
 * 
 * @param severity Severity of the log message.
 * @param file Char pointer to a string that contains the source filename.
 * @param line Line number for which the log message is.
 * @param funcname Function name in which the log message is called.
 * @param fmt Format string to display the log message.
 */
void _lv_log (VisLogSeverity severity, const char *file,
			int line, const char *funcname, const char *fmt, ...)
{
	char str[1024];
	va_list va;
	
	assert (fmt != NULL);

	va_start (va, fmt);
	vsnprintf (str, 1023, fmt, va);
	va_end (va);

	switch (severity) {
		case VISUAL_LOG_DEBUG:
			if (verboseness == VISUAL_LOG_VERBOSENESS_HIGH)
				fprintf (stderr, "libvisual DEBUG: %s: %s() [(%s,%d)]: %s\n",
						__lv_progname, funcname, file, line, str);
		
			break;
		case VISUAL_LOG_INFO:
			if (!message_handlers.info_handler)
				visual_log_set_info_handler (default_info_handler, NULL);

			if (verboseness >= VISUAL_LOG_VERBOSENESS_MEDIUM)
				message_handlers.info_handler (str, funcname, message_handlers.info_priv);

			break;
		case VISUAL_LOG_WARNING:
			if (!message_handlers.warning_handler)
				visual_log_set_warning_handler (default_warning_handler, NULL);

			if (verboseness >= VISUAL_LOG_VERBOSENESS_MEDIUM)
				message_handlers.warning_handler (str, funcname, message_handlers.warning_priv);
			
			break;

		case VISUAL_LOG_CRITICAL:
			if (!message_handlers.critical_handler)
				visual_log_set_critical_handler (default_critical_handler, NULL);

			if (verboseness >= VISUAL_LOG_VERBOSENESS_LOW)
				message_handlers.critical_handler (str, funcname, message_handlers.critical_priv);
		
			break;

		case VISUAL_LOG_ERROR:
			if (!message_handlers.error_handler)
				visual_log_set_error_handler (default_error_handler, NULL);

			if (verboseness >= VISUAL_LOG_VERBOSENESS_LOW)
				message_handlers.error_handler (str, funcname, message_handlers.error_priv);

			visual_error_raise ();

			break;
	}
}

/**
 * @}
 */

static void default_info_handler (const char *msg, const char *funcname, void *privdata)
{
	printf ("libvisual INFO: %s: %s\n", __lv_progname, msg);
}

static void default_warning_handler (const char *msg, const char *funcname, void *privdata)
{
	if (funcname)
		fprintf (stderr, "libvisual WARNING: %s: %s(): %s\n",
				__lv_progname, funcname, msg);
	else
		fprintf (stderr, "libvisual WARNING: %s: %s\n", __lv_progname, msg);
}

static void default_critical_handler (const char *msg, const char *funcname, void *privdata)
{
	if (funcname)
		fprintf (stderr, "libvisual CRITICAL: %s: %s(): %s\n",
				__lv_progname, funcname, msg);
	else
		fprintf (stderr, "libvisual CRITICAL: %s: %s\n", __lv_progname, msg);
}

static void default_error_handler (const char *msg, const char *funcname, void *privdata)
{
	if (funcname)
		fprintf (stderr, "libvisual ERROR: %s: %s(): %s\n",
				__lv_progname, funcname, msg);
	else
		fprintf (stderr, "libvisual ERROR: %s: %s\n", __lv_progname, msg);
}

