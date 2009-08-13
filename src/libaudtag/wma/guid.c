#include <string.h>
#include <glib-2.0/glib.h>

#include "libaudcore/vfs.h"
#include "guid.h"
#include "wma_fmt.h"
#include "../util.h"
#include <inttypes.h>
GStaticRWLock file_lock = G_STATIC_RW_LOCK_INIT;

GUID * guid_read_from_file (VFSFile * handle)
{
    GUID temp;

    if (vfs_fread (& temp, sizeof (GUID), 1, handle) != 1)
        return NULL;

    temp.be64 = GUINT64_SWAP_LE_BE (temp.be64);
    return g_memdup (& temp, sizeof (GUID));
}

GUID * guid_convert_from_string (const gchar * string)
{
    GUID temp;

    if (sscanf (string, "%" SCNx32 "-%" SCNx16 "-%" SCNx16 "-%" SCNx64,
     & temp.le32, & temp.le16_1, & temp.le16_2, & temp.be64) != 4)
        return NULL;

    return g_memdup (& temp, sizeof (GUID));
}

gboolean guid_equal(GUID *g1, GUID *g2) {
/*

    DEBUG("GUID 1 = %8x-%hx-%hx-%"PRIx64"\n", g1->le32, g1->le16_1, g1->le16_2, g1->be64);
    DEBUG("GUID 2 = %8x-%hx-%hx-%"PRIx64"\n", g2->le32, g2->le16_1, g2->le16_2, g2->be64);
*/

    g_return_val_if_fail((g1 != NULL) && (g2 != NULL), FALSE);
    if (!memcmp(g1, g2, 16)) {
//        DEBUG("equal\n");
        return TRUE;
    }
//    DEBUG("not equal\n");
    return FALSE;
}

int get_guid_type(GUID *g) {
    GUID *g1;
    int i;
    for (i = 0; i < ASF_OBJECT_LAST - 1; i++) {
        g1 = guid_convert_from_string(object_types_map[i].guid_value);
        if (guid_equal(g, g1)) {
            g_free(g1);
            return i;
        }
    }
    return -1;
}

void writeGuidToFile(VFSFile * f, int guid_type) {

    gchar * strGuid = object_types_map[guid_type].guid_value;
    GUID * g = guid_convert_from_string (strGuid);

    g->be64 = GUINT64_SWAP_LE_BE(g->be64);
    vfs_fwrite(&(g->le32), 4, 1, f);
    vfs_fwrite(&(g->le16_1), 2, 1, f);
    vfs_fwrite(&(g->le16_2), 2, 1, f);
    vfs_fwrite(&(g->be64), 8, 1, f);

    g_free (g);
}
