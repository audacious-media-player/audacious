#ifndef _XS_SIDPLAY2_H
#define _XS_SIDPLAY2_H

#include "xmms-sid.h"

#ifdef __cplusplus
extern "C" {
#endif

gboolean	xs_sidplay2_isourfile(gchar *);
void		xs_sidplay2_close(t_xs_status *);
gboolean	xs_sidplay2_init(t_xs_status *);
gboolean	xs_sidplay2_initsong(t_xs_status *);
guint		xs_sidplay2_fillbuffer(t_xs_status *, gchar *, guint);
gboolean	xs_sidplay2_loadsid(t_xs_status *, gchar *);
void		xs_sidplay2_deletesid(t_xs_status *);
t_xs_tuneinfo*	xs_sidplay2_getsidinfo(gchar *);

#ifdef __cplusplus
}
#endif
#endif /* _XS_SIDPLAY2_H */
