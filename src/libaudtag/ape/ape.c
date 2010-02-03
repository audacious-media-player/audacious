#include <glib.h>
#include <glib/gstdio.h>
#include "ape.h"
#include "../util.h"
#include <inttypes.h>
#include "../tag_module.h"

#define APE_IDENTIFIER "APETAGEX"

tag_module_t ape = {ape_can_handle_file, ape_populate_tuple_from_file, ape_write_tuple_to_file};

gchar* ape_items[] = {"Album", "Title", "Copyright", "Artist", "Track", "Year", "Genre", "Comment"};

/* reading fucntions */


APEv2Header *readAPEHeader(VFSFile *fd) {
    APEv2Header *header = g_new0(APEv2Header, 1);
    header->preamble = read_char_data(fd, 8);
    header->version = read_LEuint32(fd);
    header->tagSize = read_LEuint32(fd);
    header->itemCount = read_LEuint32(fd);
    header->reserved = read_LEuint64(fd);
    return header;
}

gchar* read_NULLterminatedString(VFSFile *fd) {
    gchar buf[1024];
    gchar ch;
    int size = 0;
    while (1) {
        vfs_fread(&ch, 1, 1, fd);
        if (ch != 0) {
            buf[size] = ch;
            size++;
        } else
            break;
    }    
   gchar* ret = g_strndup(buf, size);

    return ret;
}

gchar* readUTF_8(VFSFile *fd, int size) {
    gchar *buf = g_new0(gchar, size);
    vfs_fread(buf, size, 1, fd);
    return buf;
}

APETagItem *readAPETagItem(VFSFile *fd) {
    APETagItem *tagitem = g_new0(APETagItem, 1);
    tagitem->size = read_LEuint32(fd);
    tagitem->flags = read_LEuint32(fd);
    tagitem->key = read_NULLterminatedString(fd);
    tagitem->value = readUTF_8(fd, tagitem->size);

    return tagitem;
}

//writing functions

void write_tagItemToFile(VFSFile *fd, APETagItem *item) {
    item->size -=1;
    vfs_fwrite(&item->size, 4, 1, fd);
    vfs_fwrite(&item->flags, 4, 1, fd);
    vfs_fwrite(item->key, strlen(item->key) +1, 1, fd);
    vfs_fwrite(item->value, item->size, 1, fd);
}

void write_apeHeaderToFile(VFSFile *fd, APEv2Header *header) {
    vfs_fwrite(header->preamble, 8, 1, fd);
    vfs_fwrite(&header->version, 4, 1, fd);
    vfs_fwrite(&header->tagSize, 4, 1, fd);
    vfs_fwrite(&header->itemCount, 4, 1, fd);
    vfs_fwrite(&header->flags, 4, 1, fd);
    vfs_fwrite(&header->reserved, 8, 1, fd);
}

void write_allTagsToFile(VFSFile *fd, APEv2Header *header) {
    mowgli_node_t *n, *tn;

    MOWGLI_LIST_FOREACH_SAFE(n, tn, tagKeys->head) {
        APETagItem *tagItem = (APETagItem*) mowgli_dictionary_retrieve(tagItems, (gchar*) (n->data));
        write_tagItemToFile(fd, tagItem);
    }
}

APEv2Header *computeNewHeader() {
    guint32 size = 0;
    mowgli_node_t *n, *tn;
    APEv2Header *header = g_new0(APEv2Header, 1);

    MOWGLI_LIST_FOREACH_SAFE(n, tn, tagKeys->head) {
        APETagItem *tagItem = (APETagItem*) mowgli_dictionary_retrieve(tagItems, (gchar*) (n->data));
        size += (tagItem->size + 4 + 4 + strlen(tagItem->key));
    }
    header->flags = 0;
    header->preamble = APE_IDENTIFIER;
    header->reserved = 0;
    header->version = 2000;
    header->itemCount = tagKeys->count;
    header->tagSize = size + 32 ; //(32 - the size of the footer)

    return header;
}

int getTagItemID(APETagItem *tagitem) {
    int i = 0;
    for (i = 0; i < APE_ITEMS_NO; i++) {
        if (!g_utf8_collate(tagitem->key, ape_items[i]))
            return i;
    }
    return -1;
}

gboolean tagContainsHeader(APEv2Header *header) {
    return (header->flags & 0x0001);
}

void add_tagItemFromTuple(Tuple *tuple, int tuple_field, int ape_field) {
    gboolean new = FALSE;
    APETagItem *tagItem = (APETagItem*) mowgli_dictionary_retrieve(tagItems, ape_items[ape_field]);
    if (tagItem == NULL) {
        tagItem = g_new0(APETagItem, 1);
        new = TRUE;
    }
    gchar* value = 0;
    if (tuple_get_value_type(tuple, tuple_field, NULL) == TUPLE_STRING)
        value = g_strdup(tuple_get_string(tuple, tuple_field, NULL));
    if (tuple_get_value_type(tuple, tuple_field, NULL) == TUPLE_INT)
        value = g_strdup_printf("%d", tuple_get_int(tuple, tuple_field, NULL));

    tagItem->flags = 0;
    tagItem->key = g_strdup(ape_items[ape_field]);
    tagItem->value = value;
    tagItem->size = strlen(value) + 1;
    if (new) {
        mowgli_node_add(tagItem->key, mowgli_node_create(), tagKeys);
        mowgli_dictionary_add(tagItems, tagItem->key, tagItem);
    }
}



