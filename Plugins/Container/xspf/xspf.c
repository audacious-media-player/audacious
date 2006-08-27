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
add_file(xmlNode *track, const gchar *filename, gint pos)
{
	xmlNode *nptr;
	TitleInput *tuple;
	gchar *locale_uri = NULL;

	tuple = bmp_title_input_new();

	// creator, album, title, duration, trackNum, annotation, image, 
	for(nptr = track->children; nptr != NULL; nptr = nptr->next){
		if(nptr->type == XML_ELEMENT_NODE && !xmlStrcmp(nptr->name, "location")){
			xmlChar *str = xmlNodeGetContent(nptr);
			locale_uri = g_locale_from_utf8(str,-1,NULL,NULL,NULL);
			if(!locale_uri){
				/* try ISO-8859-1 for last resort */
				locale_uri = g_convert(str, -1, "ISO-8859-1", "UTF-8", NULL, NULL, NULL);
			}
			xmlFree(str);
			if(locale_uri){
				tuple->file_name = g_path_get_basename(locale_uri);
				tuple->file_path = g_path_get_dirname(locale_uri);
			}
		}
		else if(nptr->type == XML_ELEMENT_NODE && !xmlStrcmp(nptr->name, "creator")){
			tuple->performer = (gchar *)xmlNodeGetContent(nptr);
		}
		else if(nptr->type == XML_ELEMENT_NODE && !xmlStrcmp(nptr->name, "album")){
			tuple->album_name = (gchar *)xmlNodeGetContent(nptr);
		}
		else if(nptr->type == XML_ELEMENT_NODE && !xmlStrcmp(nptr->name, "title")){
			tuple->track_name = (gchar *)xmlNodeGetContent(nptr);
		}
		else if(nptr->type == XML_ELEMENT_NODE && !xmlStrcmp(nptr->name, "duration")){
			xmlChar *str = xmlNodeGetContent(nptr);
			tuple->length = atol(str);
			xmlFree(str);
		}
		else if(nptr->type == XML_ELEMENT_NODE && !xmlStrcmp(nptr->name, "trackNum")){
			xmlChar *str = xmlNodeGetContent(nptr);
			tuple->track_number = atol(str);
			xmlFree(str);
		}

		//
		// additional metadata
		//
		// year, date, genre, comment, file_ext, file_path, formatter
		//
		else if(nptr->type == XML_ELEMENT_NODE && !xmlStrcmp(nptr->name, "meta")){
			xmlChar *rel = NULL;
			
			rel = xmlGetProp(nptr, "rel");

			if(!xmlStrcmp(rel, "year")){
				xmlChar *cont = xmlNodeGetContent(nptr);
				tuple->year = atol(cont);
				xmlFree(cont);
				continue;
			}
			else if(!xmlStrcmp(rel, "date")){
				tuple->date = (gchar *)xmlNodeGetContent(nptr);
				continue;
			}
			else if(!xmlStrcmp(rel, "genre")){
				tuple->genre = (gchar *)xmlNodeGetContent(nptr);
				continue;
			}
			else if(!xmlStrcmp(rel, "comment")){
				tuple->comment = (gchar *)xmlNodeGetContent(nptr);
				continue;
			}
			else if(!xmlStrcmp(rel, "file_ext")){
				tuple->file_ext = (gchar *)xmlNodeGetContent(nptr);
				continue;
			}
			else if(!xmlStrcmp(rel, "file_path")){
				tuple->file_path = (gchar *)xmlNodeGetContent(nptr);
				continue;
			}
			else if(!xmlStrcmp(rel, "formatter")){
				tuple->formatter = (gchar *)xmlNodeGetContent(nptr);
				continue;
			}
			else if(!xmlStrcmp(rel, "mtime")){
				xmlChar *str;
				str = xmlNodeGetContent(nptr);
				tuple->mtime = (time_t)atoll(str);
				if(str)
					xmlFree(str);
				continue;
			}
			xmlFree(rel);
			rel = NULL;
		}

	}
	if (tuple->length == 0) {
		tuple->length = -1;
	}
	// add file to playlist
	playlist_load_ins_file_tuple(locale_uri, filename, pos, tuple);
	pos++;
	if(locale_uri) {
		g_free(locale_uri);
		locale_uri = NULL;
	}
}

static void
find_track(xmlNode *tracklist, const gchar *filename, gint pos)
{
	xmlNode *nptr;
	for(nptr = tracklist->children; nptr != NULL; nptr = nptr->next){
		if(nptr->type == XML_ELEMENT_NODE && !xmlStrcmp(nptr->name, "track")){
			add_file(nptr, filename, pos);
		}
	}
}

