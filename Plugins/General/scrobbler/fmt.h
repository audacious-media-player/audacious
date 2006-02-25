#ifndef FMT_H
#define FMT_H 1
#define pdebug(s, b) if(b) { fmt_debug(__FILE__, __FUNCTION__, (s)); }

#include <time.h>

char *fmt_escape(char *);
char *fmt_unescape(char *);
char *fmt_timestr(time_t, int);
char *fmt_vastr(char *, ...);
void fmt_debug(char *, const char *, char *);
char *fmt_string_pack(char *, char *, ...);
int fmt_strcasecmp(const char *s1, const char *s2);
int fmt_strncasecmp(const char *s1, const char *s2, size_t n);
#endif
