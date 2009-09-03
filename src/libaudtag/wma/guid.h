#ifndef _GUID_H
#define _GUID_H

#include <glib.h>
#include <libaudcore/vfs.h>

/* this stuff may be moved to ../util.h if needed by other formats */
typedef struct _guid {
    guint32 le32;
    guint16 le16_1;
    guint16 le16_2;
    guint64 be64;
}GUID;


GUID *guid_read_from_file(VFSFile *);
gboolean guid_write_to_file(VFSFile *, int);

GUID *guid_convert_from_string(const gchar *);
gchar *guid_convert_to_string(const GUID*);
gboolean guid_equal(GUID *, GUID *);
int get_guid_type(GUID *);
const gchar *wma_guid_map(int);

#endif /* _GUID_H */
