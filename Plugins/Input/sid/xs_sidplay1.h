#ifndef _XS_SIDPLAY1_H
#define _XS_SIDPLAY1_H

#include "xmms-sid.h"

/* Maximum audio frequency supported by libSIDPlay v1 */
#define SIDPLAY1_MAX_FREQ	(48000)

#ifdef __cplusplus
extern "C" {
#endif

gboolean	xs_sidplay1_isourfile(gchar *);
void		xs_sidplay1_close(t_xs_status *);
gboolean	xs_sidplay1_init(t_xs_status *);
gboolean	xs_sidplay1_initsong(t_xs_status *);
guint		xs_sidplay1_fillbuffer(t_xs_status *, gchar *, guint);
gboolean	xs_sidplay1_loadsid(t_xs_status *, gchar *);
void		xs_sidplay1_deletesid(t_xs_status *);
t_xs_tuneinfo*	xs_sidplay1_getsidinfo(gchar *);

#ifdef __cplusplus
}
#endif
#endif /* _XS_SIDPLAY1_H */
