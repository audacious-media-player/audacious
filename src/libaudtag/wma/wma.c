/* functions for WMA file handling */

#include <glib-2.0/glib.h>
#include "wma.h"
#include "guid.h"
#include "wma_fmt.h"
#include "../util.h"


int filePosition = 0;
int newfilePosition = 0;

HeaderObject header;
HeaderObject newHeader;

gboolean wma_can_handle_file(const char *file_path)
{	
	DEBUG_TAG("can handle file\n");
	int retval = FALSE;
	GUID *guid1 = guid_read_from_file(file_path,0);
	GUID *guid2 = guid_convert_from_string(ASF_HEADER_OBJECT_GUID);
	if(guid_equal(guid1, guid2))
		retval = TRUE;
	g_free(guid1);
	g_free(guid2);
	return retval;
}

void readHeaderObject(VFSFile *f)
{
    DEBUG_TAG("read header object\n");

    //we have allready read the GUID so increase filePositions with 16
    filePosition = 16;
    vfs_fseek(f,filePosition,SEEK_SET);
    //read the header size
    guint64 hs;
    vfs_fread(&hs,8,1,f);
    header.size = hs;
    filePosition += 8;
    //read  the number of objects
    guint32 ho;
    vfs_fread(&ho,4,1,f);
    header.objectsNr = ho;
    filePosition += 4;
    //8+8 reserved bytes 

    filePosition += 2;
    return;
}

Tuple *readCodecName(VFSFile *f, Tuple *tuple)
{
	guint64 object_size ;
    guint32 entriesCount;
    guint16 codec_name_length;
    gunichar2* codec_name;
    
	vfs_fseek(f, filePosition +16, SEEK_SET);
	vfs_fread(&object_size, 8,1,f);
	/* skip reserved bytes */
	vfs_fseek(f,16,SEEK_CUR);
    vfs_fread(&entriesCount, 4, 1, f);

	if(entriesCount >= 1)
	{
		/* skip type */
		vfs_fseek(f,2,SEEK_CUR);
		vfs_fread(&codec_name_length,2,1,f);
		codec_name = g_new0(gunichar2, codec_name_length);
		vfs_fread(codec_name,codec_name_length*2,1,f);
		gchar* cnameUTF8 = g_utf16_to_utf8(codec_name,codec_name_length,NULL,NULL,NULL);
		tuple_associate_string(tuple,FIELD_CODEC,NULL,cnameUTF8);
    }

	filePosition += object_size;
   
    return tuple;
}

Tuple *readFilePropObject(VFSFile *f, Tuple *tuple )
{
	guint64  size ;
	guint64 creationDate;
	guint64 playDuration;
	guint16 year;	
	
	/*jump over the GUID */
	vfs_fseek(f,filePosition + 16, SEEK_SET);
	/* read the object size */
	
	vfs_fread(&size,8,1,f);
	/* ignore file id and file size 16 + 8 = 24*/
	vfs_fseek(f,24,SEEK_CUR);
	/*read creadtion date -   given as the number of 100-nanosecond
	intervals since January 1, 1601  */
	vfs_fread(&creationDate,8,1,f);	
	year = get_year(creationDate);	
	DEBUG_TAG("Year = %d\n",year);		
	vfs_fseek(f,8,SEEK_CUR);	
	/* read play duration - time needed to play the file in 100-nanosecond 
	units */	
	vfs_fread(&playDuration,8,1,f);
	DEBUG_TAG("play duration = %lld\n",playDuration);	
	/* increment filePosition */	
	filePosition += size;
	
	tuple_associate_int(tuple,FIELD_LENGTH,NULL,playDuration/1000);
	DEBUG_TAG("length = %d\n",playDuration/10000);
	tuple_associate_int(tuple,FIELD_YEAR,NULL,year);

	return tuple;
}

