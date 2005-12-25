#include "types.h"

typedef struct __PSFTAG
{
 char *key;
 char *value;
 struct __PSFTAG *next;
} PSFTAG;

typedef struct {
        u32 length;
        u32 stop;
        u32 fade;
        char *title,*artist,*game,*year,*genre,*psfby,*comment,*copyright;
        PSFTAG *tags;
} PSFINFO;

int sexypsf_seek(u32 t);
void sexypsf_stop(void);
void sexypsf_execute(void);

PSFINFO *sexypsf_load(char *path);
PSFINFO *sexypsf_getpsfinfo(char *path);
void sexypsf_freepsfinfo(PSFINFO *info);

void sexypsf_update(unsigned char*,long);
