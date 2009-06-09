#include "libaudcore/tuple.h"
#include "../tag_module.h"

#include "module.h"

#include "wma.h"

gboolean can_handle (Tuple *tuple)
{	
	const char *file_path = tuple_get_string(tuple, FIELD_FILE_PATH, NULL);
	return wma_can_handle_file(file_path);
}

Tuple *populate_tuple_from_file(Tuple* tuple)
{
	return NULL;
}

gboolean write_tuple_to_file (Tuple* tuple)
{
	return TRUE;
}

tag_module_t wma = {can_handle, populate_tuple_from_file, write_tuple_to_file };

