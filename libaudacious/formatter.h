#ifndef XMMS_FORMATTER_H
#define XMMS_FORMATTER_H

#include <glib.h>

typedef struct {
    gchar *values[256];
} Formatter;


G_BEGIN_DECLS

Formatter *xmms_formatter_new(void);
void xmms_formatter_destroy(Formatter * formatter);
void xmms_formatter_associate(Formatter * formatter, guchar id,
                              gchar * value);
void xmms_formatter_dissociate(Formatter * formatter, guchar id);
gchar *xmms_formatter_format(Formatter * formatter, gchar * format);

G_END_DECLS

#endif
