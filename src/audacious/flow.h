/*
 * Audacious
 * Copyright (c) 2007 William Pitcock
 *
 * flow.h: Definition of flow context structure, flow management API.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 * The Audacious team does not consider modular code linking to
 * Audacious or using our public API to be a derived work. 
 */

#include <glib.h>
#include <mowgli.h>

#include "output.h"

#ifndef AUDACIOUS_FLOW_H
#define AUDACIOUS_FLOW_H

typedef struct {
    gint time;
    gpointer data;
    gsize len;
    AFormat fmt;
    gint srate;
    gint channels;
    gboolean error;
} FlowContext;

typedef void (*FlowFunction)(FlowContext *ctx);

typedef struct _FlowElement {
    struct _FlowElement *prev, *next;
    FlowFunction func;
} FlowElement;

typedef struct {
    mowgli_object_t parent;
    FlowElement *head, *tail;
} Flow;

gsize flow_execute(Flow *flow, gint time, gpointer *data, gsize len, AFormat fmt, 
     gint srate, gint channels);

Flow *flow_new(void);
void flow_link_element(Flow *flow, FlowFunction func);
void flow_unlink_element(Flow *flow, FlowFunction func);

#define flow_destroy(flow) mowgli_object_unref(flow)

#endif /* AUDACIOUS_FLOW_H */