Tuple *readContentDescriptionObject(VFSFile *f, Tuple *tuple)
{
	guint64 size;
	guint16 titleLen;
	guint16 authorLen;
	guint16 copyrightLen;
	guint16 descLen;
	
	gunichar2 *title;
	gunichar2 *author;
	gunichar2 *copyright;
	gunichar2 *desc;	
	
	
	vfs_fseek(f, filePosition +16, SEEK_SET);
	
	vfs_fread(&size,8,1,f);
	vfs_fread(&titleLen,2,1,f);
	vfs_fread(&authorLen,2,1,f);
	vfs_fread(&copyrightLen,2,1,f);
	vfs_fread(&descLen,2,1,f);
	/* ignore rating length */
	vfs_fseek(f,2,SEEK_CUR);
	
	title = g_new0(gunichar2,titleLen/sizeof(gunichar2));
	author = g_new0(gunichar2,authorLen/sizeof(gunichar2));
	copyright = g_new0(gunichar2,copyrightLen/sizeof(gunichar2));
	desc = g_new0(gunichar2,descLen/sizeof(gunichar2));

	
	vfs_fread(title, titleLen, 1, f);
	vfs_fread(author, authorLen, 1, f);
	vfs_fread(copyright, copyrightLen, 1, f);
	vfs_fread(desc, descLen, 1, f);
	
	filePosition += size;
	g_utf16_to_utf8(title,titleLen,NULL,NULL,NULL);
	tuple_associate_string(tuple,FIELD_TITLE,NULL,g_utf16_to_utf8(title,titleLen,NULL,NULL,NULL));
	tuple_associate_string(tuple,FIELD_ARTIST,NULL,g_utf16_to_utf8(author,authorLen,NULL,NULL,NULL));
	tuple_associate_string(tuple,FIELD_COPYRIGHT,NULL,g_utf16_to_utf8(copyright,copyrightLen,NULL,NULL,NULL));
	
	tuple_associate_string(tuple,FIELD_COMMENT,NULL,g_utf16_to_utf8(desc,descLen,NULL,NULL,NULL));
	
	return tuple;
}


Tuple *readExtendedContentObj(VFSFile *f, Tuple *tuple)
{
	guint64 size;
	guint16 countDescriptors;
	int i;	
	vfs_fseek(f,filePosition+16, SEEK_SET);
	vfs_fread(&size,8,1,f);
	vfs_fread(&countDescriptors,2,1,f);

	for(i =0; i<countDescriptors; i++)
	{
		guint16 nameLen;
		guint16 valueDataType;
		guint16 valueLen;
		gunichar2 *valueStr;
		gunichar2 *name;
		gchar* utf8Name;
		guint32 valueDW;
		guint32 valueBool;
		vfs_fread(&nameLen,2,1,f);
	
		name = g_new0(gunichar2,nameLen/sizeof(gunichar2));
		
		vfs_fread(name,nameLen,1,f);
		utf8Name =  g_utf16_to_utf8 (name,nameLen,NULL,NULL,NULL);
		DEBUG_TAG("name = %s\n",utf8Name);
	
		vfs_fread(&valueDataType,2,1,f);
		vfs_fread(&valueLen,2,1,f);
		switch(valueDataType)
		{
			case 0:
			{
				valueStr = g_new0(gunichar2,valueLen/sizeof(gunichar2));
				vfs_fread(valueStr,valueLen,1,f);
				
				gchar* utf8Value = g_utf16_to_utf8(valueStr,valueLen,NULL,NULL,NULL);
				DEBUG_TAG("value = %s\n", utf8Value);
				
				if(!g_strcmp0(utf8Name,"WM/Genre"))
				{
					tuple_associate_string(tuple,FIELD_GENRE,NULL,utf8Value);
					
				}
				if(!g_strcmp0(utf8Name,"WM/AlbumTitle"))
				{
					tuple_associate_string(tuple,FIELD_ALBUM,NULL,utf8Value);
				}
				
				if(!g_strcmp0(utf8Name,"WM/TrackNumber"))
				{
					DEBUG_TAG("track number \n");
					tuple_associate_int(tuple,FIELD_TRACK_NUMBER,NULL,atoi(utf8Value));
				}
				
			}break;
			
			case 1:
			{
				vfs_fseek(f,valueLen,SEEK_CUR);
			}break;
			
			case 2:
			{
				vfs_fread(&valueBool,4,1,f);
			}break;
			
			case 3:
			{
				vfs_fread(&valueDW,4,1,f);
			}break;
			
		}

	}
	
	filePosition += size;
	return tuple;
}

