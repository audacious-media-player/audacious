/* functions for WMA file handling */
#include <glib.h>
#include <glib/gstdio.h>
#include <inttypes.h>

#include "wma.h"
#include "guid.h"
#include "wma_fmt.h"
#include "../util.h"
#include "../tag_module.h"



int filePosition = 0;
int newfilePosition = 0;

tag_module_t wma = {wma_can_handle_file, wma_populate_tuple_from_file, wma_write_tuple_to_file};

void writeGuidToFile(VFSFile *f, int guid_type);

gboolean wma_can_handle_file(VFSFile *fd) {
    DEBUG("can handle file\n");
        if(fd == NULL)
        DEBUG("F NULLL\n");
    int retval = FALSE;
    GUID *guid1 = g_new0(GUID, 1);
    memcpy(guid1, guid_read_from_file(fd, 0), sizeof (GUID));
    GUID *guid2 = g_new0(GUID, 1);
          memcpy(guid2,guid_convert_from_string(ASF_HEADER_OBJECT_GUID),sizeof(GUID));
    if (guid_equal(guid1, guid2))
        retval = TRUE;
    return retval;
}

HeaderObject *readHeaderObject(VFSFile *f) {
    DEBUG("read header object\n");
    HeaderObject *header = g_new0(HeaderObject, 1);

    //we have allready read the GUID so increase filePositions with 16
    filePosition = 16;
    vfs_fseek(f, filePosition, SEEK_SET);
    //read the header size
    guint64 hs;
    vfs_fread(&hs, 8, 1, f);
    header->size = hs;
    filePosition += 8;
    //read  the number of objects
    guint32 ho;
    vfs_fread(&ho, 4, 1, f);
    header->objectsNr = ho;
    filePosition += 4;
    //8+8 reserved bytes

    filePosition += 2;
    return header;
}

Tuple *readCodecName(VFSFile *f, Tuple *tuple) {
    guint64 object_size;
    guint32 entriesCount;
    guint16 codec_name_length;
    gunichar2* codec_name;

    if(f==NULL)
    {
        DEBUG("fd null\n");
        return NULL;
    }

    vfs_fseek(f, filePosition + 16, SEEK_SET);
    vfs_fread(&object_size, 8, 1, f);

    /* skip reserved bytes */
    vfs_fseek(f, 16, SEEK_CUR);
    vfs_fread(&entriesCount, 4, 1, f);

    if (entriesCount >= 1) {
        /* skip type */
        vfs_fseek(f, 2, SEEK_CUR);
        vfs_fread(&codec_name_length, 2, 1, f);
        codec_name = g_new0(gunichar2, codec_name_length);
        vfs_fread(codec_name, codec_name_length * 2, 1, f);
        gchar* cnameUTF8 = g_utf16_to_utf8(codec_name, codec_name_length, NULL, NULL, NULL);
        tuple_associate_string(tuple, FIELD_CODEC, NULL, cnameUTF8);
            DEBUG("sdasfas\n");
    }
     DEBUG("sdasfas\n");
    filePosition += object_size;
    DEBUG("after read codec name\n");
    return tuple;
}

Tuple *readFilePropObject(VFSFile *f, Tuple *tuple) {
    guint64 size;
    guint64 creationDate;
    guint64 playDuration;
    guint16 year;

    /*jump over the GUID */
    vfs_fseek(f, filePosition + 16, SEEK_SET);
    /* read the object size */

    vfs_fread(&size, 8, 1, f);
    /* ignore file id and file size 16 + 8 = 24*/
    vfs_fseek(f, 24, SEEK_CUR);
    /*read creation date - given as the number of 100-nanosecond
    intervals since January 1, 1601  */
    vfs_fread(&creationDate, 8, 1, f);
    year = get_year(creationDate);
    DEBUG("Year = %d\n", year);
    vfs_fseek(f, 8, SEEK_CUR);
    /* read play duration - time needed to play the file in 100-nanosecond
    units */
    vfs_fread(&playDuration, 8, 1, f);
    DEBUG("play duration = %"PRId64"\n", playDuration);
    /* increment filePosition */
    filePosition += size;

    tuple_associate_int(tuple, FIELD_LENGTH, NULL, (int) playDuration / 1000);
    DEBUG("length = %"PRId64"\n", playDuration / 10000);
    //tuple_associate_int(tuple, FIELD_YEAR, NULL, year);

    return tuple;
}

