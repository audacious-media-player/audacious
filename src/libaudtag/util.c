#include <glib.h>
#include "util.h"
#include <inttypes.h>

/* convert windows time to unix time */
time_t unix_time(guint64 win_time)
{
    guint64 t = (guint64) ((win_time / 10000000LL) - 11644473600LL);
    return (time_t) t;
}

guint16 get_year(guint64 win_time)
{
    GDate* d = g_date_new();
    g_date_set_time_t(d, unix_time(win_time));
    guint16 year = g_date_get_year(d);
    g_date_free(d);
    return year;
}

Tuple *makeTuple(Tuple *tuple, const gchar* title, const gchar* artist,
                 const gchar* comment, const gchar* album,
                 const gchar * genre, const gchar* year,
                 const gchar* filePath, int tracnr)
{

    tuple_associate_string(tuple, FIELD_ARTIST, NULL, artist);
    tuple_associate_string(tuple, FIELD_TITLE, NULL, title);
    tuple_associate_string(tuple, FIELD_COMMENT, NULL, comment);
    tuple_associate_string(tuple, FIELD_ALBUM, NULL, album);
    tuple_associate_string(tuple, FIELD_GENRE, NULL, genre);
    tuple_associate_string(tuple, FIELD_YEAR, NULL, year);
    tuple_associate_int(tuple, FIELD_TRACK_NUMBER, NULL, tracnr);
    tuple_associate_string(tuple, FIELD_FILE_PATH, NULL, filePath);
    return tuple;
}

const gchar* get_complete_filepath(Tuple *tuple)
{
    const gchar* filepath;
    const gchar* dir;
    const gchar* file;

    dir = tuple_get_string(tuple, FIELD_FILE_PATH, NULL);
    file = tuple_get_string(tuple, FIELD_FILE_NAME, NULL);
    filepath = g_strdup_printf("%s/%s", dir, file);
    AUDDBG("file path = %s\n", filepath);
    return filepath;
}

void print_tuple(Tuple *tuple)
{
#if WMA_DEBUG
    AUDDBG("--------------TUPLE PRINT --------------------\n");
    const gchar* title = tuple_get_string(tuple, FIELD_TITLE, NULL);
    AUDDBG("title = %s\n", title);
    /* artist */
    const gchar* artist = tuple_get_string(tuple, FIELD_ARTIST, NULL);
    AUDDBG("artist = %s\n", artist);

    /* copyright */
    const gchar* copyright = tuple_get_string(tuple, FIELD_COPYRIGHT, NULL);
    AUDDBG("copyright = %s\n", copyright);

    /* comment / description */

    const gchar* comment = tuple_get_string(tuple, FIELD_COMMENT, NULL);
    AUDDBG("comment = %s\n", comment);

    /* codec name */
    const gchar* codec_name = tuple_get_string(tuple, FIELD_CODEC, NULL);
    AUDDBG("codec = %s\n", codec_name);

    /* album */
    const gchar* album = tuple_get_string(tuple, FIELD_ALBUM, NULL);
    AUDDBG("Album = %s\n", album);

    /*track number */
    gint track_nr = tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL);
    AUDDBG("Track nr = %d\n", track_nr);

    /* genre */
    const gchar* genre = tuple_get_string(tuple, FIELD_GENRE, NULL);
    AUDDBG("Genre = %s \n", genre);

    /* length */
    gint length = tuple_get_int(tuple, FIELD_LENGTH, NULL);
    AUDDBG("Length = %d\n", length);

    /* year */
    gint year = tuple_get_int(tuple, FIELD_YEAR, NULL);
    AUDDBG("Year = %d\n", year);

    /* quality */
    const gchar* quality = tuple_get_string(tuple, FIELD_QUALITY, NULL);
    AUDDBG("quality = %s\n", quality);

    /* path */
    const gchar* path = tuple_get_string(tuple, FIELD_FILE_PATH, NULL);
    AUDDBG("path = %s\n", path);

    /* filename */
    const gchar* filename = tuple_get_string(tuple, FIELD_FILE_NAME, NULL);
    AUDDBG("filename = %s\n", filename);

    AUDDBG("-----------------END---------------------\n");
#endif
}

void seek(VFSFile *f, long pos)
{

    vfs_fseek(f, pos, SEEK_SET);
}

void skip(VFSFile *f, int amount)
{
    vfs_fseek(f, amount, SEEK_CUR);
}

gchar *read_char_data(VFSFile *fd, int size)
{
    gchar *value = g_new0(gchar, size);
    vfs_fread(value, size, 1, fd);
    return value;
}

