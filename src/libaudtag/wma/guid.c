/*
 * Copyright 2009 Paula Stanciu
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

/* this stuff may be moved to ../util.h if needed by other formats */

#include <inttypes.h>

#include "libaudcore/vfs.h"
#include "../util.h"
#include "guid.h"
#include "wma_fmt.h"

GUID *guid_read_from_file(VFSFile * f)
{
    GUID temp;

    if ((f == NULL) || (vfs_fread(&temp, sizeof(GUID), 1, f) != 1))
        return NULL;

    temp.le32 = GUINT32_FROM_LE(temp.le32);
    temp.le16_1 = GUINT16_FROM_LE(temp.le16_1);
    temp.le16_2 = GUINT16_FROM_LE(temp.le16_2);
    temp.be64 = GUINT64_FROM_BE(temp.be64);

    return g_memdup(&temp, sizeof(GUID));
}

gboolean guid_write_to_file(VFSFile * f, int guid_type)
{
    g_return_val_if_fail(f != NULL, FALSE);

    GUID *g = guid_convert_from_string(wma_guid_map(guid_type));

    gboolean ret = write_LEuint32(f, g->le32) && write_LEuint16(f, g->le16_1) && write_LEuint16(f, g->le16_1) && write_LEuint64(f, g->be64);
    g_free(g);
    return ret;
}

GUID *guid_convert_from_string(const gchar * string)
{
    GUID temp;

    if (sscanf(string, "%" SCNx32 "-%" SCNx16 "-%" SCNx16 "-%" SCNx64, &temp.le32, &temp.le16_1, &temp.le16_2, &temp.be64) != 4)
        return NULL;
    return g_memdup(&temp, sizeof(GUID));
}

gchar *guid_convert_to_string(const GUID * g)
{

    return g_strdup_printf("%8x-%hx-%hx-%" PRIx64 "\n", GUINT32_TO_LE(g->le32), GUINT16_TO_LE(g->le16_1), GUINT16_TO_LE(g->le16_2), GUINT64_TO_BE(g->be64));
}

gboolean guid_equal(GUID * g1, GUID * g2)
{
    /*
       AUDDBG("GUID 1 = %8x-%hx-%hx-%"PRIx64"\n", g1->le32, g1->le16_1, g1->le16_2, g1->be64);
       AUDDBG("GUID 2 = %8x-%hx-%hx-%"PRIx64"\n", g2->le32, g2->le16_1, g2->le16_2, g2->be64);
     */

    g_return_val_if_fail((g1 != NULL) && (g2 != NULL), FALSE);
    if (!memcmp(g1, g2, 16))
    {
        //        AUDDBG("equal\n");

        return TRUE;
    }
    /* AUDDBG("not equal\n"); */
    return FALSE;
}

int get_guid_type(GUID * g)
{
    GUID *g1;
    int i;
    for (i = 0; i < ASF_OBJECT_LAST - 1; i++)
    {
        g1 = guid_convert_from_string(wma_guid_map(i));
        if (guid_equal(g, g1))
        {

            g_free(g1);
            return i;
        }
    }
    return -1;
}

const gchar *wma_guid_map(int i)
{
    const gchar *_guid_map[ASF_OBJECT_LAST] = {
        ASF_HEADER_OBJECT_GUID,
        ASF_FILE_PROPERTIES_OBJECT_GUID,
        ASF_STREAM_PROPERTIES_OBJECT_GUID,
        ASF_HEADER_EXTENSION_OBJECT_GUID,
        ASF_CODEC_LIST_OBJECT_GUID,
        ASF_SCRIPT_COMMAND_OBJECT_GUID,
        ASF_MARKER_OBJECT_GUID,
        ASF_BITRATE_MUTUAL_EXCLUSION_OBJECT_GUID,
        ASF_ERROR_CORRECTION_OBJECT_GUID,
        ASF_CONTENT_DESCRIPTION_OBJECT_GUID,
        ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT_GUID,
        ASF_CONTENT_BRANDING_OBJECT_GUID,
        ASF_STREAM_BITRATE_PROPERTIES_OBJECT_GUID,
        ASF_CONTENT_ENCRYPTION_OBJECT_GUID,
        ASF_EXTENDED_CONTENT_ENCRYPTION_OBJECT_GUID,
        ASF_DIGITAL_SIGNATURE_OBJECT_GUID,
        ASF_PADDING_OBJECT_GUID
    };
    return _guid_map[i];
}