Tuple *readContentDescriptionObject(VFSFile *f, Tuple *tuple) {
    guint64 size;
    guint16 titleLen;
    guint16 authorLen;
    guint16 copyrightLen;
    guint16 descLen;

    gunichar2 *title;
    gunichar2 *author;
    gunichar2 *copyright;
    gunichar2 *desc;


    vfs_fseek(f, filePosition + 16, SEEK_SET);

    vfs_fread(&size, 8, 1, f);
    vfs_fread(&titleLen, 2, 1, f);
    vfs_fread(&authorLen, 2, 1, f);
    vfs_fread(&copyrightLen, 2, 1, f);
    vfs_fread(&descLen, 2, 1, f);
    /* ignore rating length */
    vfs_fseek(f, 2, SEEK_CUR);

    // 	title = g_new0(gunichar2,titleLen/sizeof(gunichar2));
    // 	author = g_new0(gunichar2,authorLen/sizeof(gunichar2));
    // 	copyright = g_new0(gunichar2,copyrightLen/sizeof(gunichar2));
    // 	desc = g_new0(gunichar2,descLen/sizeof(gunichar2));

    title = g_new0(gunichar2, titleLen);
    author = g_new0(gunichar2, authorLen);
    copyright = g_new0(gunichar2, copyrightLen);
    desc = g_new0(gunichar2, descLen);
    if (titleLen != 0) {
        vfs_fread(title, titleLen, 1, f);
    }
    if (authorLen != 0) {
        vfs_fread(author, authorLen, 1, f);
    }
    if (copyrightLen != 0) {
        vfs_fread(copyright, copyrightLen, 1, f);
    }
    if (descLen != 0) {
        vfs_fread(desc, descLen, 1, f);
    }

    filePosition += size;
    g_utf16_to_utf8(title, titleLen, NULL, NULL, NULL);
    tuple_associate_string(tuple, FIELD_TITLE, NULL, g_utf16_to_utf8(title, titleLen, NULL, NULL, NULL));
    tuple_associate_string(tuple, FIELD_ARTIST, NULL, g_utf16_to_utf8(author, authorLen, NULL, NULL, NULL));
    tuple_associate_string(tuple, FIELD_COPYRIGHT, NULL, g_utf16_to_utf8(copyright, copyrightLen, NULL, NULL, NULL));

    tuple_associate_string(tuple, FIELD_COMMENT, NULL, g_utf16_to_utf8(desc, descLen, NULL, NULL, NULL));

    return tuple;
}

Tuple *readExtendedContentObj(VFSFile *f, Tuple *tuple) {
    guint64 size;
    guint16 countDescriptors;
    int i;
    vfs_fseek(f, filePosition + 16, SEEK_SET);
    vfs_fread(&size, 8, 1, f);
    vfs_fread(&countDescriptors, 2, 1, f);

    for (i = 0; i < countDescriptors; i++) {
        guint16 nameLen;
        guint16 valueDataType;
        guint16 valueLen;
        gunichar2 *valueStr;
        gunichar2 *name;
        gchar* utf8Name;
        guint32 valueDW;
        guint32 valueBool;
        vfs_fread(&nameLen, 2, 1, f);

        name = g_new0(gunichar2, nameLen / sizeof (gunichar2));

        vfs_fread(name, nameLen, 1, f);
        utf8Name = g_utf16_to_utf8(name, nameLen, NULL, NULL, NULL);
        DEBUG("name = %s\n", utf8Name);

        vfs_fread(&valueDataType, 2, 1, f);
        vfs_fread(&valueLen, 2, 1, f);

        if (valueLen == 0)
            break;

        switch (valueDataType) {
            case 0:
            {
                valueStr = g_new0(gunichar2, valueLen / sizeof (gunichar2));
                vfs_fread(valueStr, valueLen, 1, f);

                gchar* utf8Value = g_utf16_to_utf8(valueStr, valueLen, NULL, NULL, NULL);
                DEBUG("value = %s\n", utf8Value);

                if (utf8Name != NULL) {
                    if (!strcmp(utf8Name, "WM/Genre")) {
                        tuple_associate_string(tuple, FIELD_GENRE, NULL, utf8Value);

                    }

                    if (!strcmp(utf8Name, "WM/AlbumTitle")) {
                        tuple_associate_string(tuple, FIELD_ALBUM, NULL, utf8Value);
                    }

                    if (!strcmp(utf8Name, "WM/TrackNumber")) {
                        DEBUG("track number \n");
                        tuple_associate_int(tuple, FIELD_TRACK_NUMBER, NULL, atoi(utf8Value));
                    }
                }

            }
                break;

            case 1:
            {
                vfs_fseek(f, valueLen, SEEK_CUR);
            }
                break;

            case 2:
            {
                vfs_fread(&valueBool, 4, 1, f);
            }
                break;

            case 3:
            {
                vfs_fread(&valueDW, 4, 1, f);
            }
                break;

        }

    }

    filePosition += size;
    return tuple;
}

