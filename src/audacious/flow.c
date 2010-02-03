/*
 * Audacious
 * Copyright (c) 2007 William Pitcock
 *
 * flow.c: flow management API.
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

#include "main.h"
#include "flow.h"

mowgli_object_class_t flow_klass;

static void
flow_destructor(Flow *flow)
{
    FlowElement *element, *element2;

    g_return_if_fail(flow != NULL);

    MOWGLI_ITER_FOREACH_SAFE(element, element2, flow->head)
        g_slice_free(FlowElement, element);

    g_slice_free(Flow, flow);
}

gsize
flow_execute(Flow *flow, gint time, gpointer *data, gsize len, AFormat fmt,
     gint srate, gint channels)
{
    FlowElement *element;
    FlowContext context;

    g_return_val_if_fail(flow != NULL, 0);
    g_return_val_if_fail(data != NULL, 0);

    context.time = time;
    context.data = *data;
    context.len = len;
    context.fmt = fmt;
    context.srate = srate;
    context.channels = channels;
    context.error = FALSE;

    MOWGLI_ITER_FOREACH(element, flow->head)
    {
        element->func(&context);

        if (context.error) {
            AUDDBG("context.error!\n");
            break;
        }
    }

    *data = context.data;

    return context.len;
}

Flow *
flow_new(void)
{
    static int init = 0;
    Flow *out;

    if (!init)
    {
        mowgli_object_class_init(&flow_klass, "audacious.flow",
            (mowgli_destructor_t) flow_destructor, FALSE);
        ++init;
    }

    out = g_slice_new0(Flow);
    mowgli_object_init(mowgli_object(out), NULL, &flow_klass, NULL);

    return out;
}

void
flow_link_element(Flow *flow, FlowFunction func)
{
    FlowElement *element;

    g_return_if_fail(flow != NULL);
    g_return_if_fail(func != NULL);

    element = g_slice_new0(FlowElement);
    element->func = func;
    element->prev = flow->tail;

    if (flow->tail)
        flow->tail->next = element;

    flow->tail = element;

    if (!flow->head)
        flow->head = element;
}

/* TBD: unlink all elements of func, or just the first --nenolod */
void
flow_unlink_element(Flow *flow, FlowFunction func)
{
    FlowElement *iter, *iter2;

    g_return_if_fail(flow != NULL);
    g_return_if_fail(func != NULL);

    MOWGLI_ITER_FOREACH_SAFE(iter, iter2, flow->head)
        if (iter->func == func)
        {
            if (iter->next)
                iter->next->prev = iter->prev;

            iter->prev->next = iter->next;

            if (flow->tail == iter)
                flow->tail = iter->prev;

            if (flow->head == iter)
                flow->head = iter->next;

            g_slice_free(FlowElement, iter);
        }
}
