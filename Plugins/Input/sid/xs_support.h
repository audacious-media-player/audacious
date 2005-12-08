#ifndef XS_SUPPORT_H
#define XS_SUPPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xmms-sid.h"
#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif


/*
 * Functions
 */
guint16 xs_rd_be16(FILE *);
guint32 xs_rd_be32(FILE *);
size_t	xs_rd_str(FILE *, gchar *, size_t);
gchar	*xs_strncpy(gchar *, gchar *, size_t);
gint	xs_pstrcpy(gchar **, const gchar *);
gint	xs_pstrcat(gchar **, const gchar *);
void	xs_pnstrcat(gchar *, size_t, gchar *);
gchar	*xs_strrchr(gchar *, gchar);
inline 	void xs_findnext(gchar *, guint *);
inline 	void xs_findeol(gchar *, guint *);
inline	void xs_findnum(gchar *, guint *);

#ifdef HAVE_MEMSET
#define	xs_memset memset
#else
void	*xs_memset(void *, int, size_t);
#endif

#ifdef __cplusplus
}
#endif
#endif /* XS_SUPPORT_H */