static void
playlist_load_xspf(const gchar * filename, gint pos)
{
	xmlDocPtr doc;
	xmlNode *nptr, *nptr2;

	g_return_if_fail(filename != NULL);

	doc = xmlParseFile(filename);
	if (doc == NULL)
		return;

	// find trackList
	for(nptr = doc->children; nptr != NULL; nptr = nptr->next) {
		if(nptr->type == XML_ELEMENT_NODE && !xmlStrcmp(nptr->name, "playlist")){
			for(nptr2 = nptr->children; nptr2 != NULL; nptr2 = nptr2->next){
				if(nptr2->type == XML_ELEMENT_NODE && !xmlStrcmp(nptr2->name, "trackList")){
					find_track(nptr2, filename, pos);
				}
			}

		}
	}

	xmlFreeDoc(doc);

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

		/* try locale encoding first */
		utf_filename = g_locale_to_utf8(entry->filename, -1, NULL, NULL, NULL);

		if (!utf_filename) {
			/* if above fails, try to guess */
			utf_filename = str_to_utf8(entry->filename);
		}

		if(!g_utf8_validate(utf_filename, -1, NULL))
			continue;
		xmlAddChild(location, xmlNewText(utf_filename));
		xmlAddChild(track, location);
		xmlAddChild(tracklist, track);

		/* do we have a tuple? */
		if (entry->tuple != NULL)
		{
			if (entry->tuple->performer != NULL &&
			    g_utf8_validate(entry->tuple->performer, -1, NULL))
			{
				tmp = xmlNewNode(NULL, "creator");
				xmlAddChild(tmp, xmlNewText(entry->tuple->performer));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->album_name != NULL &&
			    g_utf8_validate(entry->tuple->album_name, -1, NULL))
			{
				tmp = xmlNewNode(NULL, "album");
				xmlAddChild(tmp, xmlNewText(entry->tuple->album_name));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->track_name != NULL &&
			    g_utf8_validate(entry->tuple->track_name, -1, NULL))
			{
				tmp = xmlNewNode(NULL, "title");
				xmlAddChild(tmp, xmlNewText(entry->tuple->track_name));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->length > 0)
			{
				gchar *str;
				str = g_malloc(128); // XXX fix me.
				tmp = xmlNewNode(NULL, "duration");
				sprintf(str, "%d", entry->tuple->length);
				xmlAddChild(tmp, xmlNewText(str));
				g_free(str);
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->track_number != 0)
			{
				gchar *str;
				str = g_malloc(128); // XXX fix me.
				tmp = xmlNewNode(NULL, "trackNum");
				sprintf(str, "%d", entry->tuple->track_number);
				xmlAddChild(tmp, xmlNewText(str));
				g_free(str);
				xmlAddChild(track, tmp);
			}

			//
			// additional metadata
			//
			// year, date, genre, comment, file_ext, file_path, formatter
			//
			if (entry->tuple->year != 0)
			{
				gchar *str;
				str = g_malloc(128); // XXX fix me.
				tmp = xmlNewNode(NULL, "meta");
				xmlSetProp(tmp, "rel", "year");
				sprintf(str, "%d", entry->tuple->year);
				xmlAddChild(tmp, xmlNewText(str));
				xmlAddChild(track, tmp);
				g_free(str);
			}

			if (entry->tuple->date != NULL &&
			    g_utf8_validate(entry->tuple->date, -1, NULL))
			{
				tmp = xmlNewNode(NULL, "meta");
				xmlSetProp(tmp, "rel", "date");
				xmlAddChild(tmp, xmlNewText(entry->tuple->date));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->genre != NULL &&
			    g_utf8_validate(entry->tuple->genre, -1, NULL))
			{
				tmp = xmlNewNode(NULL, "meta");
				xmlSetProp(tmp, "rel", "genre");
				xmlAddChild(tmp, xmlNewText(entry->tuple->genre));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->comment != NULL &&
			    g_utf8_validate(entry->tuple->comment, -1, NULL))
			{
				tmp = xmlNewNode(NULL, "meta");
				xmlSetProp(tmp, "rel", "comment");
				xmlAddChild(tmp, xmlNewText(entry->tuple->comment));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->file_ext != NULL &&
			    g_utf8_validate(entry->tuple->file_ext, -1, NULL))
			{
				tmp = xmlNewNode(NULL, "meta");
				xmlSetProp(tmp, "rel", "file_ext");
				xmlAddChild(tmp, xmlNewText(entry->tuple->file_ext));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->file_path != NULL &&
			    g_utf8_validate(entry->tuple->file_path, -1, NULL))
			{
				tmp = xmlNewNode(NULL, "meta");
				xmlSetProp(tmp, "rel", "file_path");
				xmlAddChild(tmp, xmlNewText(entry->tuple->file_path));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->formatter != NULL &&
			    g_utf8_validate(entry->tuple->formatter, -1, NULL))
			{
				tmp = xmlNewNode(NULL, "meta");
				xmlSetProp(tmp, "rel", "formatter");
				xmlAddChild(tmp, xmlNewText(entry->tuple->formatter));
				xmlAddChild(track, tmp);
			}

			if (entry->tuple->mtime) {
				gchar *str;
				str = g_malloc(128); // XXX fix me.
				tmp = xmlNewNode(NULL, "meta");
				xmlSetProp(tmp, "rel", "mtime");
				sprintf(str, "%ld", entry->tuple->mtime);
				xmlAddChild(tmp, xmlNewText(str));
				xmlAddChild(track, tmp);
				g_free(str);
			}				

		}
		if(utf_filename) {
			g_free(utf_filename);
			utf_filename = NULL;
		}
	}

	xmlSaveFormatFile(filename, doc, 1);
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
