#ifndef XS_FILTER_H
#define XS_FILTER_H

#include "xmms-sid.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
typedef struct {
} t_xs_filter;

void	xs_filter_init(t_xs_filter *);
*/
gint	xs_filter_rateconv(void *, void *, const AFormat, const gint, const gint);

#ifdef __cplusplus
}
#endif
#endif /* XS_FILTER_H */
