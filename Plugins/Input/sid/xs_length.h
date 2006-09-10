#ifndef _XS_LENGTH_H
#define _XS_LENGTH_H

#include "xmms-sid.h"
#include "xs_md5.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Defines and typedefs
 */
typedef struct _t_xs_sldb_node {
	t_xs_md5hash	md5Hash;	/* 128-bit MD5 hash-digest */
	gint		nLengths;	/* Number of lengths */
	gint		*sLengths;	/* Lengths in seconds */
	struct _t_xs_sldb_node *pPrev, *pNext;
} t_xs_sldb_node;


typedef struct {
	t_xs_sldb_node	*pNodes,
			**ppIndex;
	gint		n;
} t_xs_sldb;


/*
 * Functions
 */
gint			xs_sldb_read(t_xs_sldb *, const gchar *);
gint			xs_sldb_index(t_xs_sldb *);
void			xs_sldb_free(t_xs_sldb *);
t_xs_sldb_node *	xs_sldb_get(t_xs_sldb *, const gchar *);


gint			xs_songlen_init(void);
void			xs_songlen_close(void);
t_xs_sldb_node *	xs_songlen_get(const gchar *);

#ifdef __cplusplus
}
#endif
#endif /* _XS_LENGTH_H */
