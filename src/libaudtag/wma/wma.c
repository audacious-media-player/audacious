/*
 * Copyright 2009 Paula Stanciu
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include <inttypes.h>
#include <glib-2.0/glib/gstdio.h>
#include <libaudcore/tuple.h>

#include "guid.h"
#include "wma.h"
#include "wma_fmt.h"
#include "module.h"
#include "../util.h"

/* static functions */
static GenericHeader *read_generic_header(VFSFile * f, gboolean read_data)
{
    TAGDBG("read top-level header object\n");
    g_return_val_if_fail((f != NULL), NULL);
    GenericHeader *header = g_new0(GenericHeader, 1);
    header->guid = guid_read_from_file(f);
    header->size = read_LEuint64(f);
    if (read_data)
        header->data = (gchar *) read_char_data(f, header->size);
    else
        header->data = NULL;

    gchar *s = guid_convert_to_string(header->guid);
    TAGDBG("read GUID: %s\n", s);
    g_free(s);

    return header;
}

static HeaderObj *read_top_header_object(VFSFile * f)
{
    TAGDBG("read top-level header object\n");
    g_return_val_if_fail((f != NULL), NULL);
    HeaderObj *header = g_new0(HeaderObj, 1);

    //we have already read the GUID so we skip it (16 bytes)
    vfs_fseek(f, 16, SEEK_SET);

    header->size = read_LEuint64(f);
    header->objectsNr = read_LEuint32(f);
    TAGDBG("Number of child objects: %d\n", header->objectsNr);

    header->res1 = read_uint8(f);
    header->res2 = read_uint8(f);

    if ((header->size == -1) || (header->objectsNr == -1) || (header->res2 != 2))
    {
        g_free(header);
        return NULL;
    }
    return header;
}

static ContentDescrObj *read_content_descr_obj(VFSFile * f)
{
    ContentDescrObj *cdo = g_new0(ContentDescrObj, 1);
    cdo->guid = guid_read_from_file(f);
    cdo->size = read_LEuint64(f);
    cdo->title_length = read_LEuint16(f);
    cdo->author_length = read_LEuint16(f);
    cdo->copyright_length = read_LEuint16(f);
    cdo->desc_length = read_LEuint16(f);
    cdo->rating_length = read_LEuint16(f);
    cdo->title = fread_utf16(f, cdo->title_length);
    cdo->author = fread_utf16(f, cdo->author_length);
    cdo->copyright = fread_utf16(f, cdo->copyright_length);
    cdo->description = fread_utf16(f, cdo->desc_length);
    cdo->rating = fread_utf16(f, cdo->rating_length);
    return cdo;
}

static ExtContentDescrObj *read_ext_content_descr_obj(VFSFile * f, Tuple * t, gboolean populate_tuple)
{
    ExtContentDescrObj *ecdo = g_new0(ExtContentDescrObj, 1);
    ecdo->guid = guid_read_from_file(f);
    ecdo->size = read_LEuint64(f);
    ecdo->content_desc_count = read_LEuint16(f);
    ecdo->descriptors = read_descriptors(f, ecdo->content_desc_count, t, populate_tuple);
    return ecdo;
}

static guint find_descriptor_id(gchar * s)
{
    TAGDBG("finding descriptor id for '%s'\n", s);
    g_return_val_if_fail(s != NULL, -1);
    gchar *l[DESC_LAST] = { DESC_ALBUM_STR, DESC_YEAR_STR, DESC_GENRE_STR, DESC_TRACK_STR };
    guint i;
    for (i = 0; i < DESC_LAST; i++)
        if (!strcmp(l[i], s))
        {
            TAGDBG("found descriptor %s\n", s);
            return i;
        }
    return -1;
}

