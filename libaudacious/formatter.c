#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <string.h>
#include "formatter.h"


Formatter *
xmms_formatter_new(void)
{
    Formatter *formatter = g_new0(Formatter, 1);

    xmms_formatter_associate(formatter, '%', "%");
    return formatter;
}

void
xmms_formatter_destroy(Formatter * formatter)
{
    int i;

    for (i = 0; i < 256; i++)
        if (formatter->values[i])
            g_free(formatter->values[i]);
    g_free(formatter);
}

void
xmms_formatter_associate(Formatter * formatter, guchar id, char *value)
{
    xmms_formatter_dissociate(formatter, id);
    formatter->values[id] = g_strdup(value);
}

void
xmms_formatter_dissociate(Formatter * formatter, guchar id)
{
    if (formatter->values[id])
        g_free(formatter->values[id]);
    formatter->values[id] = 0;
}

gchar *
xmms_formatter_format(Formatter * formatter, char *format)
{
    char *p, *q, *buffer;
    int len;

    for (p = format, len = 0; *p; p++)
        if (*p == '%') {
            if (formatter->values[(int) *++p])
                len += strlen(formatter->values[(int) *p]);
            else if (!*p) {
                len += 1;
                p--;
            }
            else
                len += 2;
        }
        else
            len++;
    buffer = g_malloc(len + 1);
    for (p = format, q = buffer; *p; p++)
        if (*p == '%') {
            if (formatter->values[(int) *++p]) {
                strcpy(q, formatter->values[(int) *p]);
                q += strlen(q);
            }
            else {
                *q++ = '%';
                if (*p != '\0')
                    *q++ = *p;
                else
                    p--;
            }
        }
        else
            *q++ = *p;
    *q = 0;
    return buffer;
}
