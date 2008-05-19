#ifndef AUDACIOUS_DEBUG_H
#define AUDACIOUS_DEBUG_H

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


#endif /* AUDACIOUS_DEBUG_H */
