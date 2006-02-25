#include <pthread.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <curl/curl.h>
#include <stdio.h>
#include "fmt.h"
#include "md5.h"
#include "tags/include/tags.h"
#include "queue.h"
#include "scrobbler.h"
#include "config.h"
#include <glib.h>

#define SCROBBLER_HS_URL "http://post.audioscrobbler.com"
#define SCROBBLER_CLI_ID "xms"
#define SCROBBLER_HS_WAIT 1800
#define SCROBBLER_SB_WAIT 10
#define SCROBBLER_VERSION "1.1"
#define CACHE_SIZE 1024

/* Scrobblerbackend for xmms plugin, first draft */

static int	sc_hs_status,
		sc_hs_timeout,
		sc_hs_errors,
		sc_sb_errors,
		sc_bad_users,
		sc_submit_interval,
		sc_submit_timeout,
		sc_srv_res_size,
		sc_giveup,
		sc_major_error_present;

static char 	*sc_submit_url,
		*sc_username,
		*sc_password,
		*sc_challenge_hash,
		sc_response_hash[33],
		*sc_srv_res,
		sc_curl_errbuf[CURL_ERROR_SIZE],
		*sc_major_error;

static void dump_queue();

/* Error functions */

static void sc_throw_error(char *errortxt)
{
	sc_major_error_present = 1;
	if(sc_major_error == NULL)
		sc_major_error = strdup(errortxt);

	return;
}

int sc_catch_error(void)
{
	return sc_major_error_present;
}

char *sc_fetch_error(void)
{
	return sc_major_error;
}

void sc_clear_error(void)
{
	sc_major_error_present = 0;
	if(sc_major_error != NULL)
		free(sc_major_error);
	sc_major_error = NULL;

	return;
}

static size_t sc_store_res(void *ptr, size_t size,
		size_t nmemb,
		void *stream)
{
	int len = size * nmemb;

	sc_srv_res = realloc(sc_srv_res, sc_srv_res_size + len + 1);
	memcpy(sc_srv_res + sc_srv_res_size,
			ptr, len);
	sc_srv_res_size += len;
	return len;
}

static void sc_free_res(void)
{
	if(sc_srv_res != NULL)
		free(sc_srv_res);
	sc_srv_res = NULL;
	sc_srv_res_size = 0;
}

static int sc_parse_hs_res(void)
{
	char *interval;

	if (!sc_srv_res_size) {
		pdebug("No reply from server", DEBUG);
		return -1;
	}
	*(sc_srv_res + sc_srv_res_size) = 0;

	if (!strncmp(sc_srv_res, "FAILED ", 7)) {
		interval = strstr(sc_srv_res, "INTERVAL");
		if(!interval) {
			pdebug("missing INTERVAL", DEBUG);
		}
		else
		{
			*(interval - 1) = 0;
			sc_submit_interval = strtol(interval + 8, NULL, 10);
		}

		/* Throwing a major error, just in case */
		/* sc_throw_error(fmt_vastr("%s", sc_srv_res));
		   sc_hs_errors++; */
		pdebug(fmt_vastr("error: %s", sc_srv_res), DEBUG);

		return -1;
	}

	if (!strncmp(sc_srv_res, "UPDATE ", 7)) {
		interval = strstr(sc_srv_res, "INTERVAL");
		if(!interval)
		{
			pdebug("missing INTERVAL", DEBUG);
		}
		else
		{
			*(interval - 1) = 0;
			sc_submit_interval = strtol(interval + 8, NULL, 10);
		}

		sc_submit_url = strchr(strchr(sc_srv_res, '\n') + 1, '\n') + 1;
		*(sc_submit_url - 1) = 0;
		sc_submit_url = strdup(sc_submit_url);
		sc_challenge_hash = strchr(sc_srv_res, '\n') + 1;
		*(sc_challenge_hash - 1) = 0;
		sc_challenge_hash = strdup(sc_challenge_hash);

		/* Throwing major error. Need to alert client to update. */
		sc_throw_error(fmt_vastr("Please update Audacious.\n"
			"Update available at: http://audacious-media-player.org"));
		pdebug(fmt_vastr("update client: %s", sc_srv_res + 7), DEBUG);

		/*
		 * Russ isn't clear on whether we can submit with a not-updated
		 * client.  Neither is RJ.  I use what we did before.
		 */
		sc_giveup = -1;
		return -1;
	}
	if (!strncmp(sc_srv_res, "UPTODATE\n", 9)) {
		sc_bad_users = 0;

		interval = strstr(sc_srv_res, "INTERVAL");
		if (!interval) {
			pdebug("missing INTERVAL", DEBUG);
			/*
			 * This is probably a bad thing, but Russ seems to
			 * think its OK to assume that an UPTODATE response
			 * may not have an INTERVAL...  We return -1 anyway.
			 */
			return -1;
		}
		else
		{
			*(interval - 1) = 0;
			sc_submit_interval = strtol(interval + 8, NULL, 10);
		}

		sc_submit_url = strchr(strchr(sc_srv_res, '\n') + 1, '\n') + 1;
		*(sc_submit_url - 1) = 0;
		sc_submit_url = strdup(sc_submit_url);
		sc_challenge_hash = strchr(sc_srv_res, '\n') + 1;
		*(sc_challenge_hash - 1) = 0;
		sc_challenge_hash = strdup(sc_challenge_hash);

		return 0;
	}
	if(!strncmp(sc_srv_res, "BADUSER", 7)) {
		/* Throwing major error. */
		sc_throw_error("Incorrect username/password.\n"
				"Please fix in configuration.");
		pdebug("incorrect username/password", DEBUG);

		interval = strstr(sc_srv_res, "INTERVAL");
		if(!interval)
		{
			pdebug("missing INTERVAL", DEBUG);
		}
		else
		{
			*(interval - 1) = 0;
			sc_submit_interval = strtol(interval + 8, NULL, 10);
		}

		return -1;
	}

	pdebug(fmt_vastr("unknown server-reply '%s'", sc_srv_res), DEBUG);
	return -1;
}

