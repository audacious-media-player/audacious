#ifndef GUID_H

#define GUID_H

/* operations on GUIDs */
typedef struct _guid
{
	guint32 le32;
	guint16 le16_1;
	guint16 le16_2;
	guint64 be64;
}GUID;


GUID *guid_read_from_file(const gchar* file_path, int offset);
GUID *guid_convert_from_string(const gchar *s);
gboolean guid_equal(GUID *g1, GUID *g2);
int get_guid_type(GUID *g);

#endif