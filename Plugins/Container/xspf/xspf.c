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

static void
playlist_load_xspf(const gchar * filename, gint pos)
{
	xmlDocPtr	   doc;
	xmlXPathObjectPtr  xpathObj;
        xmlXPathContextPtr xpathCtx;
	xmlNodeSetPtr      n;
	gint i;

	g_return_if_fail(filename != NULL);

	doc = xmlParseFile(filename);
	if (doc == NULL)
		return;

	xpathCtx = xmlXPathNewContext(doc);
	if (xpathCtx == NULL)
	{
		g_message("xpathCtx is NULL.");
		return;
	}

	if (xmlXPathRegisterNs(xpathCtx, "xspf", "http://xspf.org/ns/0/") != 0)
	{
		g_message("Failed to register XSPF namespace.");
		return;
	}

	/* TODO: what about xspf:artist, xspf:title, xspf:length? */
	xpathObj = xmlXPathEvalExpression("//xspf:location", xpathCtx);
	if (xpathObj == NULL)
	{
		g_message("XPath Expression failed to evaluate.");
		return;
	}

	xmlXPathFreeContext(xpathCtx);

	n = xpathObj->nodesetval;
	if (n == NULL)
	{
		g_message("XPath Expression yielded no results.");
		return;
	}

	for (i = 0; i < n->nodeNr && n->nodeTab[i]->children != NULL; i++)
	{
		char *uri = XML_GET_CONTENT(n->nodeTab[i]->children);
		gchar *locale_uri = NULL;
		
		if (uri == NULL)
			continue;

		++pos;
		locale_uri = g_locale_from_utf8(uri, -1, NULL, NULL, NULL);
		if(locale_uri) {
			playlist_ins(locale_uri, pos);
			g_free(locale_uri);
			locale_uri = NULL;
		}
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
		gchar *utf_filename = NULL;

		track = xmlNewNode(NULL, "track");
		location = xmlNewNode(NULL, "location");

		utf_filename  = g_locale_to_utf8(entry->filename, -1, NULL, NULL, NULL);
		if(!utf_filename)
			continue;
		xmlAddChild(location, xmlNewText(utf_filename));
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
		if(utf_filename) {
			g_free(utf_filename);
			utf_filename = NULL;
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
