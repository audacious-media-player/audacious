/* functions for WMA file handling */

#include <glib.h>
#include "wma.h"
#include "guid.h"
#include "wma_fmt.h"

gboolean wma_can_handle_file(const char *file_path)
{	
	int retval = FALSE;
	GUID *guid1 = guid_read_from_file(file_path);
	GUID *guid2 = guid_convert_from_string(ASF_HEADER_OBJECT_GUID);
	if(guid_equal(guid1, guid2))
		retval = TRUE;
	g_free(guid1);
	g_free(guid2);
	return retval;
}