gboolean ape_can_handle_file(VFSFile *f) {
    APEv2Header *header = readAPEHeader(f);
    if (!strcmp(header->preamble, APE_IDENTIFIER))
        return TRUE;
    else {
        //maybe the tag is at the end of the file
        guint64 filesize = vfs_fsize(f);
        vfs_fseek(f, filesize-32, SEEK_SET);
        APEv2Header *header = readAPEHeader(f);
        if (!strcmp(header->preamble, APE_IDENTIFIER))
            return TRUE;
    }
    return FALSE;
}

Tuple *ape_populate_tuple_from_file(Tuple *tuple, VFSFile *f) {
    int i;
    vfs_fseek(f, 0, SEEK_SET);
    headerPosition = 0;
    guint64 filesize = vfs_fsize(f);
    APEv2Header *header = readAPEHeader(f);
    if (strcmp(header->preamble, APE_IDENTIFIER)) {
        g_free(header);
        //maybe the tag is at the end of the file
        vfs_fseek(f, filesize-32, SEEK_SET);
        header = readAPEHeader(f);
        if (!strcmp(header->preamble, APE_IDENTIFIER)) {
            //read all the items from the end of the file
            //   if(tagContainsHeader(header))
            gchar* path = g_strdup(f->uri);
            vfs_fclose(f);
            f = vfs_fopen(path,"r");
            long offset = filesize - (header->tagSize);
            vfs_fseek(f, offset, SEEK_SET);
            headerPosition = vfs_ftell(f);
            //   else vfs_fseek(f,-(header->tagSize),SEEK_END);
        } else
            return NULL;
    } else
        return NULL;
    if(tagKeys != NULL)
    {
        mowgli_node_t *n, *tn;
        MOWGLI_LIST_FOREACH_SAFE(n, tn, tagKeys->head)
        {
            mowgli_node_delete(n,tagKeys);
        }
    }
    tagKeys = mowgli_list_create();
    tagItems = mowgli_dictionary_create(strcasecmp);
    for (i = 0; i < header->itemCount; i++) {
        APETagItem *tagitem = readAPETagItem(f);
        int tagid = getTagItemID(tagitem);
        mowgli_node_add(tagitem->key, mowgli_node_create(), tagKeys);
        mowgli_dictionary_add(tagItems, tagitem->key, tagitem);

        switch (tagid) {
            case APE_ALBUM:
            {
                tuple_associate_string(tuple, FIELD_ALBUM, NULL, tagitem->value);
            }
            break;
            case APE_TITLE:
            {
                tuple_associate_string(tuple, FIELD_TITLE, NULL, tagitem->value);
            }
            break;
            case APE_COPYRIGHT:
            {
                tuple_associate_string(tuple, FIELD_COPYRIGHT, NULL, tagitem->value);

            }
            break;
            case APE_ARTIST:
            {
                tuple_associate_string(tuple, FIELD_ARTIST, NULL, tagitem->value);
            }
            break;
            case APE_TRACKNR:
            {
                tuple_associate_int(tuple, FIELD_TRACK_NUMBER, NULL, atoi(tagitem->value));
            }
            break;
            case APE_YEAR:
            {
                tuple_associate_int(tuple, FIELD_YEAR, NULL, atoi(tagitem->value));
            }
            break;
            case APE_GENRE:
            {
                tuple_associate_string(tuple, FIELD_GENRE, NULL, tagitem->value);
            }
            break;
            case APE_COMMENT:
            {
                tuple_associate_string(tuple, FIELD_COMMENT, NULL, tagitem->value);
            }
            break;
        }
    }
    return tuple;
}

gboolean ape_write_tuple_to_file(Tuple* tuple, VFSFile *f)
{
    VFSFile *tmp;
    const gchar *tmpdir = g_get_tmp_dir();
    gchar *tmp_path = g_strdup_printf("file://%s/%s", tmpdir, "tmp.mpc");
    tmp = vfs_fopen(tmp_path, "w+");


    if (tuple_get_string(tuple, FIELD_ARTIST, NULL))
        add_tagItemFromTuple(tuple, FIELD_ARTIST, APE_ARTIST);

    if (tuple_get_string(tuple, FIELD_TITLE, NULL))
        add_tagItemFromTuple(tuple, FIELD_TITLE, APE_TITLE);


    if (tuple_get_string(tuple, FIELD_ALBUM, NULL))
        add_tagItemFromTuple(tuple, FIELD_ALBUM, APE_ALBUM);

    if (tuple_get_string(tuple, FIELD_COMMENT, NULL))
        add_tagItemFromTuple(tuple, FIELD_COMMENT, APE_COMMENT);

    if (tuple_get_string(tuple, FIELD_GENRE, NULL))
        add_tagItemFromTuple(tuple, FIELD_GENRE, APE_GENRE);

    if (tuple_get_int(tuple, FIELD_YEAR, NULL) != -1)
        add_tagItemFromTuple(tuple, FIELD_YEAR, APE_YEAR);

    if(tuple_get_int(tuple,FIELD_TRACK_NUMBER,NULL) != -1)
        add_tagItemFromTuple(tuple, FIELD_TRACK_NUMBER, APE_TRACKNR);

    copyAudioData(f,tmp,0,headerPosition);

    APEv2Header *header = computeNewHeader();
    write_apeHeaderToFile(tmp, header);

    write_allTagsToFile(tmp, header);
    write_apeHeaderToFile(tmp, header);


    gchar *uri = g_strdup(f -> uri);
    vfs_fclose(tmp);
    gchar* f1 = g_filename_from_uri(tmp_path,NULL,NULL);
    gchar* f2 = g_filename_from_uri(uri,NULL,NULL);
    if (g_rename(f1,f2 ) == 0) {
        AUDDBG("the tag was updated successfully\n");
    } else {
        AUDDBG("an error has occured\n");
    }
    //skip the tag at the end - add the new tag
    return TRUE;
}
