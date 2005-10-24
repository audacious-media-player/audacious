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

#ifndef XML_DOCUMENT_H
#define XML_DOCUMENT_H

#include <glib.h>

typedef enum {
    BMP_XML_NODE_DOC = 0,
    BMP_XML_NODE_ELEMENT,
    BMP_XML_NODE_ATTRIB,
    BMP_XML_NODE_TEXT
} BmpXmlNodeType;

#define BMP_XML_DOCUMENT(x)           ((BmpXmlDocument *)(x))
typedef struct {
    GNode *tree;
    GNode *current_node;
    guint current_depth;
    GMarkupParseContext *parse_context;
} BmpXmlDocument;

#define BMP_XML_NODE_DATA(x)          ((BmpXmlNodeData *)(x))
typedef struct {
    BmpXmlNodeType type;
} BmpXmlNodeData;

#define BMP_XML_DOC_NODE_DATA(x)      ((BmpXmlDocNodeData *)(x))
typedef struct {
    BmpXmlNodeType type;
} BmpXmlDocNodeData;

#define BMP_XML_ELEMENT_NODE_DATA(x)  ((BmpXmlElementNodeData *)(x))
typedef struct {
    BmpXmlNodeType type;
    gchar *name;
} BmpXmlElementNodeData;

#define BMP_XML_ATTRIB_NODE_DATA(x)   ((BmpXmlAttribNodeData *)(x))
typedef struct {
    BmpXmlNodeType type;
    gchar *name;
    gchar *value;
} BmpXmlAttribNodeData;

#define BMP_XML_TEXT_NODE_DATA(x)     ((BmpXmlTextNodeData *)(x))
typedef struct {
    BmpXmlNodeType type;
    gchar *text;
    gsize length;
} BmpXmlTextNodeData;


GNode *bmp_xml_doc_node_new(void);
void bmp_xml_doc_node_data_free(BmpXmlDocNodeData * data);

GNode *bmp_xml_element_node_new(const gchar * name);
void bmp_xml_element_node_data_free(BmpXmlElementNodeData * data);

GNode *bmp_xml_attrib_node_new(const gchar * name, const gchar * value);
void bmp_xml_attrib_node_data_free(BmpXmlAttribNodeData * data);

void bmp_xml_text_node_data_free(BmpXmlTextNodeData * data);
GNode *bmp_xml_text_node_new(const gchar * text, gsize length);


gboolean bmp_xml_document_load(BmpXmlDocument ** document,
                               const gchar * filename, GError ** error);

void bmp_xml_document_free(BmpXmlDocument * document);

GNode *bmp_xml_document_get_tree(BmpXmlDocument * document);

#endif