void readASFObject(VFSFile *f) {
    vfs_fseek(f, filePosition + 16, SEEK_SET);
    guint64 size;
    vfs_fread(&size, 8, 1, f);

    filePosition += size;
}

Tuple *wma_populate_tuple_from_file(VFSFile *fd) {
    DEBUG("wma populate tuple from file\n");


    Tuple *tuple = tuple_new_from_filename(fd->uri);
    HeaderObject *header = readHeaderObject(fd);
    int i;

    for (i = 0; i < header->objectsNr; i++) {
        GUID *guid = g_new0(GUID, 1);
        memcpy(guid, guid_read_from_file(fd, filePosition), sizeof (GUID));
        int guid_type = get_guid_type(guid);
        switch (guid_type) {
            case ASF_CODEC_LIST_OBJECT:
            {
                DEBUG("codec header  \n");
                tuple = readCodecName(fd, tuple);
            }
                break;

            case ASF_CONTENT_DESCRIPTION_OBJECT:
            {
                DEBUG("content description\n");
                tuple = readContentDescriptionObject(fd, tuple);
            }
                break;
            case ASF_FILE_PROPERTIES_OBJECT:
            {
                DEBUG("file properties object\n");
                /* get year and play duration from here */
                tuple = readFilePropObject(fd, tuple);

            }
                break;

            case ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT:
            {
                DEBUG("asf extended content description object\n");
                tuple = readExtendedContentObj(fd, tuple);
            }
                break;

            default:
            {
                DEBUG("default\n");
                readASFObject(fd);
            }
        }
    }
    /* wma = lossy */
    tuple_associate_string(tuple, FIELD_QUALITY, NULL, "lossy");

    printTuple(tuple);
    //wma_write_tuple_to_file(tuple);
    return tuple;
}

HeaderObject *copyHeaderObject(VFSFile *from, VFSFile *to) {
    /*copy guid */
    gchar buf[16];
    HeaderObject *newHeader = g_new0(HeaderObject, 1);

    vfs_fread(buf, 16, 1, from);
    vfs_fwrite(buf, 16, 1, to);
    gchar buf2[2];
    /*read and copy total size */
    vfs_fread(&newHeader->size, 8, 1, from);
    vfs_fwrite(&newHeader->size, 8, 1, to);
    DEBUG("HEADER %"PRId64"\n", newHeader->size);
    /* read and copy number of header objects */
    vfs_fread(&newHeader->objectsNr, 4, 1, from);

    vfs_fwrite(&newHeader->objectsNr, 4, 1, to);

    /* copy the next 2 reserved bytes */
    vfs_fread(buf2, 2, 1, from);
    vfs_fwrite(buf2, 2, 1, to);

    newfilePosition += 30;
    filePosition += 30;
    return newHeader;
}

void copyASFObject(VFSFile *from, VFSFile *to) {
    /* copy GUID */
    gchar buf[16];
    guint64 totalSize;

    vfs_fseek(to, newfilePosition, SEEK_SET);
    vfs_fseek(from, filePosition, SEEK_SET);

    vfs_fread(buf, 16, 1, from);
    vfs_fwrite(buf, 16, 1, to);

    /*read and copy total size */
    vfs_fread(&totalSize, 8, 1, from);
    vfs_fwrite(&totalSize, 8, 1, to);

    DEBUG("total size = %"PRId64"\n", totalSize);
    /*read and copy the rest of the object */
    char buf2[totalSize];
    vfs_fread(buf2, totalSize - 24, 1, from);
    vfs_fwrite(buf2, totalSize - 24, 1, to);

    newfilePosition += totalSize;
    filePosition += totalSize;
}

ContentField getStringContentFromTuple(Tuple *tuple, int nfield) {
    ContentField content;
    glong length = 0;
    content.strValue = g_utf8_to_utf16(tuple_get_string(tuple, nfield, NULL), -1, NULL, &length, NULL);
    if(content.strValue == NULL)
    {
        content.size = 0;
        return content;
    }
    length *= sizeof (gunichar2);
    DEBUG("len 1 = %ld\n", length);
    length += 2;
    content.size = length;
    DEBUG("len 2 = %ld\n", length);
    return content;
}