static ContentDescriptor *read_descriptor(VFSFile * f, Tuple * t, gboolean populate_tuple)
{
    ContentDescriptor *cd = g_new0(ContentDescriptor, 1);
    gchar *val = NULL, *name = NULL;
    guint32 intval = -1;
    gint dtype;
    TAGDBG("reading name_len\n");
    cd->name_len = read_LEuint16(f);
    TAGDBG("reading name\n");
    cd->name = fread_utf16(f, cd->name_len);
    TAGDBG("reading val_type\n");
    cd->val_type = read_LEuint16(f);
    TAGDBG("reading val_len\n");
    cd->val_len = read_LEuint16(f);

    name = utf8(cd->name);
    dtype = find_descriptor_id(name);
    g_free(name);

    TAGDBG("reading val\n");

    if (populate_tuple)
    {
        /*we only parse int's and UTF strings, everything else is handled as raw data */
        if (cd->val_type == 0)
        {                       /*UTF16 */
            cd->val = read_char_data(f, cd->val_len);
            val = utf8((gunichar2 *) cd->val);
            TAGDBG("val: '%s' dtype: %d\n", val, dtype);
            if (dtype == DESC_ALBUM)
                tuple_associate_string(t, FIELD_ALBUM, NULL, val);
            if (dtype == DESC_GENRE)
                tuple_associate_string(t, FIELD_GENRE, NULL, val);
            if (dtype == DESC_TRACK)
                tuple_associate_int(t, FIELD_TRACK_NUMBER, NULL, atoi(val));
            if (dtype == DESC_YEAR)
                tuple_associate_int(t, FIELD_YEAR, NULL, atoi(val));
        }
        else
        {
            if (cd->val_type == 3)
            {
                intval = read_LEuint32(f);
                TAGDBG("intval: %d, dtype: %d\n", intval, dtype);
                if (dtype == DESC_TRACK)
                    tuple_associate_int(t, FIELD_TRACK_NUMBER, NULL, intval);
            }
            else
                cd->val = read_char_data(f, cd->val_len);
        }
    }
    else
        cd->val = read_char_data(f, cd->val_len);
    TAGDBG("read str_val: '%s', intval: %d\n", val, intval);
    TAGDBG("exiting read_descriptor \n\n");
    return cd;
}

static ContentDescriptor **read_descriptors(VFSFile * f, guint count, Tuple * t, gboolean populate_tuple)
{
    if (count == 0)
        return NULL;
    ContentDescriptor **descs = g_new0(ContentDescriptor *, count);
    int i;
    for (i = 0; i < count; i++)
        descs[i] = read_descriptor(f, t, populate_tuple);
    return descs;
}

void free_content_descr_obj(ContentDescrObj * c)
{
    g_free(c->guid);
    g_free(c->title);
    g_free(c->author);
    g_free(c->copyright);
    g_free(c->description);
    g_free(c->rating);
    g_free(c);
}

void free_content_descr(ContentDescriptor * cd)
{
    g_free(cd->name);
    g_free(cd->val);
    g_free(cd);
}

void free_ext_content_descr_obj(ExtContentDescrObj * ecdo)
{
    int i;
    g_free(ecdo->guid);
    for (i = 0; i < ecdo->content_desc_count; i++)
        free_content_descr(ecdo->descriptors[i]);
    g_free(ecdo);
}

/* returns the offset of the object in the file */
static long ftell_object_by_guid(VFSFile * f, GUID_t * g)
{
    TAGDBG("seeking object %s, with ID %d \n", guid_convert_to_string(g), get_guid_type(g));
    HeaderObj *h = read_top_header_object(f);
    g_return_val_if_fail((f != NULL) && (g != NULL) && (h != NULL), -1);

    gint i = 0;
    while (i < h->objectsNr)
    {
        GenericHeader *gen_hdr = read_generic_header(f, FALSE);
        TAGDBG("encountered GUID %s, with ID %d\n", guid_convert_to_string(gen_hdr->guid), get_guid_type(gen_hdr->guid));
        if (guid_equal(gen_hdr->guid, g))
        {
            g_free(h);
            g_free(gen_hdr);
            guint64 ret = vfs_ftell(f) - 24;
            TAGDBG("at offset %" PRIx64 "\n", ret);
            return ret;
        }
        vfs_fseek(f, gen_hdr->size - 24, SEEK_CUR);     //most headers have a size as their second field"
        i++;
    }
    TAGDBG("The object was not found\n");

    g_free(h);
    return -1;
}

