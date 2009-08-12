#ifndef TAGUTIL_H

#define TAGUTIL_H

#include <glib.h>
#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"

#define WMA_DEBUG 1

#if WMA_DEBUG
#define DEBUG(...) printf("TAG %25s:\t", __func__); printf(__VA_ARGS__);
#else
#define DEBUG(...) ;
#endif

time_t unix_time(guint64 win_time);

guint16 get_year(guint64 win_time);

void printTuple(Tuple *tuple);

//Tuple *makeTuple(Tuple *tuple, const gchar* title, const gchar* artist,
//        const gchar* comment, const gchar* album,
//        const gchar * genre, const gchar* year,
//        const gchar* filePath, int tracnr);


const gchar* get_complete_filepath(Tuple *tuple);

gchar *read_ASCII(VFSFile *fd, int size);

#endif		   