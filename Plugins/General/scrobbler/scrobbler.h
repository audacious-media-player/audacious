#ifndef NET_H
#define NET_H 1

#include "libaudacious/titlestring.h"

int sc_idle(GMutex *);
void sc_init(char *, char *);
void sc_addentry(GMutex *, TitleInput *, int);
void sc_cleaner(void);
int sc_catch_error(void);
char *sc_fetch_error(void);
void sc_clear_error(void);
#endif
