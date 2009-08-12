/*
 * Logging and debug mechanisms
 * Copyright (c) 2009 Audacious team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work.
 */
/**
 * @file log.h
 * @brief Informative logging API (aud_log(), aud_debug() and friends).
 * Functions for logfile handling, log contexts, logging levels, etc.
 * Also functions and macros for debug-level stuff.
 */

#ifndef AUD_LOG_CTX
#  define AUD_LOG_CTX NULL
#endif

#ifndef AUDACIOUS_LOG_H
#define AUDACIOUS_LOG_H

#include <glib.h>
#include <stdarg.h>

G_BEGIN_DECLS

/** Log levels from least noisy to noisiest */
typedef enum {
    AUD_LOG_NONE = 0,       /**< Pseudo log-level for suppressing most log messages */
    AUD_LOG_FATAL_ERROR,
    AUD_LOG_ERROR,
    AUD_LOG_WARNING,
    AUD_LOG_INFO,
    AUD_LOG_DEBUG,          /**< General debugging */
    AUD_LOG_DEBUG_INT,      /**< Intensive debugging (more details) */
    AUD_LOG_ALL             /**< Pseudo log-level for full logging */
} AudLogLevel;

gint aud_log_init(const gchar *filename, const gchar *mode, gint level);
void aud_log_close(void);

void aud_log_add_thread_context(GThread *thread, const gchar *name);
void aud_log_delete_thread_context(GThread *thread);

void aud_logv(const gchar *ctx, gint level, const gchar *fmt, va_list args) __attribute__ ((format(printf, 3, 0)));
void aud_log(const gchar *ctx, gint level, const gchar *fmt, ...)  __attribute__ ((format(printf, 3, 4)));
void aud_log_line(const gchar *ctx, gint level, const gchar *file, const gchar *func, gint line, const gchar *fmt, ...)  __attribute__ ((format(printf, 6, 7)));


/* These are here as a quick hack for transition from glib message system */
//#define GLIB_COMPAT

#ifdef GLIB_COMPAT
#undef g_message
#undef g_warning
#undef g_debug
#undef g_error
#undef g_critical
#define g_message(...) aud_log(AUD_LOG_CTX, AUD_LOG_INFO, __VA_ARGS__)
#define g_warning(...) aud_log(AUD_LOG_CTX, AUD_LOG_WARNING, __VA_ARGS__)
#define g_error(...) do { aud_log(AUD_LOG_CTX, AUD_LOG_ERROR, __VA_ARGS__); abort(); } while (0)
#define g_critical(...) do { aud_log(AUD_LOG_CTX, AUD_LOG_ERROR, __VA_ARGS__); abort(); } while (0)
#endif

//@{
/** Convenience wrapper message macros */
#if defined(DEBUG)
#  define aud_message(...) aud_log_line(AUD_LOG_CTX, AUD_LOG_INFO, __FILE__, __FUNCTION__, (gint) __LINE__, __VA_ARGS__)
#  define aud_warning(...) aud_log_line(AUD_LOG_CTX, AUD_LOG_WARNING, __FILE__, __FUNCTION__, (gint) __LINE__ , __VA_ARGS__)
#else
#  define aud_message(...) aud_log(AUD_LOG_CTX, AUD_LOG_INFO, __VA_ARGS__)
#  define aud_warning(...) aud_log(AUD_LOG_CTX, AUD_LOG_WARNING, __VA_ARGS__)
#endif
//@}

//@{
/** Debug message macro and transitional aliases */
#if defined(DEBUG)
#  define AUDDBG(...) aud_log_line(AUD_LOG_CTX, AUD_LOG_DEBUG, __FILE__, __FUNCTION__, (gint) __LINE__, __VA_ARGS__)
#  define aud_debug AUDDBG
#  ifdef GLIB_COMPAT
#    define g_debug AUDDBG
#  endif
#else
#  define AUDDBG(...)
#  define aud_debug(...)
#  ifdef GLIB_COMPAT
#    define g_debug
#  endif
#endif
//@}

//@{
/** Extra debug messages (more noisy, needs DEBUG > 1) */
#if defined(DEBUG) && (DEBUG > 1)
#  define AUDDBG_I(...) aud_log_line(AUD_LOG_CTX, AUD_LOG_DEBUG_INT, __FILE__, __FUNCTION__, (gint) __LINE__, __VA_ARGS__)
#else
#  define AUDDBG_I(...)
#endif
//@}

G_END_DECLS

#endif /* AUDACIOUS_LOG_H */