static void hexify(char *pass, int len)
{
	char *bp = sc_response_hash;
	char hexchars[] = "0123456789abcdef";
	int i;

	memset(sc_response_hash, 0, sizeof(sc_response_hash));
	
	for(i = 0; i < len; i++) {
		*(bp++) = hexchars[(pass[i] >> 4) & 0x0f];
		*(bp++) = hexchars[pass[i] & 0x0f];
	}
	*bp = 0;

	return;
}

static int sc_handshake(void)
{
	int status;
	char buf[4096];
	CURL *curl;

	snprintf(buf, sizeof(buf), "%s/?hs=true&p=%s&c=%s&v=%s&u=%s",
			SCROBBLER_HS_URL, SCROBBLER_VERSION,
			SCROBBLER_CLI_ID, "0.3.8.1", sc_username);

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_URL, buf);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, 
			sc_store_res);
	memset(sc_curl_errbuf, 0, sizeof(sc_curl_errbuf));
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, sc_curl_errbuf);
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
	status = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	sc_hs_timeout = time(NULL) + SCROBBLER_HS_WAIT;

	if (status) {
		pdebug(sc_curl_errbuf, DEBUG);
		sc_hs_errors++;
		sc_free_res();
		return -1;
	}

	if (sc_parse_hs_res()) {
		sc_hs_errors++;
		sc_free_res();
		return -1;
	}

	if (sc_challenge_hash != NULL) {
		md5_state_t md5state;
		unsigned char md5pword[16];
		
		md5_init(&md5state);
		/*pdebug(fmt_vastr("Pass Hash: %s", sc_password), DEBUG);*/
		md5_append(&md5state, (unsigned const char *)sc_password,
				strlen(sc_password));
		/*pdebug(fmt_vastr("Challenge Hash: %s", sc_challenge_hash), DEBUG);*/
		md5_append(&md5state, (unsigned const char *)sc_challenge_hash,
				strlen(sc_challenge_hash));
		md5_finish(&md5state, md5pword);
		hexify(md5pword, sizeof(md5pword));
		/*pdebug(fmt_vastr("Response Hash: %s", sc_response_hash), DEBUG);*/
	}

	sc_hs_errors = 0;
	sc_hs_status = 1;

	sc_free_res();

	pdebug(fmt_vastr("submiturl: %s - interval: %d", 
				sc_submit_url, sc_submit_interval), DEBUG);

	return 0;
}

