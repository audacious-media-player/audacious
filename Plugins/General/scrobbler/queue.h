#ifndef QUEUE_H
#define QUEUE_H 1

#define I_ARTIST(i) i->artist
#define I_TITLE(i) i->title
#define I_TIME(i) i->utctime
#define I_LEN(i) i->len
#define I_MB(i) i->mb
#define I_ALBUM(i) i->album

typedef struct {
	char *artist,
		*title,
		*mb,
		*album,
		*utctime,
		len[16];
	int numtries;
	void *next;
} item_t;
void q_put(metatag_t *, int);
item_t *q_put2(char *, char *, char *, char *, char *, char *);
item_t *q_peek(void);
item_t *q_peekall(int);
int q_get(void);
void q_free(void);
int q_len(void);
#endif