gint writeContentFieldSizeToFile(VFSFile *to, ContentField c, int filepos) {
    vfs_fwrite(&c.size, 2, 1, to);
    newfilePosition += 2;
    return 2;
}

gint writeContentFieldValueToFile(VFSFile *to, ContentField c, int filepos) {
    if (c.strValue == NULL) {
        c.strValue = g_new0(gunichar2, 2);
    }
    DEBUG("STR VAL = %s\n", g_utf16_to_utf8(c.strValue, -1, NULL, NULL, NULL));
    DEBUG("C Size = %d\n", c.size);
    vfs_fwrite(c.strValue, c.size, 1, to);
    newfilePosition += c.size;

    return c.size;
}

void printContentField(ContentField c) {
    DEBUG("------------- ContentField ------------------\n");
    DEBUG("ContentField size : %d\n", c.size);
    DEBUG("ContentField value : %s\n", g_utf16_to_utf8(c.strValue, c.size, NULL, NULL, NULL));
    DEBUG("------------- END ---------------------------\n");
}

void copyData(VFSFile *from, VFSFile *to, int posFrom, int posTo, gint size) {
    gchar bufer[size];
    vfs_fseek(from, posFrom, SEEK_SET);
    vfs_fseek(to, posTo, SEEK_SET);
    vfs_fread(bufer, size, 1, from);
    vfs_fwrite(bufer, size, 1, to);

    filePosition += size;
    newfilePosition += size;
}

void copySize(VFSFile *from, VFSFile *to, int posFrom, int posTo) {
    gchar buf[8];
    // 	/* copy guid */
    vfs_fseek(from, posFrom, SEEK_SET);
    vfs_fseek(to, posTo, SEEK_SET);
    vfs_fread(buf, 8, 1, from);
    vfs_fwrite(buf, 8, 1, to);
    filePosition += 8;
    newfilePosition += 8;

}

void skipObjectSizeFromFile(VFSFile *from) {
    vfs_fseek(from, 2, SEEK_CUR);
    filePosition += 2;
}

gint copyContentObject(VFSFile * from, VFSFile *to) {
    guint16 s;
    vfs_fread(&s, 2, 1, from);
    vfs_fwrite(&s, 2, 1, to);

    gunichar2 t[s];

    vfs_fread(t, s, 1, from);
    vfs_fwrite(t, s, 1, to);

    filePosition += s + 2;
    newfilePosition += s + 2;

    return s + 2;
}

