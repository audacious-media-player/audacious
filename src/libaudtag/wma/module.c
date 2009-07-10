#include "libaudcore/tuple.h"
#include "../tag_module.h"
#include "../util.h"
#include "module.h"
#include "wma.h"

gboolean can_handle (Tuple *tuple)
{	
	char *file_path =get_complete_filepath(tuple);
	return wma_can_handle_file(file_path);
}

Tuple *populate_tuple_from_file(Tuple *tuple)
{
    printf("populate tuple from file - return");
    return wma_populate_tuple_from_file(tuple);
}

gboolean write_tuple_to_file (Tuple* tuple)
{
	return TRUE;
}

tag_module_t wma = {can_handle, populate_tuple_from_file, write_tuple_to_file };

