#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"

/* defines functions for WMA file handling */
#ifndef AUDTAG_WMA_H
#define AUDTAG_WMA_H
// #define TEST 
// #ifdef TEST
// #define VFSFile FILE
// #define vfs_fopen(...) fopen(__VA_ARGS__)
// #define vfs_fread(...) fread(__VA_ARGS__)
// #define vfs_fwrite(...) fwrite(__VA_ARGS__)
// #define vfs_fseek(...) fseek(__VA_ARGS__)
// #define vfs_fclose(...) fclose(__VA_ARGS__)
// #define vfs_feof(...)   feof(__VA_ARGS__)
// #define vfs_ftell(...) feof(__VA_ARGS__)
// #endif

#define WMA_DEBUG

#ifdef WMA_DEBUG
#define DEBUG_TAG(...) printf(__VA_ARGS__)
#endif

typedef struct headerObj
{
    guint64 size;
    guint32 objectsNr;
    //8+8 byte reserved
}HeaderObject;


typedef struct contentField
{
	guint16 size;
	gunichar2 *strValue;
}ContentField;

gboolean wma_can_handle_file(const gchar *file_path);

Tuple *wma_populate_tuple_from_file(Tuple* tuple);

gboolean wma_write_tuple_to_file (Tuple* tuple);


#endif