void writeContentDescriptionObject(VFSFile *from, VFSFile *to, Tuple *tuple) {
    guint64 size;

    ContentField title;
    ContentField author;
    ContentField description;
    ContentField ititle;
    ContentField iauthor;
    ContentField idescription;
    ContentField icopyright;
    ContentField irating;

    title = getStringContentFromTuple(tuple, FIELD_TITLE);
    author = getStringContentFromTuple(tuple, FIELD_ARTIST);
    description = getStringContentFromTuple(tuple, FIELD_COMMENT);

    printContentField(title);
    printContentField(author);
    printContentField(description);

    copyData(from, to, filePosition, newfilePosition, 24);
    size = 24;
    if (title.size != 0) {

        size += writeContentFieldSizeToFile(to, title, newfilePosition);
        /* read the size and  advance in the from file */
        vfs_fread(&(ititle.size), 2, 1, from);
    } else {
        vfs_fread(&(ititle.size), 2, 1, from);
        int tmp = writeContentFieldSizeToFile(to, ititle, newfilePosition);
        tmp = 0;
    }

    filePosition += 2;

    if (author.size != 0) {
        size += writeContentFieldSizeToFile(to, author, newfilePosition);
        /* read the size and  advance in the from file */
        vfs_fread(&(iauthor.size), 2, 1, from);
    } else {
        vfs_fread(&(iauthor.size), 2, 1, from);
        int tmp = writeContentFieldSizeToFile(to, iauthor, newfilePosition);
        tmp = 0;
    }
    filePosition += 2;


    /* copyright */
    vfs_fread(&(icopyright.size), 2, 1, from);
    filePosition += 2;
    int k = writeContentFieldSizeToFile(to, icopyright, newfilePosition);

    if (description.size != 0) {
        size += writeContentFieldSizeToFile(to, description, newfilePosition);
        /* read the size and  advance in the from file */
        vfs_fread(&(idescription.size), 2, 1, from);
    } else {
        vfs_fread(&(idescription.size), 2, 1, from);
        int tmp = writeContentFieldSizeToFile(to, idescription, newfilePosition);
        tmp = 0;
    }
    filePosition += 2;

    /* rating */
    vfs_fread(&(irating.size), 2, 1, from);
    filePosition += 2;
    k = writeContentFieldSizeToFile(to, irating, newfilePosition);

    /* write the content */

    if (title.size != 0) {
        size += writeContentFieldValueToFile(to, title, newfilePosition);
        //skip this on the from file
        vfs_fseek(from, ititle.size, SEEK_CUR);
        filePosition += ititle.size;
    } else if(ititle.size != 0) {

        vfs_fread(ititle.strValue, ititle.size, 1, from);
        size += writeContentFieldValueToFile(to, ititle, newfilePosition);
        filePosition += ititle.size;
    }

    if (author.size != 0) {
        size += writeContentFieldValueToFile(to, author, newfilePosition);
        //skip this on the from file
        vfs_fseek(from, iauthor.size, SEEK_CUR);
        filePosition += iauthor.size;
    } else if (iauthor.size != 0){
        vfs_fread(iauthor.strValue, iauthor.size, 1, from);
        size += writeContentFieldValueToFile(to, iauthor, newfilePosition);
        filePosition += iauthor.size;
    }


    vfs_fseek(from, filePosition, SEEK_SET);
    if(icopyright.size != 0)
    {
        vfs_fread(icopyright.strValue, icopyright.size, 1, from);
        size += writeContentFieldValueToFile(to, icopyright, newfilePosition);
        filePosition += icopyright.size;
    }
//TODO daca size-ul din tuple e 2 (null termination) sa nu mai scriu din tuple ci sa citesc din fisierul initial
    if (description.size != 0) {
        size += writeContentFieldValueToFile(to, description, newfilePosition);
        //skip this on the from file
        vfs_fseek(from, idescription.size, SEEK_CUR);
        filePosition += idescription.size;
    } else if (idescription.size != 0) {
        vfs_fread(idescription.strValue, idescription.size, 1, from);
        size += writeContentFieldValueToFile(to, idescription, newfilePosition);
        filePosition += idescription.size;
    }


    if(irating.size != 0)
    {
        vfs_fread(irating.strValue, irating.size, 1, from);
        size += writeContentFieldValueToFile(to, irating, newfilePosition);
        filePosition += irating.size;
    }



    DEBUG("from pos %d\n", filePosition);
    DEBUG("to pos %d\n", newfilePosition);

    //write the new size
    if (filePosition != newfilePosition) {

        vfs_fseek(to, newfilePosition - size + 16, SEEK_SET);
        vfs_fwrite(&size, 2, 1, to);
        vfs_fseek(to, newfilePosition, SEEK_SET);
    }

}