static int sc_parse_sb_res(void)
{
	char *ch, *ch2;

	if (!sc_srv_res_size) {
		pdebug("No response from server", DEBUG);
		return -1;
	}
	*(sc_srv_res + sc_srv_res_size) = 0;

	if (!strncmp(sc_srv_res, "OK", 2)) {
		if ((ch = strstr(sc_srv_res, "INTERVAL"))) {
			sc_submit_interval = strtol(ch + 8, NULL, 10);
			pdebug(fmt_vastr("got new interval: %d",
						sc_submit_interval), DEBUG);
		}

		pdebug(fmt_vastr("submission ok: %s", sc_srv_res), DEBUG);

		return 0;
	}

	if (!strncmp(sc_srv_res, "BADAUTH", 7)) {
		if ((ch = strstr(sc_srv_res, "INTERVAL"))) {
			sc_submit_interval = strtol(ch + 8, NULL, 10);
			pdebug(fmt_vastr("got new interval: %d",
						sc_submit_interval), DEBUG);
		}

		pdebug("incorrect username/password", DEBUG);

		sc_giveup = 0;

		/*
		 * We obviously aren't authenticated.  The server might have
		 * lost our handshake status though, so let's try
		 * re-handshaking...  This might not be proper.
		 * (we don't give up)
		 */
		sc_hs_status = 0;

		if(sc_challenge_hash != NULL)
			free(sc_challenge_hash);
		if(sc_submit_url != NULL)
			free(sc_submit_url);

		sc_challenge_hash = sc_submit_url = NULL;
		sc_bad_users++;

		if(sc_bad_users > 2)
		{
			pdebug("3 BADAUTH returns on submission. Halting "
				"submissions until login fixed.", DEBUG)
			sc_throw_error("Incorrect username/password.\n"
				"Please fix in configuration.");
		}

		return -1;
	}

	if (!strncmp(sc_srv_res, "FAILED", 6))  {
		if ((ch = strstr(sc_srv_res, "INTERVAL"))) {
			sc_submit_interval = strtol(ch + 8, NULL, 10);
			pdebug(fmt_vastr("got new interval: %d",
						sc_submit_interval), DEBUG);
		}

		/* This could be important. (Such as FAILED - Get new plugin) */
		/*sc_throw_error(fmt_vastr("%s", sc_srv_res));*/

		pdebug(sc_srv_res, DEBUG);

		return -1;
	}

	if (!strncmp(sc_srv_res, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">", 50)) {
		ch = strstr(sc_srv_res, "<TITLE>") + 7;
		ch2 = strstr(sc_srv_res, "</TITLE>");
		*ch2 = '\0';

		pdebug(fmt_vastr("HTTP Error (%d): '%s'",
					atoi(ch), ch + 4), DEBUG);
		*ch2 = '<';

		return -1;
	}

	pdebug(fmt_vastr("unknown server-reply %s", sc_srv_res), DEBUG);

	return -1;
}

static gchar *sc_itemtag(char c, int n, char *str)
{
    static char buf[256]; 
    snprintf(buf, 256, "&%c[%d]=%s", c, n, str);
    return buf;
}

#define cfa(f, l, n, v) \
curl_formadd(f, l, CURLFORM_COPYNAME, n, \
		CURLFORM_PTRCONTENTS, v, CURLFORM_END)

static int sc_generateentry(GString *submission)
{
	int i;
	item_t *item;

	i = 0;
#ifdef ALLOW_MULTIPLE
	q_peekall(1);
	while ((item = q_peekall(0)) && i < 10) {
#else
		item = q_peek();
#endif
		if (!item)
			return i;

                g_string_append(submission,sc_itemtag('a',i,I_ARTIST(item)));
                g_string_append(submission,sc_itemtag('t',i,I_TITLE(item)));
                g_string_append(submission,sc_itemtag('l',i,I_LEN(item)));
                g_string_append(submission,sc_itemtag('i',i,I_TIME(item)));
                g_string_append(submission,sc_itemtag('m',i,I_MB(item)));
                g_string_append(submission,sc_itemtag('b',i,I_ALBUM(item)));

		pdebug(fmt_vastr("a[%d]=%s t[%d]=%s l[%d]=%s i[%d]=%s m[%d]=%s b[%d]=%s",
				i, I_ARTIST(item),
				i, I_TITLE(item),
				i, I_LEN(item),
				i, I_TIME(item),
				i, I_MB(item),
				i, I_ALBUM(item)), DEBUG);
#ifdef ALLOW_MULTIPLE
		i++;
	}
#endif

	return i;
}

