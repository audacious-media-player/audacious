#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "queue.h"
#include "fmt.h"

static item_t *q_queue = NULL;
static item_t *q_queue_last = NULL;
static int q_nitems;

static void q_item_free(item_t *item)
{
	if (item == NULL)
		return;
	curl_free(item->artist);
	curl_free(item->title);
	curl_free(item->utctime);
	curl_free(item->mb);
	curl_free(item->album);
	free(item);
}

void q_put(TitleInput *tuple, int len)
{
	item_t *item;

	item = malloc(sizeof(item_t));

	item->artist = fmt_escape(tuple->performer);
	item->title = fmt_escape(tuple->track_name);
	item->utctime = fmt_escape(fmt_timestr(time(NULL), 1));
	snprintf(item->len, sizeof(item->len), "%d", len);

#ifdef NOTYET
	if(tuple->mb == NULL)
#endif
		item->mb = fmt_escape("");
#ifdef NOTYET
	else
		item->mb = fmt_escape((char*)tuple->mb);
#endif

	if(tuple->album_name == NULL)
		item->album = fmt_escape("");
	else
		item->album = fmt_escape((char*)tuple->album_name);

	q_nitems++;

	item->next = NULL;

	if(q_queue_last == NULL)
		q_queue = q_queue_last = item;
	else
	{
        	q_queue_last->next = item;
		q_queue_last = item;
	}
}

item_t *q_put2(char *artist, char *title, char *len, char *time,
		char *album, char *mb)
{
	char *temp = NULL;
	item_t *item;

	item = calloc(1, sizeof(item_t));
	temp = fmt_unescape(artist);
	item->artist = fmt_escape(temp);
	curl_free(temp);
	temp = fmt_unescape(title);
	item->title = fmt_escape(temp);
	curl_free(temp);
	memcpy(item->len, len, sizeof(len));
	temp = fmt_unescape(time);
	item->utctime = fmt_escape(temp);
	curl_free(temp);
	temp = fmt_unescape(album);
	item->album = fmt_escape(temp);
	curl_free(temp);
	temp = fmt_unescape(mb);
	item->mb = fmt_escape(temp);
	curl_free(temp);

	q_nitems++;

	item->next = NULL;
	if(q_queue_last == NULL)
		q_queue = q_queue_last = item;
	else
	{
		q_queue_last->next = item;
		q_queue_last = item;
	}

	return item;
}

item_t *q_peek(void)
{
	if (q_nitems == 0)
		return NULL;
	return q_queue;
}

item_t *q_peekall(int rewind)
{
	static item_t *citem = NULL;
	item_t *temp_item;

	if (rewind) {
		citem = q_queue;
		return NULL;
	}

	temp_item = citem;

	if(citem != NULL)
		citem = citem->next;

	return temp_item;
}

int q_get(void)
{
	item_t *item;

	if (q_nitems == 0)
		return 0;
	
	item = q_queue;

	if(item == NULL)
		return 0;

	q_nitems--;
	q_queue = q_queue->next;

	q_item_free(item);

	if (q_nitems == 0)
	{
		q_queue_last = NULL;
		return 0;
	}

	return -1;
}

void q_free(void)
{
	while (q_get());
}

int q_len(void)
{
	return q_nitems;
}
