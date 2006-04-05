#ifndef _CUTILS_H
#define _CUTILS_H

int strstart(const char *str, const char *val, const char **ptr);
int stristart(const char *str, const char *val, const char **ptr);
void pstrcpy(char *buf, int buf_size, const char *str);
char *pstrcat(char *buf, int buf_size, const char *s);

time_t mktimegm(struct tm *tm);
const char *small_strptime(const char *p, const char *fmt,
                           struct tm *dt);

#endif