static int sc_submitentry(gchar *entry)
{
	CURL *curl;
	/* struct HttpPost *post = NULL , *last = NULL; */
	int status;
        GString *submission;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_URL, sc_submit_url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
			sc_store_res);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
	/*cfa(&post, &last, "debug", "failed");*/

	/*pdebug(fmt_vastr("Username: %s", sc_username), DEBUG);*/
        submission = g_string_new("u=");
        g_string_append(submission,(gchar *)sc_username);

	/*pdebug(fmt_vastr("Response Hash: %s", sc_response_hash), DEBUG);*/
        g_string_append(submission,"&s=");
        g_string_append(submission,(gchar *)sc_response_hash);

	g_string_append(submission, entry);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)submission->str);
	memset(sc_curl_errbuf, 0, sizeof(sc_curl_errbuf));
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, sc_curl_errbuf);

	/*
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, SCROBBLER_SB_WAIT);
	*/

	status = curl_easy_perform(curl);

	curl_easy_cleanup(curl);

        g_string_free(submission,TRUE);

	if (status) {
		pdebug(sc_curl_errbuf, DEBUG);
		sc_sb_errors++;
		sc_free_res();
		return -1;
	}

	if (sc_parse_sb_res()) {
		sc_sb_errors++;
		sc_free_res();
		pdebug(fmt_vastr("Retrying in %d secs, %d elements in queue",
					sc_submit_interval, q_len()), DEBUG);
		return -1;
	}
	sc_free_res();
	return 0;
}

static void sc_handlequeue(GMutex *mutex)
{
	GString *submitentry;
	int nsubmit;
	int wait;

	if(sc_submit_timeout < time(NULL) && sc_bad_users < 3)
	{
		submitentry = g_string_new("");

		g_mutex_lock(mutex);

		nsubmit = sc_generateentry(submitentry);

		g_mutex_unlock(mutex);

		if (nsubmit > 0)
		{
			pdebug(fmt_vastr("Number submitting: %d", nsubmit), DEBUG);
			pdebug(fmt_vastr("Submission: %s", submitentry->str), DEBUG);

			if(!sc_submitentry(submitentry->str))
			{
				g_mutex_lock(mutex);

#ifdef ALLOW_MULTIPLE
				q_free();
#else
				q_get();
#endif
				/*
				 * This should make sure that the queue doesn't
				 * get submitted multiple times on a nasty
				 * segfault...
				 */
				dump_queue();

				g_mutex_unlock(mutex);

				sc_sb_errors = 0;
			}
			if(sc_sb_errors)
			{
				if(sc_sb_errors < 5)
					/* Retry after 1 min */
					wait = 60;
				else
					wait = /* sc_submit_interval + */
						( ((sc_sb_errors - 5) < 7) ?
						(60 << (sc_sb_errors-5)) :
						7200 );
				
				sc_submit_timeout = time(NULL) + wait;

				pdebug(fmt_vastr("Error while submitting. "
					"Retrying after %d seconds.", wait),
					DEBUG);
			}
		}

		g_string_free(submitentry, TRUE);
	}
}

static void read_cache(void)
{
	FILE *fd;
	char *home, buf[PATH_MAX], *cache = NULL, *ptr1, *ptr2;
	int cachesize, written, i = 0;
	item_t *item;

	cachesize = written = 0;

	if(!(home = getenv("HOME")))
		return;

	snprintf(buf, sizeof(buf), "%s/.audacious/scrobblerqueue.txt", home);

	if (!(fd = fopen(buf, "r")))
		return;
	pdebug(fmt_vastr("Opening %s", buf), DEBUG);
	while(!feof(fd))
	{
		cachesize += CACHE_SIZE;
		cache = realloc(cache, cachesize + 1);
		written += fread(cache + written, 1, CACHE_SIZE, fd);
		cache[written] = '\0';
	}
	fclose(fd);
	ptr1 = cache;
	while(ptr1 < cache + written - 1)
	{
		char *artist, *title, *len, *time, *album, *mb;

		pdebug("Pushed:", DEBUG);
		ptr2 = strchr(ptr1, ' ');
		artist = calloc(1, ptr2 - ptr1 + 1);
		strncpy(artist, ptr1, ptr2 - ptr1);
		ptr1 = ptr2 + 1;
		ptr2 = strchr(ptr1, ' ');
		title = calloc(1, ptr2 - ptr1 + 1);
		strncpy(title, ptr1, ptr2 - ptr1);
		ptr1 = ptr2 + 1;
		ptr2 = strchr(ptr1, ' ');
		len = calloc(1, ptr2 - ptr1 + 1);
		strncpy(len, ptr1, ptr2 - ptr1);
		ptr1 = ptr2 + 1;
		ptr2 = strchr(ptr1, ' ');
		time = calloc(1, ptr2 - ptr1 + 1);
		strncpy(time, ptr1, ptr2 - ptr1);
		ptr1 = ptr2 + 1;
		ptr2 = strchr(ptr1, ' ');
		album = calloc(1, ptr2 - ptr1 + 1);
		strncpy(album, ptr1, ptr2 - ptr1);
		ptr1 = ptr2 + 1;
		ptr2 = strchr(ptr1, '\n');
		if(ptr2 != NULL)
			*ptr2 = '\0';
		mb = calloc(1, strlen(ptr1) + 1);
		strncpy(mb, ptr1, strlen(ptr1));
		if(ptr2 != NULL)
			*ptr2 = '\n';
		/* Why is our save printing out CR/LF? */
		ptr1 = ptr2 + 1;

		item = q_put2(artist, title, len, time, album, mb);
		pdebug(fmt_vastr("a[%d]=%s t[%d]=%s l[%d]=%s i[%d]=%s m[%d]=%s b[%d]=%s",
				i, I_ARTIST(item),
				i, I_TITLE(item),
				i, I_LEN(item),
				i, I_TIME(item),
				i, I_MB(item),
				i, I_ALBUM(item)), DEBUG);
		free(artist);
		free(title);
		free(len);
		free(time);
		free(album);
		free(mb);

		i++;
	}
	pdebug("Done loading cache.", DEBUG);
}