VFSFile *make_temp_file()
{
    /* create a temporary file */
    const gchar *tmpdir = g_get_tmp_dir();
    gchar *tmp_path = g_strdup_printf("file://%s/%s", tmpdir, "wmatmp.wma");
    return vfs_fopen(tmp_path, "w+");
}

long ftell_object_by_str(VFSFile * f, gchar * s)
{
    GUID_t *g = guid_convert_from_string(s);
    long res = ftell_object_by_guid(f, g);
    g_free(g);
    return res;
}

static void write_content_descr_obj_from_tuple(VFSFile * f, ContentDescrObj * cdo, Tuple * t)
{
    glong size;
    gboolean free_cdo = FALSE;
    if (cdo == NULL)
    {
        cdo = g_new0(ContentDescrObj, 1);
        free_cdo = TRUE;
    }

    cdo->title = g_utf8_to_utf16(tuple_get_string(t, FIELD_TITLE, NULL), -1, NULL, &size, NULL);
    cdo->title_length = 2 * (size + 1);
    cdo->author = g_utf8_to_utf16(tuple_get_string(t, FIELD_ARTIST, NULL), -1, NULL, &size, NULL);
    cdo->author_length = 2 * (size + 1);
    cdo->copyright = g_utf8_to_utf16(tuple_get_string(t, FIELD_COPYRIGHT, NULL), -1, NULL, &size, NULL);
    cdo->copyright_length = 2 * (size + 1);
    cdo->description = g_utf8_to_utf16(tuple_get_string(t, FIELD_COMMENT, NULL), -1, NULL, &size, NULL);
    cdo->desc_length = 2 * (size + 1);
    cdo->size = 34 + cdo->title_length + cdo->author_length + cdo->copyright_length + cdo->desc_length;
    guid_write_to_file(f, ASF_CONTENT_DESCRIPTION_OBJECT);
    write_LEuint64(f, cdo->size);
    write_LEuint16(f, cdo->title_length);
    write_LEuint16(f, cdo->author_length);
    write_LEuint16(f, cdo->copyright_length);
    write_LEuint16(f, cdo->desc_length);
    write_LEuint16(f, 2);       // rating_length = 2
    write_utf16(f, cdo->title, cdo->title_length);
    write_utf16(f, cdo->title, cdo->title_length);
    write_utf16(f, cdo->author, cdo->author_length);
    write_utf16(f, cdo->copyright, cdo->copyright_length);
    write_utf16(f, cdo->description, cdo->desc_length);
    write_utf16(f, NULL, 2);    //rating == NULL

    if (free_cdo)
        free_content_descr_obj(cdo);
}

static void write_ext_content_descr_obj_from_tuple(VFSFile * f, ExtContentDescrObj * ecdo, Tuple * tuple)
{
}

static gboolean write_generic_header(VFSFile * f, GenericHeader * gh)
{
    TAGDBG("Writing generic header\n");
    guid_write_to_file(f, get_guid_type(gh->guid));
    return write_char_data(f, gh->data, gh->size);
}

static void free_generic_header(GenericHeader * gh)
{
    g_free(gh->guid);
    g_free(gh->data);
    g_free(gh);
}

static gboolean write_top_header_object(VFSFile * f, HeaderObj * header)
{
    TAGDBG("write header object\n");
    vfs_fseek(f, 0, SEEK_SET);
    return (guid_write_to_file(f, ASF_HEADER_OBJECT) && write_LEuint64(f, header->size) && write_LEuint32(f, header->objectsNr) && write_uint8(f, header->res1) &&      /* the reserved fields */
            write_uint8(f, header->res2));
}

/* interface functions */
gboolean wma_can_handle_file(VFSFile * f)
{
    GUID_t *guid1 = guid_read_from_file(f);
    GUID_t *guid2 = guid_convert_from_string(ASF_HEADER_OBJECT_GUID);
    gboolean equal = ((guid1 != NULL) && guid_equal(guid1, guid2));

    g_free(guid1);
    g_free(guid2);

    return equal;
}

