/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "xml_document.h"

#include <glib.h>
#include <string.h>

/* document builder callbacks */

static void bmp_xml_doc_build_start_element(GMarkupParseContext * context,
                                            const gchar * element_name,
                                            const gchar ** attrib_names,
                                            const gchar ** attrib_values,
                                            gpointer user_data,
                                            GError ** error);

static void bmp_xml_doc_build_end_element(GMarkupParseContext * context,
                                          const gchar * element_name,
                                          gpointer user_data,
                                          GError ** error);

static void bmp_xml_doc_build_text(GMarkupParseContext * context,
                                   const gchar * text,
                                   gsize text_len,
                                   gpointer user_data,
                                   GError ** error);

static void bmp_xml_doc_build_ignore(GMarkupParseContext * context,
                                     const gchar * text,
                                     gsize text_len,
                                     gpointer user_data,
                                     GError ** error);

static void bmp_xml_doc_build_error(GMarkupParseContext * context,
                                    GError * error,
                                    gpointer user_data);

static void bmp_xml_doc_build_destroy(BmpXmlDocument * data);

static GMarkupParser bmp_xml_doc_builder = {
    bmp_xml_doc_build_start_element,
    bmp_xml_doc_build_end_element,
    bmp_xml_doc_build_text,
    bmp_xml_doc_build_ignore,
    bmp_xml_doc_build_error
};

static GDestroyNotify bmp_xml_node_data_free_func[] = {
    (GDestroyNotify) bmp_xml_doc_node_data_free,
    (GDestroyNotify) bmp_xml_element_node_data_free,
    (GDestroyNotify) bmp_xml_attrib_node_data_free,
    (GDestroyNotify) bmp_xml_text_node_data_free
};

GNode *
bmp_xml_doc_node_new(void)
{
    BmpXmlDocNodeData *data;
    data = g_new0(BmpXmlDocNodeData, 1);
    data->type = BMP_XML_NODE_DOC;
    return g_node_new(data);
}

void
bmp_xml_doc_node_data_free(BmpXmlDocNodeData * data)
{
    g_return_if_fail(data != NULL);
    g_free(data);
}

GNode *
bmp_xml_element_node_new(const gchar * name)
{
    BmpXmlElementNodeData *data;
    data = g_new0(BmpXmlElementNodeData, 1);
    data->type = BMP_XML_NODE_ELEMENT;
    data->name = g_strdup(name);
    return g_node_new(data);
}

void
bmp_xml_element_node_data_free(BmpXmlElementNodeData * data)
{
    g_return_if_fail(data != NULL);
    g_free(data->name);
    g_free(data);
}

GNode *
bmp_xml_attrib_node_new(const gchar * name,
                        const gchar * value)
{
    BmpXmlAttribNodeData *data;
    data = g_new0(BmpXmlAttribNodeData, 1);
    data->type = BMP_XML_NODE_ATTRIB;
    data->name = g_strdup(name);
    data->value = g_strdup(value);
    return g_node_new(data);
}

void
bmp_xml_attrib_node_data_free(BmpXmlAttribNodeData * data)
{
    g_assert(data != NULL);
    g_free(data->name);
    g_free(data->value);
    g_free(data);
}

GNode *
bmp_xml_text_node_new(const gchar * text, gsize length)
{
    BmpXmlTextNodeData *data;
    data = g_new0(BmpXmlTextNodeData, 1);
    data->type = BMP_XML_NODE_TEXT;
    data->text = g_new0(gchar, length);
    memcpy(data->text, text, length);
    data->length = length;
    return g_node_new(data);
}

void
bmp_xml_text_node_data_free(BmpXmlTextNodeData * data)
{
    g_return_if_fail(data != NULL);
    g_free(data->text);
    g_free(data);
}

void
bmp_xml_node_data_free(GNode * node)
{
    BmpXmlNodeData *data;

    g_return_if_fail(node != NULL);
    g_return_if_fail(node->data != NULL);

    data = BMP_XML_NODE_DATA(node->data);
    (*bmp_xml_node_data_free_func[data->type]) (data);
}

