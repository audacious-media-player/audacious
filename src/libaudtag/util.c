
#include "util.h"





/* convert windows time to unix time */
time_t unix_time(guint64 win_time)
{
	guint64 t = (guint64)((win_time/10000000LL) - 11644473600LL);
	return (time_t)t;
}

guint16 get_year(guint64 win_time)
{
	GDate* d = g_date_new();
	g_date_set_time_t(d, unix_time(win_time));
	guint16 year = g_date_get_year(d); 
	g_date_free(d);
	return year;
}
Tuple *makeTuple(Tuple *tuple, const gchar* title, const gchar* artist,  
							   const gchar* comment, const gchar* album, 
							   const gchar * genre, const gchar* year, 
							   const gchar* filePath,int tracnr)
{

	tuple_associate_string(tuple,FIELD_ARTIST, NULL,artist);
	tuple_associate_string(tuple,FIELD_TITLE, NULL, title);
	tuple_associate_string(tuple, FIELD_COMMENT,NULL, comment);
	tuple_associate_string(tuple, FIELD_ALBUM,NULL, album);
	tuple_associate_string(tuple, FIELD_GENRE,NULL, genre);
	tuple_associate_string(tuple, FIELD_YEAR,NULL, year);
	tuple_associate_int(tuple, FIELD_TRACK_NUMBER,NULL, tracnr);
	tuple_associate_string(tuple, FIELD_FILE_PATH,NULL, filePath);
	return tuple;
}


const gchar* get_complete_filepath(Tuple *tuple)
{
    const gchar* filepath;
    const gchar* dir;
    const gchar* file;

    dir = tuple_get_string(tuple, FIELD_FILE_PATH, NULL);
    file = tuple_get_string(tuple, FIELD_FILE_NAME, NULL);
    filepath = g_strdup_printf("%s/%s",dir,file);
    printf("file path = %s\n",filepath);
    return filepath;
}


void printTuple(Tuple *tuple)
{
printf("--------------TUPLE PRINT --------------------\n");
	const gchar*  title = tuple_get_string(tuple,FIELD_TITLE,NULL);
	printf("title = %s\n",title);
	/* artist */
	const gchar*  artist = tuple_get_string(tuple,FIELD_ARTIST,NULL);
	printf("artist = %s\n",artist);
	
	/* copyright */
	const gchar* copyright = tuple_get_string(tuple, FIELD_COPYRIGHT,NULL);
	printf("copyright = %s\n",copyright);
	
	/* comment / description */
	
	const gchar* comment = tuple_get_string(tuple, FIELD_COMMENT,NULL);
	printf("comment = %s\n",comment);
	
	/* codec name */	
	const gchar* codec_name = tuple_get_string(tuple,FIELD_CODEC,NULL);
	printf("codec = %s\n",codec_name);
	
	/* album */	
	const gchar* album = tuple_get_string(tuple,FIELD_ALBUM,NULL);
	printf("Album = %s\n",album);
	
	/*track number */
	gint track_nr = tuple_get_int(tuple, FIELD_TRACK_NUMBER,NULL);
	printf("Track nr = %d\n",track_nr);
	
	/* genre */
	const gchar* genre = tuple_get_string(tuple, FIELD_GENRE,NULL);
	printf("Genre = %s \n",genre);
	
	/* length */
	gint length = tuple_get_int(tuple,FIELD_LENGTH,NULL);
	printf("Length = %d\n",length);
	
	/* year */
	gint year = tuple_get_int(tuple,FIELD_YEAR,NULL);
	printf("Year = %d\n",year);
	
	/* quality */
	const gchar* quality = tuple_get_string(tuple,FIELD_QUALITY,NULL);
	printf("quality = %s\n",quality);
	
	printf("-----------------END---------------------\n");
}