void writeExtendedContentObj(VFSFile *from, VFSFile *to, Tuple *tuple) {
    guint64 size;
    guint64 newsize;
    guint16 content_count;
    int found_album = 0;
    int found_genre = 0;
    int found_tracknr = 0;
    int i;
    gchar buf[16];

    vfs_fseek(to, newfilePosition, SEEK_SET);
    vfs_fseek(from, filePosition, SEEK_SET);

    /* copy guid */
    vfs_fread(buf, 16, 1, from);
    vfs_fwrite(buf, 16, 1, to);
    /* size  - we will change this later */
    vfs_fread(&size, 8, 1, from);
    filePosition += size;
    vfs_fwrite(&size, 8, 1, to);

    /* content count - we might change this later */
    vfs_fread(&content_count, 2, 1, from);
    vfs_fwrite(&content_count, 2, 1, to);
    //	newfilePosition += 16+8+2;
    newsize = 16 + 8 + 2;

    for (i = 0; i < content_count; i++) {
        guint16 name_len;
        guint16 data_type;

        /* read the name */
        vfs_fread(&name_len, 2, 1, from);
        gunichar2 *name;

        name = g_new0(gunichar2, name_len / sizeof (gunichar2));
        vfs_fread(name, name_len, 1, from);

        gchar *utf8Name = g_utf16_to_utf8(name, name_len, NULL, NULL, NULL);

//        DEBUG("NAME = %s\n", utf8Name);

        if (utf8Name != NULL) {
            if (!strcmp(utf8Name, "WM/Genre")) {
                found_genre = 1;
                /* write the name to the tmp file */
                vfs_fwrite(&name_len, 2, 1, to);
                vfs_fwrite(name, name_len, 1, to);

                /*write the value data type and len we will write */
                /* genre = string */
                data_type = 0;
                vfs_fwrite(&data_type, 2, 1, to);
                //skip 2 in the original file */
                vfs_fseek(from, 2, SEEK_CUR);
                glong genre_tuple_len;
                gunichar2 *genre = g_utf8_to_utf16(tuple_get_string(tuple, FIELD_GENRE, NULL), -1, NULL, &genre_tuple_len, NULL);
                genre_tuple_len *= 2;
                genre_tuple_len += 2;
                vfs_fwrite(&genre_tuple_len, 2, 1, to);
                vfs_fwrite(genre, genre_tuple_len, 1, to);
                guint16 l;
                vfs_fread(&l, 2, 1, from);
                vfs_fseek(from, l, SEEK_CUR);
                filePosition += 4 + l;
                newsize += 2 + name_len + 2 + 2 + genre_tuple_len;
                continue;
            }

            if (!strcmp(utf8Name, "WM/AlbumTitle")) {
                found_album = 1;
                /* write the name to the tmp file */
                vfs_fwrite(&name_len, 2, 1, to);
                vfs_fwrite(name, name_len, 1, to);

                /*write the value data type and len we will write */
                /* album = string */
                data_type = 0;
                vfs_fwrite(&data_type, 2, 1, to);
                //skip 2 in the original file */

                guint16 album_tuple_len;
                glong tl;
                gunichar2 *album = g_utf8_to_utf16(tuple_get_string(tuple, FIELD_ALBUM, NULL), -1, NULL, &tl, NULL);
                album_tuple_len = tl;
                album_tuple_len *= sizeof (gunichar2);
                album_tuple_len += 2;
                vfs_fwrite(&album_tuple_len, 2, 1, to);
                vfs_fwrite(album, album_tuple_len, 1, to);
                vfs_fseek(from, 2, SEEK_CUR);
                guint16 l;
                vfs_fread(&l, 2, 1, from);
                vfs_fseek(from, l, SEEK_CUR);
                filePosition += 4 + l;
                newsize += 2 + name_len + 2 + 2 + album_tuple_len;
                continue;
            }

            if (!strcmp(utf8Name, "WM/TrackNumber")) {
                found_tracknr = 1;
                /* write the name to the tmp file */
                vfs_fwrite(&name_len, 2, 1, to);
                vfs_fwrite(name, name_len, 1, to);

                /*write the value data type and len we will write */
                /* tracknumber  = int */
                data_type = 3;
                vfs_fwrite(&data_type, 2, 1, to);
                //skip 2 in the original file */
                vfs_fseek(from, 2, SEEK_CUR);
                guint16 tracknr_tuple_len = 4;
                guint32 tracknr = tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL);

                vfs_fwrite(&tracknr_tuple_len, 2, 1, to);
                vfs_fwrite(&tracknr, tracknr_tuple_len, 1, to);
                newsize += 2 + name_len + 2 + 2 + tracknr_tuple_len;
                guint16 l;
                vfs_fread(&l, 2, 1, from);
                vfs_fseek(from, l, SEEK_CUR);
                filePosition += 4 + l;
                continue;
            }
        }

        guint16 valueSize;
        DEBUG("copy datA \n");
        vfs_fwrite(&name_len, 2, 1, to);
        vfs_fwrite(name, name_len, 1, to);


        vfs_fread(&data_type, 2, 1, from);
        vfs_fwrite(&data_type, 2, 1, to);


        vfs_fread(&valueSize, 2, 1, from);
        vfs_fwrite(&valueSize, 2, 1, to);

        valueSize = valueSize;
        gchar value[valueSize];
        vfs_fread(value, valueSize, 1, from);
        vfs_fwrite(value, valueSize, 1, to);
        newsize += 2 + name_len + 2 + 2 + valueSize;


    }
    filePosition = vfs_ftell(from);
    newfilePosition += newsize;
}

void writeHeaderExtensionObject(VFSFile *from, VFSFile *to) {

    gchar buf[16];
    guint64 size;
    DEBUG("file position = %d\n", filePosition);
    vfs_fseek(to, newfilePosition, SEEK_SET);
    vfs_fseek(from, filePosition, SEEK_SET);
    /* copy guid */
    vfs_fread(buf, 16, 1, from);
    vfs_fwrite(buf, 16, 1, to);

    /* size  - we will change this later */
    vfs_fread(&size, 8, 1, from);
    vfs_fwrite(&size, 8, 1, to);
    DEBUG("extension size = %"PRId64"\n", size);
    gchar buf2[size];
    vfs_fread(buf2, size, 1, from);
    vfs_fwrite(buf2, size, 1, to);
    newfilePosition += size;
    filePosition += size;
}

