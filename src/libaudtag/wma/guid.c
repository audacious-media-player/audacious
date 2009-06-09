#include <string.h>
#include <glib.h>

#include "libaudcore/vfs.h"
#include "guid.h"
#include "wma.h"
#include "wma_fmt.h"
#include "wma_fmt.h"

GStaticRWLock file_lock = G_STATIC_RW_LOCK_INIT;

GUID *guid_read_from_file(const gchar* file_path)
{
	VFSFile *f;
	gchar buf[16];
	GUID *g = g_new0(GUID, 1);

	g_static_rw_lock_reader_lock(&file_lock);
	f = vfs_fopen(file_path, "r");
	vfs_fread(buf, 16, 1, f);

	/* everything is converted to host endianness */
	g->le32 = GUINT32_FROM_LE(buf[0]);
	g->le16_1 = GUINT16_FROM_LE(buf[4]);
	g->le16_2 = GUINT16_FROM_LE(buf[6]);
	g->be64 = GUINT64_FROM_BE(buf[8]);
	return g;
}

GUID *guid_convert_from_string(const gchar *s)
{
	/* make sure the string is in the right format */
	GRegex *regex = 
		g_regex_new("[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{16}",
			    0, 0, NULL);

	if(!g_regex_match(regex, s, 0, NULL))
	{
		g_regex_unref(regex);
		return NULL;
	}

	GUID *g = g_new0(GUID, 1);
	/* obviously, the data is converted to host order */
	sscanf ( s,"%8x-%hx-%hx-%llx\n", &g->le32, &g->le16_1, &g->le16_2, &g->be64);
	return g;
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
	for( i=0; i < ASF_OBJECT_LAST-1; i++);
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
