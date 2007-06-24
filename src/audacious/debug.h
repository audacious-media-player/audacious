/*
 * audacious: Cross-platform multimedia player.
 * debug.h: Debugging macros.
 *
 * Copyright (c) 2005-2007 Audacious development team.
 * Copyright (c) 2003-2005 BMP development team.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <glib.h>

#ifdef NDEBUG

/* void REQUIRE_LOCK(GMutex *m); */
#  define REQUIRE_LOCK(m)

/* void REQUIRE_STR_UTF8(const gchar *str); */
#  define REQUIRE_STR_UTF8(str)

/* void REQUIRE_STATIC_LOCK(GStaticMutex *m); */
#  define REQUIRE_STATIC_LOCK(m)

#else                           /* !NDEBUG */

/* void REQUIRE_LOCK(GMutex *m); */
#  define REQUIRE_LOCK(m) G_STMT_START { \
       if (g_mutex_trylock(m)) { \
           g_critical(G_STRLOC ": Mutex not locked!"); \
           g_mutex_unlock(m); \
       } \
   } G_STMT_END

/* void REQUIRE_STATIC_LOCK(GStaticMutex *m); */
#  define REQUIRE_STATIC_LOCK(m) G_STMT_START { \
       if (G_TRYLOCK(m)) { \
           g_critical(G_STRLOC ": Mutex not locked!"); \
           G_UNLOCK(m); \
       } \
   } G_STMT_END

/* void REQUIRE_STR_UTF8(const gchar *str); */
#  define REQUIRE_STR_UTF8(str) G_STMT_START { \
       if (!g_utf_validate(str, -1, NULL)) \
            g_warning(G_STRLOC ": String is not UTF-8!"); \
   } G_STMT_END

#endif                          /* NDEBUG */


#endif
