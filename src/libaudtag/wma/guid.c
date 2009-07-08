#include <string.h>
#include <glib-2.0/glib.h>

#include "libaudcore/vfs.h"
#include "guid.h"
#include "wma_fmt.h"
#include "../util.h"

GStaticRWLock file_lock = G_STATIC_RW_LOCK_INIT;
void writeGuidToFile(VFSFile *f,int guid_type);


GUID *guid_read_from_file(const gchar* file_path, int offset)
{
/*    printf("offset = %d\n",offset);   */
	VFSFile *f;
	gchar buf[16];
	GUID *g = g_new0(GUID, 1);

	g_static_rw_lock_reader_lock(&file_lock);
	f = vfs_fopen(file_path, "r");
    if(f == NULL)
        DEBUG_TAG("fopen error\n");
    else
        DEBUG_TAG("fopen ok\n");

    vfs_fseek(f,offset,SEEK_SET);
    vfs_fread(buf,16,1,f);
    g = (GUID*)buf;
    g->be64 =  GUINT64_SWAP_LE_BE(g->be64);
    return g;
}

GUID *guid_convert_from_string(const gchar* s)
{
    GUID * gg = g_malloc (sizeof * gg);

    if (sscanf (s, "%8x-%hx-%hx-%llx", & gg->le32, & gg->le16_1, & gg->le16_2,
     & gg->be64) != 4)
    {
        g_free (gg);
        return NULL;
    }

    return gg;
}

gboolean guid_equal(GUID *g1, GUID *g2)
{
	g_return_val_if_fail((g1 != NULL)&&(g2 != NULL), FALSE);
	if (!memcmp(g1, g2, 16))
		return TRUE;
	return FALSE;
}

int get_guid_type(GUID *g)
{
	GUID *g1;
	int i;
	for( i=0; i < ASF_OBJECT_LAST-1; i++)
	{
		g1 = guid_convert_from_string(object_types_map[i].guid_value);
		if(guid_equal(g, g1))
		{
			g_free(g1);
			return i;
		}
	}
	return -1;
}

void writeGuidToFile(VFSFile * f,int guid_type)
{

	gchar * strGuid = object_types_map[guid_type].guid_value;
	GUID *g  = g_new(GUID,1);
	memcpy(g,guid_convert_from_string(strGuid),sizeof(GUID));
	g->be64 =  GUINT64_SWAP_LE_BE(g->be64);
	vfs_fwrite(&(g->le32),4,1,f);
	vfs_fwrite(&(g->le16_1),2,1,f);
	vfs_fwrite(&(g->le16_2),2,1,f);
	vfs_fwrite(&(g->be64),8,1,f);
}
