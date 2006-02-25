#ifndef NET_H
#define NET_H 1

int sc_idle(GMutex *);
void sc_init(char *, char *);
void sc_addentry(GMutex *, metatag_t *, int);
void sc_cleaner(void);
int sc_catch_error(void);
char *sc_fetch_error(void);
void sc_clear_error(void);
#endif