static void dump_queue(void)
{
	FILE *fd;
	item_t *item;
	char *home, buf[PATH_MAX];

	/*pdebug("Entering dump_queue();", DEBUG);*/

	if (!(home = getenv("HOME")))
	{
		pdebug("No HOME directory found. Cannot dump queue.", DEBUG);
		return;
	}

	snprintf(buf, sizeof(buf), "%s/.audacious/scrobblerqueue.txt", home);

	if (!(fd = fopen(buf, "w")))
	{
		pdebug(fmt_vastr("Failure opening %s", buf), DEBUG);
		return;
	}

	pdebug(fmt_vastr("Opening %s", buf), DEBUG);

	q_peekall(1);

	while ((item = q_peekall(0))) {
		fprintf(fd, "%s %s %s %s %s %s\n",
					I_ARTIST(item),
					I_TITLE(item),
					I_LEN(item),
					I_TIME(item),
					I_ALBUM(item),
					I_MB(item));
	}

	fclose(fd);
}

/* This was made public */

void sc_cleaner(void)
{
	if(sc_submit_url != NULL)
		free(sc_submit_url);
	if(sc_username != NULL)
		free(sc_username);
	if(sc_password != NULL)
		free(sc_password);
	if(sc_challenge_hash != NULL)
		free(sc_challenge_hash);
	if(sc_srv_res != NULL)
		free(sc_srv_res);
	if(sc_major_error != NULL)
		free(sc_major_error);
	dump_queue();
	q_free();
	pdebug("scrobbler shutting down", DEBUG);
}

static void sc_checkhandshake(void)
{
	int wait;

	if (sc_hs_status)
		return;
	if (sc_hs_timeout < time(NULL))
	{
		sc_handshake();

		if(sc_hs_errors)
		{
			if(sc_hs_errors < 5)
				/* Retry after 60 seconds */
				wait = 60;
			else
				wait = /* sc_submit_interval + */
					( ((sc_hs_errors - 5) < 7) ?
					(60 << (sc_hs_errors-5)) :
					7200 );
			sc_hs_timeout = time(NULL) + wait;
			pdebug(fmt_vastr("Error while handshaking. Retrying "
				"after %d seconds.", wait), DEBUG);
		}
	}
}

/**** Public *****/

/* Called at session startup*/

void sc_init(char *uname, char *pwd)
{
	sc_hs_status = sc_hs_timeout = sc_hs_errors = sc_submit_timeout =
		sc_srv_res_size = sc_giveup = sc_major_error_present =
		sc_bad_users = sc_sb_errors = 0;
	sc_submit_interval = 100;

	sc_submit_url = sc_username = sc_password = sc_srv_res =
		sc_challenge_hash = sc_major_error = NULL;
	sc_username = strdup(uname);
	sc_password = strdup(pwd);
	read_cache();
	pdebug("scrobbler starting up", DEBUG);
}

void sc_addentry(GMutex *mutex, metatag_t *meta, int len)
{
	g_mutex_lock(mutex);
	q_put(meta, len);
	/*
	 * This will help make sure the queue will be saved on a nasty
	 * segfault...
	 */
	dump_queue();
	g_mutex_unlock(mutex);
}

/* Call periodically from the plugin */
int sc_idle(GMutex *mutex)
{
	sc_checkhandshake();
	if (sc_hs_status)
		sc_handlequeue(mutex);

	return sc_giveup;
}