BmpXmlDocument *
bmp_xml_document_new(void)
{
    BmpXmlDocument *document;

    document = g_new0(BmpXmlDocument, 1);

    document->parse_context =
        g_markup_parse_context_new(&bmp_xml_doc_builder, 0,
                                   document, (GDestroyNotify)
                                   bmp_xml_doc_build_destroy);
    document->current_depth = 0;

    document->tree = bmp_xml_doc_node_new();
    document->current_node = document->tree;

    return document;
}

void
bmp_xml_document_free(BmpXmlDocument * document)
{
    g_return_if_fail(document != NULL);

    g_node_traverse(document->tree, G_IN_ORDER, G_TRAVERSE_ALL, -1,
                    (GNodeTraverseFunc) bmp_xml_node_data_free, NULL);
    g_node_destroy(document->tree);

    g_free(document);
}

GNode *
bmp_xml_document_get_tree(BmpXmlDocument * document)
{
    return document->tree;
}

gboolean
bmp_xml_document_load(BmpXmlDocument ** document_ref,
                      const gchar * filename, GError ** error_out)
{
    BmpXmlDocument *document;
    gchar *buffer;
    gsize buffer_size;
    GError *error = NULL;

    g_assert(document_ref != NULL);
    g_assert(filename != NULL);

    *document_ref = NULL;

    if (!g_file_get_contents(filename, &buffer, &buffer_size, &error)) {
        g_propagate_error(error_out, error);
        return FALSE;
    }

    if (!(document = bmp_xml_document_new())) {
        g_free(buffer);
        return FALSE;
    }

    if (!g_markup_parse_context_parse(document->parse_context, buffer,
                                      buffer_size, &error)) {
        bmp_xml_document_free(document);
        g_free(buffer);
        g_propagate_error(error_out, error);
        return FALSE;
    }

    g_free(buffer);

    if (!g_markup_parse_context_end_parse(document->parse_context, &error)) {
        bmp_xml_document_free(document);
        g_propagate_error(error_out, error);
        return FALSE;
    }

    *document_ref = document;

    return TRUE;
}


static void
bmp_xml_doc_build_start_element(GMarkupParseContext * context,
                                const gchar * element_name,
                                const gchar ** attrib_names,
                                const gchar ** attrib_values,
                                gpointer user_data,
                                GError ** error)
{
    BmpXmlDocument *document;

    document = BMP_XML_DOCUMENT(user_data);

    document->current_node =
        g_node_append(document->current_node,
                      bmp_xml_element_node_new(element_name));

    while (*attrib_names) {
	g_print("%s = %s\n", *attrib_names, *attrib_values);
        g_node_append(document->current_node,
                      bmp_xml_attrib_node_new(*attrib_names++,
                                              *attrib_values++));
    }

    document->current_depth++;
}

static void
bmp_xml_doc_build_end_element(GMarkupParseContext * context,
                              const gchar * element_name,
                              gpointer user_data,
                              GError ** error)
{
    BmpXmlDocument *document;

    document = BMP_XML_DOCUMENT(user_data);
    document->current_node = document->current_node->parent;
    document->current_depth--;
}

static void
bmp_xml_doc_build_text(GMarkupParseContext * context,
                       const gchar * text,
                       gsize text_len,
                       gpointer user_data,
                       GError ** error)
{
    BmpXmlDocument *document;
    document = BMP_XML_DOCUMENT(user_data);
    g_node_append(document->current_node,
                  bmp_xml_text_node_new(text, text_len));
}

static void
bmp_xml_doc_build_ignore(GMarkupParseContext * context,
                         const gchar * text,
                         gsize text_len,
                         gpointer user_data,
                         GError ** error)
{
}

static void
bmp_xml_doc_build_error(GMarkupParseContext * context,
                        GError * error,
                        gpointer user_data)
{
}

static void
bmp_xml_doc_build_destroy(BmpXmlDocument * document)
{
    g_markup_parse_context_free(document->parse_context);
}
