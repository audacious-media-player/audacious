/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * List implementation from RCL.
 * Copyright (C) 2002, 2003, 2004
 * 				Dennis Smit <ds@nerds-incorporated.org>,
 *			  	Sepp Wijnands <mrrazz@nerds-incorporated.org>,
 *			   	Tom Wimmenhove <nohup@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *  	    Sepp Wijnands <mrrazz@nerds-incorporated.org>,
 *   	    Tom Wimmenhove <nohup@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_LIST_H
#define _LV_LIST_H

#include <libvisual/lv_common.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sys/queue.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VISUAL_LIST(obj)				(VISUAL_CHECK_CAST ((obj), 0, VisList))

typedef struct _VisListEntry VisListEntry;
typedef struct _VisList VisList;

/**
 * An VisList destroyer function needs this signature.
 *
 * @arg data The data that was stored in a VisListEntry and thus can be destroyed.
 */
typedef void (*VisListDestroyerFunc)(void *data);

/**
 * The VisListEntry data structure is an entry within the linked list.
 * It does contain a pointer to both the previous and next entry in the list and
 * a void * to the data.
 */
struct _VisListEntry {
	VisListEntry		*prev;	/**< Previous entry in the list. */
	VisListEntry		*next;	/**< Next entry in the list. */
	void			*data;	/**< Pointer to the data for this entry. */
};

/**
 * The VisList data structure holds the linked list.
 * It contains an entry pointer to both the head and tail of the list as well
 * an entry counter.
 */
struct _VisList {
	VisObject		 object;	/**< The VisObject data. */
	VisListDestroyerFunc	 destroyer;	/**< The List destroyer function. */
	VisListEntry		*head;		/**< Pointer to the beginning of the list. */
	VisListEntry		*tail;		/**< Pointer to the end of the list. */
	int			 count;		/**< Number of entries that are in the list. */
};


/* prototypes */
VisList *visual_list_new (VisListDestroyerFunc destroyer);
int visual_list_free (VisList *list);
int visual_list_destroy_elements (VisList *list);

int visual_list_set_destroyer (VisList *list, VisListDestroyerFunc destroyer);

void *visual_list_next (VisList *list, VisListEntry **le);
void *visual_list_prev (VisList *list, VisListEntry **le);

void *visual_list_get (VisList *list, int index);

int visual_list_add_at_begin (VisList *list, void *data);
int visual_list_add (VisList *list, void *data);

int visual_list_insert (VisList *list, VisListEntry **le, void *data);
int visual_list_delete (VisList *list, VisListEntry **le);

int visual_list_count (VisList *list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LV_LIST_H */