void readASFObject(VFSFile *f)
{
	vfs_fseek(f, filePosition + 16, SEEK_SET);
	guint64 size;
	vfs_fread(&size,8,1,f);
	
	filePosition += size;
}



Tuple *wma_populate_tuple_from_file(Tuple* tuple)
{
    DEBUG_TAG("wma populate tuple from file\n");
    VFSFile *file;
    /*open the file with the path received in tuple */
    const  gchar *file_path ;//= tuple_get_string(tuple,FIELD_FILE_PATH,NULL);
    /*   -------------- FOR TESTING ONLY ---------------- */
	file_path = "/home/paula/test.wma";
    
	file = vfs_fopen(file_path,"r");
    
    if(file == NULL)
        DEBUG_TAG("fopen error\n");
    else
        DEBUG_TAG("fopen ok\n");

    readHeaderObject(file);
    int i;

    for(i=0;i<header.objectsNr;i++)
    {
        const GUID *guid  = g_new0(GUID,1);
		memcpy(guid,guid_read_from_file(file_path, filePosition),sizeof(GUID));
        int guid_type = get_guid_type(guid);
        switch(guid_type)
        {
            case ASF_CODEC_LIST_OBJECT:
            {
               DEBUG_TAG("codec header  \n");
			   tuple = readCodecName(file, tuple);
            } break;

            case ASF_CONTENT_DESCRIPTION_OBJECT:
            {
                DEBUG_TAG("content description\n");
				tuple = readContentDescriptionObject(file, tuple);
            } break;
			case ASF_FILE_PROPERTIES_OBJECT:
			{
				DEBUG_TAG("file properties object\n");
				/* get year and play duration from here */
				tuple = readFilePropObject(file, tuple);
			
			}break;
			
			case ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT:
			{
				DEBUG_TAG("asf extended content description object\n");
				tuple = readExtendedContentObj(file, tuple);
			}break;
			
			default:
			{
				DEBUG_TAG("default\n");
				readASFObject(file);
			}
        }
    }
	/* wma = lossy */
	tuple_associate_string(tuple,FIELD_QUALITY,NULL,"lossy");

	printTuple(tuple);
	vfs_fclose(file);
	wma_write_tuple_to_file(tuple);
	return tuple;
}

void copyHeaderObject(VFSFile *from, VFSFile *to)
{
	/*copy guid */
	gchar buf[16];	
	vfs_fread(buf,16,1,from);
	vfs_fwrite(buf,16,1,to);
	gchar buf2[2];
	/*read and copy total size */
	vfs_fread(&newHeader.size,8,1,from);
	vfs_fwrite(&newHeader.size,8,1,to);
	/* read and copy number of header objects */
	vfs_fread(&newHeader.objectsNr,4,1,from);
		
	vfs_fwrite(&newHeader.objectsNr,4,1,to);

	/* copy the next 2 reserved bytes */
	vfs_fread(buf2,2,1,from);
	vfs_fwrite(buf2,2,1,to);
	
	newfilePosition += 30;
	filePosition += 30;
}

void copyASFObject(VFSFile *from, VFSFile *to)
{
	/* copy GUID */
	gchar buf[16];
	guint64 totalSize;
	
	vfs_fseek(to,newfilePosition,SEEK_SET);
	vfs_fseek(from,filePosition,SEEK_SET);
	
	vfs_fread(buf,16,1,from);
	vfs_fwrite(buf,16,1,to);

	/*read and copy total size */
	vfs_fread(&totalSize,8,1,from);
	vfs_fwrite(&totalSize,8,1,to);
				
	DEBUG_TAG("total size = %d\n",totalSize);
	/*read and copy the rest of the object */
	char buf2[totalSize];
	vfs_fread(buf2,totalSize,1,from);
	vfs_fwrite(buf2,totalSize,1,to);
	
	newfilePosition += totalSize;
	filePosition += totalSize;
}