/* TODO move this to util since it can be used for all the other formats */
void writeAudioData(VFSFile *from, VFSFile *to) {

    //	vfs_fseek(to,newfilePosition,SEEK_SET);
    while (vfs_feof(from) == 0) {
        gchar buf[4096];
        gint n = vfs_fread(buf, 1, 4096, from);
        vfs_fwrite(buf, n, 1, to);
    }
}

void addContentDescriptionObj(VFSFile *to, Tuple *tuple) {
    guint64 size;

    ContentField title;
    ContentField author;
    ContentField description;
    ContentField copyright;
    ContentField rating;

    title = getStringContentFromTuple(tuple, FIELD_TITLE);
    author = getStringContentFromTuple(tuple, FIELD_ARTIST);
    description = getStringContentFromTuple(tuple, FIELD_COMMENT);
    copyright = getStringContentFromTuple(tuple, FIELD_COPYRIGHT);
    //we dont have rating in tuple so make up a dummy one
    rating.size = 2;

    printContentField(title);
    printContentField(author);
    printContentField(description);
    printContentField(copyright);

    /* write guid and size to file */
    size = 24;
    writeGuidToFile(to, ASF_CONTENT_DESCRIPTION_OBJECT);
    vfs_fwrite(&size, 8, 1, to);

    newfilePosition += 24;


    size += writeContentFieldSizeToFile(to, title, newfilePosition);

    size += writeContentFieldSizeToFile(to, author, newfilePosition);

    /* copyright */
    size += writeContentFieldSizeToFile(to, copyright, newfilePosition);

    size += writeContentFieldSizeToFile(to, description, newfilePosition);

    size += writeContentFieldSizeToFile(to, rating, newfilePosition);


    size += writeContentFieldValueToFile(to, title, newfilePosition);

    if (author.size != 0) {
        size += writeContentFieldValueToFile(to, author, newfilePosition);

    }

    if (copyright.size != 0) {
        size += writeContentFieldValueToFile(to, copyright, newfilePosition);
    }
    if (description.size != 0) {

        printContentField(description);
        size += writeContentFieldValueToFile(to, description, newfilePosition);
    }

    if (rating.size != 0) {
        DEBUG("xx\n");
        size += writeContentFieldValueToFile(to, rating, newfilePosition);
    }
    DEBUG("***\n");

    DEBUG("size %"PRId64"\n", size);
    DEBUG("to pos %d\n", newfilePosition);

    vfs_fseek(to, newfilePosition - size + 16, SEEK_SET);
    vfs_fwrite(&size, 8, 1, to);
    vfs_fseek(to, newfilePosition, SEEK_SET);
    newfilePosition += 24;
}

int writeContentDescriptor(VFSFile*to, gchar* fieldName, const gchar* value) {
    if (value == NULL || fieldName == NULL)
        return 0;
    glong nameSize;
    guint16 nSize;
    guint16 dataType = 0;
    gunichar2 *name = g_utf8_to_utf16(fieldName, -1, NULL, &nameSize, NULL);
    nameSize *= sizeof (gunichar2);
    nameSize += 2;
    nSize = nameSize;
    //write the name size and name

    vfs_fwrite(&nSize, 2, 1, to);
    vfs_fwrite(name, nameSize, 1, to);

    newfilePosition += nameSize + 2;

    //write data type - string in our case
    vfs_fwrite(&dataType, 2, 1, to);

    newfilePosition += 2;

    //write value length
    glong valSize = 0;
    gunichar2 *val = g_utf8_to_utf16(value, -1, NULL, &valSize, NULL);
    valSize *= sizeof (gunichar2);
    valSize += 2;
    guint16 vSize = valSize;
    //write value size
    vfs_fwrite(&vSize, 2, 1, to);
    vfs_fwrite(val, valSize, 1, to);

    newfilePosition += valSize + 2;
    return 0;
}