gboolean write_char_data(VFSFile *f, gchar *data, size_t i)
{
   return (vfs_fwrite(data, i, 1, f) == i);
}
gchar* utf8(gunichar2* s)
{
    g_return_val_if_fail(s != NULL, NULL);
    return g_utf16_to_utf8(s, -1, NULL, NULL, NULL);
}

gunichar2 *fread_utf16(VFSFile* f, guint64 size)
{
    gunichar2 *p = (gunichar2 *) g_malloc0(size);
    if (vfs_fread(p, 1, size, f) != size)
    {
        g_free(p);
        p = NULL;
    }
    gchar * s = utf8(p);
    AUDDBG("Converted to UTF8: '%s'\n", s);
    g_free(s);
    return p;
}

gboolean write_utf16(VFSFile *f, gunichar2 *data, size_t i)
{
  return (vfs_fwrite(data, i, 1, f) == i);
}

guint8 read_uint8(VFSFile *fd)
{
    guint16 i;
    if (vfs_fread(&i, 1, 1, fd) == 1)
    {
        return i;
    }
    return -1;
}

guint16 read_LEuint16(VFSFile *fd)
{
    guint16 a;
    if (vfs_fget_le16(&a, fd))
    {
        AUDDBG("read_LEuint16 %d\n", a);
        return a;
    } else
        return -1;
}

guint16 read_BEuint16(VFSFile *fd)
{
    guint16 a;
    if (vfs_fget_be16(&a, fd))
    {
        AUDDBG("read_BEuint16 %d\n", a);
        return a;
    } else
        return -1;
}

guint32 read_LEuint32(VFSFile *fd)
{
    guint32 a;
    if (vfs_fget_le32(&a, fd))
    {
        AUDDBG("read_LEuint32 %d\n", a);
        return a;
    } else
        return -1;
}

guint32 read_BEuint32(VFSFile *fd)
{
    guint32 a;
    if (vfs_fget_be32(&a, fd))
    {
        AUDDBG("read_BEuint32 %d\n", a);
        return a;
    } else
        return -1;
}

guint64 read_LEuint64(VFSFile *fd)
{
    guint64 a;
    if (vfs_fget_le64(&a, fd))
    {
        AUDDBG("read_LEuint64 %"PRId64"\n", a);
        return a;
    } else
        return -1;
}

guint64 read_BEuint64(VFSFile *fd)
{
    guint64 a;
    if (vfs_fget_be64(&a, fd))
    {
        AUDDBG("read_BEuint14 %"PRId64"\n", a);
        return a;
    } else
        return 1;
}

gboolean write_uint8(VFSFile *fd, guint8 val)
{
    return (vfs_fwrite(&val, 1, 1, fd) == 1);
}

gboolean write_LEuint16(VFSFile *fd, guint16 val)
{
    guint16 le_val = GUINT32_TO_LE(val);
    return (vfs_fwrite(&le_val, 2, 1, fd) == 2);
}

gboolean write_BEuint32(VFSFile *fd, guint32 val)
{
    guint32 be_val = GUINT32_TO_BE(val);
    return (vfs_fwrite(&be_val, 4, 1, fd) == 4);
}

gboolean write_LEuint32(VFSFile *fd, guint32 val)
{
    guint32 le_val = GUINT32_TO_LE(val);
    return (vfs_fwrite(&le_val, 4, 1, fd) == 4);
}

gboolean write_LEuint64(VFSFile *fd, guint64 val)
{
    guint64 le_val = GUINT64_TO_LE(val);
    return (vfs_fwrite(&le_val, 8, 1, fd) == 8);
}

void copyAudioToFile(VFSFile *from, VFSFile *to, guint32 pos)
{
    vfs_fseek(from, pos, SEEK_SET);
    while (vfs_feof(from) == 0)
    {
        gchar buf[4096];
        gint n = vfs_fread(buf, 1, 4096, from);
        vfs_fwrite(buf, n, 1, to);
    }
}

void copyAudioData(VFSFile* from, VFSFile *to, guint32 pos_from, guint32 pos_to)
{
    vfs_fseek(from, pos_from, SEEK_SET);
    int bytes_read = pos_from;
    while (bytes_read < pos_to - 4096)
    {
        gchar buf[4096];
        guint32 n = vfs_fread(buf, 1, 4096, from);
        vfs_fwrite(buf, n, 1, to);
        bytes_read += n;
    }
    if (bytes_read < pos_to)
    {
        guint32 buf_size = pos_to - bytes_read;
        gchar buf2[buf_size];
        int nn = vfs_fread(buf2, 1, buf_size, from);
        vfs_fwrite(buf2, nn, 1, to);
    }
}