void writeContentDescriptionObject(VFSFile *from, VFSFile *to, Tuple *tuple)
{
	gchar buf[16];
	guint64 size = 16; /* init with the guid size */
	guint16 titleLen;
	guint16 authorLen;
	guint16 copyrightLen;
	guint16 descLen;
	guint16 ratingLen;
	
	glong title_tuple_len;
	glong author_tuple_len;
	glong copyright_tuple_len;
	glong desc_tuple_len;
	
	gunichar2 *title = g_utf8_to_utf16(tuple_get_string(tuple,FIELD_TITLE,NULL),-1, NULL,&title_tuple_len,NULL);
	gunichar2 *author = g_utf8_to_utf16(tuple_get_string(tuple,FIELD_ARTIST,NULL),-1, NULL,&author_tuple_len,NULL);
/*	gunichar2 *copyright = g_utf8_to_utf16(tuple_get_string(tuple,FIELD_COPYRIGHT,NULL),-1, NULL,&copyright_tuple_len,NULL);*/
	gunichar2 *desc = g_utf8_to_utf16(tuple_get_string(tuple,FIELD_COMMENT,NULL),-1, NULL,&desc_tuple_len,NULL);	
	
	
	/* copy guid */ 
	vfs_fread(buf,16,1,from);
	vfs_fwrite(buf,16,1,to);
	
	/* write a size -- we will change it afterwards */
	vfs_fwrite(&size,8,1,to);
	
	
	
	if(title != NULL)
	{
		titleLen = title_tuple_len;
		vfs_fwrite(&title_tuple_len,2,1,to);
		vfs_fwrite(title,title_tuple_len,1,to);
		vfs_fseek(from,titleLen+2,SEEK_CUR);
		size += title_tuple_len+2;
	}
	else 
	{
		
		vfs_fwrite(&titleLen,2,1,to);
		
		gunichar2 t[titleLen];
		vfs_fread(t,titleLen,1,from);
		vfs_fwrite(t,titleLen,1,to);
		size += titleLen;
	}

	
	if(author != NULL)
	{
		authorLen = author_tuple_len;
		vfs_fwrite(&author_tuple_len,2,1,to);
		vfs_fwrite(author,author_tuple_len,1,to);
		vfs_fseek(from,authorLen+2,SEEK_CUR);		
	}else
	{
		vfs_fread(&authorLen,2,1,from);
		vfs_fwrite(&authorLen,2,1,to);
		
		gunichar2 a[authorLen];
		vfs_fread(a,authorLen,1,from);
		vfs_fwrite(a,authorLen,1,to);
	}
	size += authorLen;
	
// 	if(copyright != NULL)
// 	{
// 		copyrightLen = copyright_tuple_len;
// 		vfs_fwrite(copyrightLen,2,1,to);
// 		vfs_fwrite(copyright,copyrightLen,1,to);
// 		vfs_fseek(from,copyrightLen,SEEK_CUR);
// 	}else
// 	{
		vfs_fread(&copyrightLen,2,1,from);
		vfs_fwrite(&copyrightLen,2,1,to);
		
		gunichar2 c[copyrightLen];
		vfs_fread(c,copyrightLen,1,from);
		vfs_fwrite(c,copyrightLen,1,to);
// 	}
	size += copyrightLen;
	
	if(desc != NULL)
	{
		descLen = desc_tuple_len;
		vfs_fwrite(&descLen,2,1,to);
		vfs_fwrite(desc,descLen,1,to);
		vfs_fseek(from, descLen+2, SEEK_CUR);
	}else
	{
		vfs_fread(&descLen,2,1,from);
		vfs_fwrite(&descLen,2,1,to);
		
		gunichar2 d[descLen];
		vfs_fread(d,descLen,1,from);
		vfs_fwrite(d,descLen,1,to);
	}
	size += descLen;

	vfs_fread(&ratingLen,2,1,from);
	vfs_fwrite(&ratingLen,2,1,to);
	
	gunichar2 rating[ratingLen];
	vfs_fread(rating,ratingLen,1,from);
	vfs_fwrite(rating,ratingLen,1,to);
	size += ratingLen;
	
	/* write the corect total size to header */
	/*newfilePosition is pointing to the start of this object  */
	
	vfs_fseek(to,newfilePosition + 16 ,SEEK_SET);
	vfs_fwrite(&size,8,1,to);
	newfilePosition += size;
	vfs_fseek(to,newfilePosition,SEEK_SET);
}


