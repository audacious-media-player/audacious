
#include <stdio.h>
#include "libaudcore/tuple.h"
#include "libaudcore/vfs.h"
#include "../../util.h"
#include "../../wma/wma.h"

int main(int argc, char** argv)
{
	Tuple * test_tuple;
	GOptionContext *gcontext = g_option_context_new ( " fileName.wma" );
	gchar **files = NULL;

	GOptionEntry entries[] =
	{
		{G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &files, "files", NULL},
		{ NULL }
	};
	GError *err;
	g_option_context_add_main_entries ( gcontext, entries, NULL );
	if ( !g_option_context_parse ( gcontext, &argc, &argv, &err ) )
	{
		g_option_context_free (gcontext);
		g_print ( "option parsing failed:  %s\n" ,err->message);
		return 1;
	}

	g_option_context_free (gcontext);

	if ( files[1]  != NULL )
	{
		g_print ( "too many arguments!\n" );
		return 1;
	}

	FILE* f = fopen ( files[0],"r" );

	if ( !f )
	{
		g_print ( "couldn't open file '%s', exiting\n", files[0] );
		return 1;
	}

	test_tuple = tuple_new();
	gchar *z =g_filename_to_utf8(files[0], -1, NULL, NULL, NULL);
	tuple_associate_string(test_tuple, FIELD_FILE_PATH, NULL, z);

	const  gchar *file_path = tuple_get_string(test_tuple, FIELD_FILE_PATH, NULL);
	printf("file path = %s\n", file_path);
	test_tuple = wma_populate_tuple_from_file(test_tuple);

	printTuple(test_tuple);						   

	return 0;
}
