/* Compatibility macros to make supporting multiple GLib versions easier.
 * Public domain. */

#ifndef AUD_GLIB_COMPAT_H
#define AUD_GLIB_COMPAT_H

#if ! GLIB_CHECK_VERSION (2, 14, 0)

static inline void g_queue_init (GQueue * q)
{
	q->head = q->tail = NULL;
	q->length = 0;
}

static inline void g_queue_clear (GQueue * q)
{
	g_list_free (q->head);
	q->head = q->tail = NULL;
	q->length = 0;
}

#define G_QUEUE_INIT {NULL, NULL, 0}
#define g_timeout_add_seconds(s, f, d) g_timeout_add (1000 * (s), (f), (d))
#endif

#endif /* AUD_GLIB_COMPAT_H */