void writeExtendedContentObj(VFSFile *from, VFSFile *to, Tuple *tuple)
{
	guint64 size;
	guint64 newsize;
	guint16 content_count;
	int found_album = 0;
	int found_genre = 0;
	int found_tracknr = 0;
	int found_year = 0;
	int i;
	gchar buf[16];
	
	vfs_fseek(to,newfilePosition,SEEK_SET);
	vfs_fseek(from,filePosition,SEEK_SET);
		
	/* copy guid */ 
	vfs_fread(buf,16,1,from);
	vfs_fwrite(buf,16,1,to);
	/* size  - we will change this later */
	vfs_fread(&size,8,1,from);	
	filePosition += size;
	vfs_fwrite(&size,8,1,to);
	
	/* content count - we might change this later */
	vfs_fread(&content_count,2,1,from);
	vfs_fwrite(&content_count,2,1,to);
	newsize = 16+8+2;
	for(i = 0; i<content_count; i++)
	{
		guint16 name_len;
		guint16 data_type;
		
		/* read the name */
		vfs_fread(&name_len,2,1,from);
		gunichar2 *name;
		
		name = g_new0(gunichar2,name_len/sizeof(gunichar2));		
		vfs_fread(name,name_len,1,from);
		
		gchar *utf8Name =  g_utf16_to_utf8 (name,name_len,NULL,NULL,NULL);
		
		DEBUG_TAG("NAME = %s\n",utf8Name);
		if(!g_strcmp0(utf8Name,"WM/Genre"))
		{
			found_genre = 1;
			/* write the name to the tmp file */
			vfs_fwrite(&name_len,2,1,to);
			vfs_fwrite(name,name_len,1,to);
			
			/*write the value data type and len we will write */
			/* genre = string */
			data_type = 0;
			vfs_fwrite(&data_type,2,1,to);
			//skip 2 in the original file */
			vfs_fseek(from,2,SEEK_CUR);
			glong genre_tuple_len;
			gunichar2 *genre = g_utf8_to_utf16(tuple_get_string(tuple,FIELD_GENRE,NULL),-1, NULL,&genre_tuple_len,NULL);
			genre_tuple_len *= 2;
			genre_tuple_len += 2;
			vfs_fwrite(&genre_tuple_len,2,1,to);
			vfs_fwrite(genre,genre_tuple_len,1,to);
			
			vfs_fseek(from,genre_tuple_len+2, SEEK_CUR);
			continue;
		
		}
		if(!g_strcmp0(utf8Name,"WM/AlbumTitle"))
		{
			found_album = 1;
			/* write the name to the tmp file */
			vfs_fwrite(&name_len,2,1,to);
			vfs_fwrite(name,name_len,1,to);
			
			/*write the value data type and len we will write */
			/* album = string */
			data_type = 0;
			vfs_fwrite(&data_type,2,1,to);
			//skip 2 in the original file */

			guint16 album_tuple_len;
			gunichar2 *album = g_utf8_to_utf16(tuple_get_string(tuple,FIELD_ALBUM,NULL),-1, NULL,&album_tuple_len,NULL);
			album_tuple_len *= sizeof(gunichar2);
			album_tuple_len +=2;
			vfs_fwrite(&album_tuple_len,2,1,to);
			vfs_fwrite(album,album_tuple_len,1,to);
			
			vfs_fseek(from,album_tuple_len+4, SEEK_CUR);
			continue;
		}
		if(!g_strcmp0(utf8Name,"WM/TrackNumber"))
		{
			found_tracknr = 1;
			/* write the name to the tmp file */
			vfs_fwrite(&name_len,2,1,to);
			vfs_fwrite(name,name_len,1,to);
			
			/*write the value data type and len we will write */
			/* tracknumber  = int */
			data_type = 3;
			vfs_fwrite(&data_type,2,1,to);
			//skip 2 in the original file */
			vfs_fseek(from,2,SEEK_CUR);
			guint16 tracknr_tuple_len = 4;
			guint32 tracknr = tuple_get_int(tuple,FIELD_TRACK_NUMBER,NULL);

			vfs_fwrite(&tracknr_tuple_len,2,1,to);
			vfs_fwrite(&tracknr,tracknr_tuple_len,1,to);
			
			vfs_fseek(from,tracknr_tuple_len+2, SEEK_CUR);
			continue;
		}
		
			gchar buf[2];
			guint16 valueSize;
			DEBUG_TAG("copy datA \n");
			vfs_fwrite(&name_len,2,1,to);
			vfs_fwrite(name,name_len,1,to);
			
			vfs_fread(&data_type,2,1,from);
			vfs_fwrite(&data_type,2,1,to);

			
			vfs_fread(&valueSize,2,1,from);
			vfs_fwrite(&valueSize,2,1,to);
			
			valueSize = valueSize;
			gchar value[valueSize];
			vfs_fread(value,valueSize,1,from);
			vfs_fwrite(value,valueSize,1,to);	
	
	}
	
	
}
void writeHeaderExtensionObject(VFSFile *from, VFSFile *to)
{

	gchar buf[16];
	guint64 size;
	DEBUG_TAG("file position = %d\n",filePosition);
	vfs_fseek(to,newfilePosition,SEEK_SET);
	vfs_fseek(from,filePosition,SEEK_SET);
	/* copy guid */ 
	vfs_fread(buf,16,1,from);
	vfs_fwrite(buf,16,1,to);
	
	/* size  - we will change this later */
	vfs_fread(&size,8,1,from);	
	vfs_fwrite(&size,8,1,to);
	DEBUG_TAG("extension size = %d\n",size);
	gchar buf2[size];
	vfs_fread(buf2,size,1,from);
	vfs_fwrite(buf2,size,1,to);
	newfilePosition += size;
	filePosition += size;
}

