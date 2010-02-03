#ifndef TAGUTIL_H

#define TAGUTIL_H

#include <glib.h>
#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"

#define WMA_DEBUG 1

#define BROKEN 1

time_t unix_time(guint64 win_time);

guint16 get_year(guint64 win_time);

void print_tuple(Tuple *tuple);

//Tuple *makeTuple(Tuple *tuple, const gchar* title, const gchar* artist,
//        const gchar* comment, const gchar* album,
//        const gchar * genre, const gchar* year,
//        const gchar* filePath, int tracnr);

gchar *utf8(gunichar2* s);
const gchar* get_complete_filepath(Tuple *tuple);

gchar *read_char_data(VFSFile *fd, int size);
gboolean write_char_data(VFSFile *f, gchar *data, size_t i);

gunichar2 *fread_utf16(VFSFile* f, guint64 size);
gboolean write_utf16(VFSFile *f, gunichar2 *data, size_t i);

guint8 read_uint8(VFSFile *fd);
guint16 read_LEuint16(VFSFile *fd);
guint16 read_BEuint16(VFSFile *fd);
guint32 read_LEuint32(VFSFile *fd);
guint32 read_BEuint32(VFSFile *fd);
guint64 read_LEuint64(VFSFile *fd);
guint64 read_BEuint64(VFSFile *fd);


gboolean write_uint8(VFSFile *fd, guint8 val);
gboolean write_BEuint16(VFSFile *fd, guint16 val);
gboolean write_LEuint16(VFSFile *fd, guint16 val);
gboolean write_BEuint32(VFSFile *fd, guint32 val);
gboolean write_LEuint32(VFSFile *fd, guint32 val);
gboolean write_BEuint64(VFSFile *fd, guint64 val);
gboolean write_LEuint64(VFSFile *fd, guint64 val);

guint64 read_LEint64(VFSFile *fd);
void copyAudioToFile(VFSFile *from, VFSFile *to, guint32 pos);
void copyAudioData(VFSFile* from, VFSFile *to, guint32 pos_from, guint32 pos_to);

/* macro for debug print */
#ifdef WMA_DEBUG
#  define AUDDBG(...) do { g_print("%s:%d %s(): ", __FILE__, (int)__LINE__, __FUNCTION__); g_print(__VA_ARGS__); } while (0)
#else
#  define AUDDBG(...) do { } while (0)
#endif

#endif
