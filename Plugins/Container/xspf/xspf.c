/*
 * Audacious: A cross-platform multimedia player
 * Copyright (c) 2006 William Pitcock, Tony Vroon, George Averill,
 *                    Giacomo Lozito, Derek Pomery and Yoshiki Yazawa.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <glib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include "audacious/main.h"
#include "audacious/util.h"
#include "audacious/playlist.h"
#include "audacious/playlist_container.h"
#include "audacious/plugin.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

/* Borrowed from BMPx. */
static int
register_namespaces (xmlXPathContextPtr xpathCtx, const xmlChar* nsList)
{
    xmlChar* nsListDup;
    xmlChar* prefix;
    xmlChar* href;
    xmlChar* next;

    nsListDup = xmlStrdup(nsList);
    if(nsListDup == NULL)
    {
	g_warning (G_STRLOC ": unable to strdup namespaces list");
	return(-1);
    }

    next = nsListDup;
    while (next != NULL)
    {
	while((*next) == ' ') next++;
	if((*next) == '\0') break;

	prefix = next;
	next = (xmlChar*)xmlStrchr(next, '=');

	if (next == NULL)
	{
	    g_warning (G_STRLOC ": invalid namespaces list format");
  	    xmlFree (nsListDup);
            return(-1);
        }
        *(next++) = '\0';

        href = next;
        next = (xmlChar*)xmlStrchr(next, ' ');
        if (next != NULL)
        {
            *(next++) = '\0';
        }

        // do register namespace
        if(xmlXPathRegisterNs(xpathCtx, prefix, href) != 0)
        {
            g_warning (G_STRLOC ": unable to register NS with prefix=\"%s\" and href=\"%s\"", prefix, href);
            xmlFree(nsListDup);
            return(-1);
        }
    }

    xmlFree(nsListDup);
    return(0);
}

/* Also borrowed from BMPx. */
xmlXPathObjectPtr
xml_execute_xpath_expression (xmlDocPtr doc,
                              const xmlChar * xpathExpr,
                              const xmlChar * nsList)
{
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;

    /*
     * Create xpath evaluation context
     */
    xpathCtx = xmlXPathNewContext (doc);
    if (xpathCtx == NULL)
    {
        return NULL;
    }

    if (nsList)
        register_namespaces (xpathCtx, nsList);

    /*
     * Evaluate xpath expression
     */
    xpathObj = xmlXPathEvalExpression (xpathExpr, xpathCtx);
    if (xpathObj == NULL)
    {
        xmlXPathFreeContext (xpathCtx);
        return NULL;
    }

    /*
     * Cleanup
     */
    xmlXPathFreeContext (xpathCtx);

    return xpathObj;
}

static void
playlist_load_xspf(const gchar * filename, gint pos)
{
	xmlDocPtr	  doc;
	xmlXPathObjectPtr xpathObj;
	xmlNodeSetPtr     n;
	gint i;

	g_return_if_fail(filename != NULL);

	doc = xmlParseFile(filename);
	if (doc == NULL)
		return;

	/* TODO: what about xspf:artist, xspf:title, xspf:length? */
	xpathObj = xml_execute_xpath_expression(doc, "//xspf:location", "xspf=http://xspf.org/ns/0/");
	if (xpathObj == NULL)
		return;

	n = xpathObj->nodesetval;
	if (n == NULL)
		return;

	for (i = 0; i < n->nodeNr; i++)
	{
		char *uri = XML_GET_CONTENT(n->nodeTab[i]->children);
		++pos;
		playlist_ins(uri, pos);
		g_free(uri);
	}

	xmlXPathFreeObject(xpathObj);
}

#define XSPF_ROOT_NODE_NAME "playlist"
#define XSPF_XMLNS "http://xspf.org/ns/0/"

static void
playlist_save_xspf(const gchar *filename, gint pos)
{
	xmlDocPtr doc;
	xmlNodePtr rootnode, tmp, tracklist;
	GList *node;

	doc = xmlNewDoc("1.0");

	rootnode = xmlNewNode(NULL, XSPF_ROOT_NODE_NAME);
	xmlSetProp(rootnode, "xmlns", XSPF_XMLNS);
	xmlSetProp(rootnode, "version", "1");
	xmlDocSetRootElement(doc, rootnode);

	tmp = xmlNewNode(NULL, "creator");
	xmlAddChild(tmp, xmlNewText(PACKAGE "-" VERSION));
	xmlAddChild(rootnode, tmp);

	tracklist = xmlNewNode(NULL, "trackList");
	xmlAddChild(rootnode, tracklist);

	for (node = playlist_get(); node != NULL; node = g_list_next(node))
	{
		PlaylistEntry *entry = PLAYLIST_ENTRY(node->data);
		xmlNodePtr track, location;

		track = xmlNewNode(NULL, "track");
		location = xmlNewNode(NULL, "location");

		xmlAddChild(location, xmlNewText(entry->filename));
		xmlAddChild(track, location);
		xmlAddChild(tracklist, track);

		/* do we have a tuple? */
		if (entry->tuple != NULL)
		{
			if (entry->tuple->performer != NULL)
			{
				tmp = xmlNewNode(NULL, "creator");
				xmlAddChild(tmp, xmlNewText(entry->tuple->performer));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->album_name != NULL)
			{
				tmp = xmlNewNode(NULL, "album");
				xmlAddChild(tmp, xmlNewText(entry->tuple->album_name));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->track_name != NULL)
			{
				tmp = xmlNewNode(NULL, "title");
				xmlAddChild(tmp, xmlNewText(entry->tuple->track_name));
				xmlAddChild(track, tmp);
			}
		}
	}

	xmlSaveFile(filename, doc);
	xmlFreeDoc(doc);
}

PlaylistContainer plc_xspf = {
	.name = "XSPF Playlist Format",
	.ext = "xspf",
	.plc_read = playlist_load_xspf,
	.plc_write = playlist_save_xspf,
};

static void init(void)
{
	playlist_container_register(&plc_xspf);
}

static void cleanup(void)
{
	playlist_container_unregister(&plc_xspf);
}

LowlevelPlugin llp_xspf = {
	NULL,
	NULL,
	"XSPF Playlist Format",
	init,
	cleanup,
};

LowlevelPlugin *get_lplugin_info(void)
{
	return &llp_xspf;
}