/* TODO move this to util since it can be used for all the other formats */
void writeAudioData(VFSFile *from, VFSFile *to)
{
	printf("zzz %d\n",ftell(from));
	while(vfs_feof(from) != 0)
	{
		printf("zzz %d\n",ftell(from));
		gchar buf[4096];
		gint n = vfs_fread(buf,1,4096,from);
		printf("copy %d bytes \n",n);
		vfs_fwrite(buf,n,1,to);
	}
}

gboolean wma_write_tuple_to_file (Tuple* tuple)
{
	newfilePosition = 0;
	filePosition = 0;
	VFSFile *file;
	VFSFile *tmpFile;
	int i;
    /*open the file with the path received in tuple */
    const  gchar *file_path ;//= tuple_get_string(tuple,FIELD_FILE_PATH,NULL);
    /*   -------------- FOR TESTING ONLY ---------------- */
	file_path = "/home/paula/test.wma";
	gchar *tmp_path = "/tmp/tmpwma.wma";    
	file = vfs_fopen(file_path,"r");
	tmpFile = vfs_fopen(tmp_path, "w");
    
    if(tmpFile == NULL)
        DEBUG_TAG("fopen error\n");
    else
        DEBUG_TAG("fopen ok\n");
	
    copyHeaderObject(file, tmpFile);


    for(i=0;i<newHeader.objectsNr;i++)
    {
        const GUID *guid  = g_new0(GUID,1);
		memcpy(guid,guid_read_from_file(file_path, filePosition),sizeof(GUID));
        int guid_type = get_guid_type(guid);
		DEBUG_TAG("guid type = %d\n",guid_type);
        switch(guid_type)
        {
            case ASF_CONTENT_DESCRIPTION_OBJECT:
            {
                DEBUG_TAG("content description\n");
				writeContentDescriptionObject(file,tmpFile, tuple);
            } break;
		
			case ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT:
			{
				DEBUG_TAG("asf extended content description object\n");
				writeExtendedContentObj(file,tmpFile, tuple);
			}break;
			case ASF_HEADER_EXTENSION_OBJECT:
			{
				DEBUG_TAG("header extension \n");
				writeHeaderExtensionObject(file, tmpFile);
			}break;
			default:
			{
				
				DEBUG_TAG("default\n");
				DEBUG_TAG("asf object = %d\n",guid_type);
				copyASFObject(file,tmpFile);
			}
        }
    }
	/* write the rest of the file */
	
	writeAudioData(file,tmpFile);
	
	vfs_fclose(file);
	vfs_fclose(tmpFile);
	
}