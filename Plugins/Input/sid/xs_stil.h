#ifndef XS_STIL_H
#define XS_STIL_H

#include "xmms-sid.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Defines and typedefs
 */
typedef struct {
	gchar	*pName,
		*pAuthor,
		*pInfo;
} t_xs_stil_subnode;

typedef struct _t_xs_stil_node {
	gchar			*pcFilename;
	t_xs_stil_subnode	subTunes[XS_STIL_MAXENTRY+1];
	struct _t_xs_stil_node	*pPrev, *pNext;
} t_xs_stil_node;


typedef struct {
	t_xs_stil_node	*pNodes,
			**ppIndex;
	gint		n;
} t_xs_stildb;


/*
 * Functions
 */
gint			xs_stildb_read(t_xs_stildb *, gchar *);
gint			xs_stildb_index(t_xs_stildb *);
void			xs_stildb_free(t_xs_stildb *);
t_xs_stil_node *	xs_stildb_get(t_xs_stildb *, gchar *, gchar *);


gint			xs_stil_init(void);
void			xs_stil_close(void);
t_xs_stil_node *	xs_stil_get(gchar *);

#ifdef __cplusplus
}
#endif
#endif /* XS_STIL_H */