gboolean wma_read_tag (Tuple * t, VFSFile * f)
{
    gchar *artist = NULL, *title = NULL, *comment = NULL;

    print_tuple(t);
    gint seek_res = vfs_fseek(f, ftell_object_by_str(f, ASF_CONTENT_DESCRIPTION_OBJECT_GUID), SEEK_SET);
    if (seek_res == 0)
    {                           //if the CONTENT_DESCRIPTION_OBJECT was found
        ContentDescrObj *cdo = read_content_descr_obj(f);
        artist = utf8(cdo->author);
        title = utf8(cdo->title);
        comment = utf8(cdo->description);
        free_content_descr_obj(cdo);
        tuple_associate_string(t, FIELD_ARTIST, NULL, artist);
        tuple_associate_string(t, FIELD_TITLE, NULL, title);
        tuple_associate_string(t, FIELD_COMMENT, NULL, comment);
    }
    seek_res = vfs_fseek(f, ftell_object_by_str(f, ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT_GUID), SEEK_SET);
    /*this populates the tuple with the attributes stored as extended descriptors */
    ExtContentDescrObj *ecdo = read_ext_content_descr_obj(f, t, TRUE);
    free_ext_content_descr_obj(ecdo);
    print_tuple(t);
    return TRUE;
}

gboolean wma_write_tag (Tuple * tuple, VFSFile * f)
{
#if BROKEN
    return FALSE;
#endif

    HeaderObj *top_ho = read_top_header_object(f);
    VFSFile *tmpfile = make_temp_file();
    gint i, cdo_pos, ecdo_pos;
    GUID_t *g;
    /*read all the headers and write them to the new file */
    /*the headers that contain tuple data will be overwritten */
    TAGDBG("Header Object size: %" PRId64 "\n", top_ho->size);
    //vfs_fseek(tmpfile, )
    for (i = 0; i < top_ho->objectsNr; i++)
    {
        GenericHeader *gh = read_generic_header(f, TRUE);
        g = guid_convert_from_string(ASF_CONTENT_DESCRIPTION_OBJECT_GUID);
        if (guid_equal(gh->guid, g))
        {
            write_content_descr_obj_from_tuple(tmpfile, (ContentDescrObj *) gh, tuple);
            g_free(g);
        }
        else
        {
            g = guid_convert_from_string(ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT_GUID);
            if (guid_equal(gh->guid, g))
                write_ext_content_descr_obj_from_tuple(tmpfile, (ExtContentDescrObj *) gh, tuple);
            else
            {
                write_generic_header(tmpfile, gh);
                g_free(g);
            }
        }
        free_generic_header(gh);
    }
    /*check wether these headers existed in the  original file */
    cdo_pos = ftell_object_by_str(f, ASF_CONTENT_DESCRIPTION_OBJECT_GUID);
    ecdo_pos = ftell_object_by_str(f, ASF_EXTENDED_CONTENT_DESCRIPTION_OBJECT_GUID);
    if (cdo_pos == -1)
    {
        write_content_descr_obj_from_tuple(tmpfile, NULL, tuple);
        top_ho->objectsNr++;
    }
    if (ecdo_pos != -1)
    {
        write_ext_content_descr_obj_from_tuple(tmpfile, NULL, tuple);
        top_ho->objectsNr++;
    }
    write_top_header_object(tmpfile, top_ho);

    gchar *f1 = g_filename_from_uri(tmpfile->uri, NULL, NULL);
    gchar *f2 = g_filename_from_uri(f->uri, NULL, NULL);
    vfs_fclose(tmpfile);
    /*
       if (g_rename(f1, f2) == 0)
       {
       TAGDBG("the tag was updated successfully\n");
       }
       else
       {
       TAGDBG("an error has occured\n");
       }
     */
    g_free(f1);
    g_free(f2);

    return TRUE;
}

tag_module_t wma = {
    .name = "WMA",
    .can_handle_file = wma_can_handle_file,
    .read_tag = wma_read_tag,
    .write_tag = wma_write_tag,
};