void addExtendedContentObj(VFSFile *to, Tuple *tuple) {
    guint64 size;
    guint64 newsize;
    guint16 content_count = 3;



    vfs_fseek(to, newfilePosition, SEEK_SET);

    /* write guid and size to file */
    size = 24;
    writeGuidToFile(to, ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT);
    vfs_fwrite(&size, 8, 1, to);

    newfilePosition += 24;


    vfs_fwrite(&content_count, 2, 1, to);
    //	newfilePosition += 16+8+2;
    newsize = 16 + 8 + 2;
    gchar * name = "WM/Genre";

    DEBUG("genre = %s\n", tuple_get_string(tuple, FIELD_GENRE, NULL));

    newsize += writeContentDescriptor(to, name,
            tuple_get_string(tuple, FIELD_GENRE, NULL));

    newsize += writeContentDescriptor(to, "WM/AlbumTitle",
            tuple_get_string(tuple, FIELD_ALBUM, NULL));

    gchar track[3];
    sprintf(track, "%d", tuple_get_int(tuple, FIELD_TRACK_NUMBER, NULL));
    newsize += writeContentDescriptor(to, "WM/TrackNumber", track);

}

gboolean wma_write_tuple_to_file(Tuple* tuple, VFSFile *fd) {
    newfilePosition = 0;
    filePosition = 0;
    VFSFile *tmpFile;
    int foundContentDesc = 0;
    int foundExtendedHeader = 0;
    int HeaderObjNr = 0;
    HeaderObject *newHeader;
    int i;
    vfs_fseek(fd,0,SEEK_SET);
    /* create a temporary file, with random name */
    const gchar *tmpdir = g_get_tmp_dir();
    DEBUG("tmpdir: '%s'\n", tmpdir);
    gchar *tmp_path = g_strdup_printf("file://%s/%s", tmpdir, "wmatmp.wma");


    tmpFile = vfs_fopen(tmp_path, "w+");

    DEBUG("fopen %s\n", tmpfile != NULL ? "success" : "failure");

    newHeader = copyHeaderObject(fd, tmpFile);


    for (i = 0; i < newHeader->objectsNr; i++) {
        GUID *guid = g_new0(GUID, 1);
        memcpy(guid, guid_read_from_file(fd, filePosition), sizeof (GUID));
        int guid_type = get_guid_type(guid);
        DEBUG("guid type = %d\n", guid_type);
        switch (guid_type) {
            case ASF_CONTENT_DESCRIPTION_OBJECT:
            {
                DEBUG("content description\n");
                writeContentDescriptionObject(fd, tmpFile, tuple);
                HeaderObjNr++;
                foundContentDesc = 1;
            }
                break;

            case ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT:
            {
                DEBUG("asf extended content description object\n");
                writeExtendedContentObj(fd, tmpFile, tuple);
                HeaderObjNr++;
                foundExtendedHeader = 1;
            }
                break;
            case ASF_HEADER_EXTENSION_OBJECT:
            {
                DEBUG("header extension \n");
                writeHeaderExtensionObject(fd, tmpFile);
                HeaderObjNr++;
            }
                break;
            default:
            {
                DEBUG("default guid= %d\n", guid_type);
                copyASFObject(fd, tmpFile);
                HeaderObjNr++;
            }
        }
    }

    if (foundContentDesc == 0) {
        DEBUG("Content Description not found\n");
        addContentDescriptionObj(tmpFile, tuple);
        HeaderObjNr++;
    }

    if (foundExtendedHeader == 0) {
        DEBUG("add exteded header \n");
        addExtendedContentObj(tmpFile, tuple);
        HeaderObjNr++;
    }
    /* we must update the total header size and number of objects */
    guint64 total_size = newfilePosition;
    DEBUG("new header %d\n", newfilePosition);
    vfs_fseek(tmpFile, 16, SEEK_SET);
    vfs_fwrite(&total_size, 8, 1, tmpFile);
    newHeader->objectsNr += 1;
    vfs_fwrite(&HeaderObjNr, 4, 1, tmpFile);
    /* go back to the end of file */
    vfs_fseek(tmpFile, newfilePosition, SEEK_SET);
    DEBUG("new header %d\n", newfilePosition);

    /* write the rest of the file */
    writeAudioData(fd, tmpFile);

    gchar *uri = g_strdup(fd -> uri);
    vfs_fclose(tmpFile);
    gchar* f1 = g_filename_from_uri(tmp_path,NULL,NULL);
    gchar* f2 = g_filename_from_uri(uri,NULL,NULL);
    if (g_rename(f1,f2 ) == 0) {
        DEBUG("the tag was updated successfully\n");
    } else {
        DEBUG("an error has occured\n");
    }
    return TRUE;
}
