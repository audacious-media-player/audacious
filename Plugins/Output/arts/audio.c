/*
 *  aRts ouput plugin for xmms
 *
 *  Copyright (C) 2000,2003  Haavard Kvaalen <havardk@xmms.org>
 *
 *  Licenced under GNU GPL version 2.
 *
 *  Audacious port by Giacomo Lozito from develia.org
 *
 */

#include "arts.h"
#include "arts_helper/arts_helper.h"
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static gboolean going, paused, helper_failed;
static guint64 written;
static struct params_info input_params, output_params;
static int helperfd;
static pid_t helper_pid;

static int (*arts_convert_func)(void **data, int length);
struct arts_config artsxmms_cfg;

struct {
	int left, right;
} volume = {100, 100};


typedef struct format_info {
  AFormat format;
  long    frequency;
  int     channels;
  long    bps;
} format_info_t;

static format_info_t input;
/* static format_info_t effect; */
/* static format_info_t output; */


void artsxmms_tell_audio(AFormat * fmt, gint * srate, gint * nch)
{
	(*fmt) = input.format;
	(*srate) = input.frequency;
	(*nch) = input.channels;
}


void artsxmms_init(void)
{
	ConfigDb *db;

	memset(&artsxmms_cfg, 0, sizeof (artsxmms_cfg));

	artsxmms_cfg.buffer_size = 400;
	
	db = bmp_cfg_db_open();
	bmp_cfg_db_get_int(db, "arts", "buffer_size",
			  &artsxmms_cfg.buffer_size);
	bmp_cfg_db_close(db);
}


static int read_all(int fd, void *buf, size_t count)
{
	size_t left = count;
	int r;
	do {
		r = read(fd, buf, left);
		if (r < 0)
			return -1;
		left -= r;
		buf = (char *)buf + r;
	} while (left > 0 && r > 0);
	return count - left;
}

static int write_all(int fd, const void *buf, size_t count)
{
	size_t left = count;
	int w;
	do {
		w = write(fd, buf, left);
		if (w < 0)
			return -1;
		left -= w;
		buf = (char *)buf + w;
	} while (left > 0 && w > 0);
/* 	g_message("wrote: %d", count - left); */
	return count - left;
}

static int wait_for_helper(int fd)
{
	struct timeval timeout;
	fd_set rdfs;
	int sr;

	FD_ZERO(&rdfs);
	FD_SET(fd, &rdfs);

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	sr = select(fd + 1, &rdfs, NULL, NULL, &timeout);
	if (sr < 0) {
		g_message("wait_for_helper(): select failed: %s",
			  strerror(errno));
		return -1;
	} else if (!sr) {
		g_message("wait_for_helper(): Timed out waiting for helper");
		return -1;
	}
	return 0;
}

static int xx;

static int helper_cmd_data(int cmd, int idata, void* ptr, int data_length)
{
	static pthread_mutex_t artsm = PTHREAD_MUTEX_INITIALIZER;
	struct command out;
	struct response in;
	int status;

	out.cmd = cmd;
	out.data = idata;
	out.data_length = data_length;
	xx++;

	if (helper_failed)
		goto failed;

	pthread_mutex_lock(&artsm);
/*  	fprintf(stderr, "Sending %d; ", out.cmd); */
	if (write_all(helperfd, &out, sizeof (out)) != sizeof (out))
		goto failed;
	if (data_length > 0)
		if (write_all(helperfd, ptr, data_length) != data_length)
			goto failed;

	if (wait_for_helper(helperfd)) {
		g_message("waiting failed: %d", cmd);
		goto failed;
	}

	if (read_all(helperfd, &in, sizeof (in)) != sizeof (in))
	{
		g_message("read failed: %d", cmd);
		goto failed;
	}

/*  	fprintf(stderr, "%d complete\n", out.cmd); */
	pthread_mutex_unlock(&artsm);

	if (in.status)
		return -in.status;
	return in.data;

 failed:
	g_message("helper_cmd_data(): failed");
	helper_failed = TRUE;
	if (helper_pid && waitpid(helper_pid, &status, WNOHANG)) {
		if (status)
			g_message("helper terminated abnormally: %d", status);
		else
			g_message("helper terminated normally");
		helper_pid = 0;
	} else if (helper_pid)
		g_message("helper has not terminated");
	pthread_mutex_unlock(&artsm);
	return -STATUS_FAILED;
}

static int helper_cmd(int cmd, int idata)
{
	return helper_cmd_data(cmd, idata, NULL, 0);
}

static int artsxmms_helper_init(struct params_info *params)
{
	int ret;
	struct init_data id;

	id.version = HELPER_VERSION;
	id.resolution = params->resolution;
	id.rate= params->frequency;
	id.nchannels = params->channels;
	id.buffer_time = artsxmms_cfg.buffer_size;

	ret = helper_cmd_data(CMD_INIT, 0, &id, sizeof (id));
	if (ret) {
		g_message("Init failed: %d", -ret);
		return -1;
	}

	return 0;
}

static void artsxmms_set_params(struct params_info *params, AFormat fmt, int rate, int nch)
{
	params->format = fmt;
	params->frequency = rate;
	params->channels = nch;

	params->bps = rate * nch;
	params->resolution = 8;
	if (!(fmt == FMT_U8 || fmt == FMT_S8))
	{
		params->bps *= 2;
		params->resolution = 16;
	}
}

int artsxmms_get_written_time(void)
{
	if (!going)
		return 0;

	return (written * 1000) / output_params.bps;
}

int artsxmms_get_output_time(void) 
{
	int time;

	if (!going)
		return 0;
	if (helper_failed)
		return -2;

	time = artsxmms_get_written_time();
	time -= helper_cmd(CMD_GET_OUTPUT_LATENCY, 0);

	if (time < 0)
		return 0;
	return time;
}

int artsxmms_playing(void)
{
	if (!going)
		return FALSE;
	
	if (!paused)
	{
		if (helper_cmd(CMD_QUERY_PLAYING, 0) <= 0)
			return FALSE;
		return TRUE;
	}

	return TRUE;
}

int artsxmms_free(void)
{
	int space;

	if (!going)
		return 0;

	space = helper_cmd(CMD_FREE, 0);
	if (space < 0)
		return 0;

	return space;
}

void artsxmms_write(gpointer ptr, int length)
{
	AFormat new_format;
	int new_frequency, new_channels;
	EffectPlugin *ep;
	
	new_format = input_params.format;
	new_frequency = input_params.frequency;
	new_channels = input_params.channels;

	ep = get_current_effect_plugin();
	if (effects_enabled() && ep && ep->query_format)
		ep->query_format(&new_format, &new_frequency, &new_channels);
	
	if (new_format != output_params.format ||
	    new_frequency != output_params.frequency ||
	    new_channels != output_params.channels)
	{
		/*
		 * The effect plugins has changed the format of the stream.
		 */

		guint64 offset = (written * 1000) / output_params.bps;
		artsxmms_set_params(&output_params, new_format,
				    new_frequency, new_channels);
		arts_convert_func = arts_get_convert_func(output_params.format);
	
		written = (offset * output_params.bps) / 1000;

		artsxmms_helper_init(&output_params);
	}

	/*
	 * Doing the effect plugin processing here adds some latency,
	 * but the alternative is just too frigging hairy.
	 */
	
	if (effects_enabled() && ep && ep->mod_samples)
		length = ep->mod_samples(&ptr, length, input_params.format,
					 input_params.frequency,
					 input_params.channels);

	if (arts_convert_func)
		arts_convert_func(ptr, length);

	helper_cmd_data(CMD_WRITE, 0, ptr, length);
	written += length;
}

void artsxmms_close(void)
{
	int status;
	going = 0;
/* 	g_message("sending quit cmd"); */
	if (!helper_cmd(CMD_QUIT, 0)) {
		waitpid(helper_pid, &status, 0);
		if (status)
			g_message("artsxmms_close(): Child exited abnormally: %d",
				  status);
	}
}

void artsxmms_flush(int time)
{
	/*
	 * Argh, no way to flush the stream from the C api.
	 */
	written = (time / 10) * (output_params.bps / 100);

}

void artsxmms_pause(short p)
{
	paused = p;
	helper_cmd(CMD_PAUSE, p);
}

static int artsxmms_start_helper()
{
	int sockets[2];

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
	{
		g_message("artsxmms_start_helper(): "
			  "Failed to create socketpair: %s", strerror(errno));
		return -1;
	}
	
	if ((helper_pid = fork()) == 0)
	{
		/* Child */
		char sockfdstr[10];
		close(sockets[1]);
		sprintf(sockfdstr, "%d", sockets[0]);
		execlp("audacious-arts-helper", "audacious-arts-helper",
		       sockfdstr, NULL);
		g_warning("artsxmms_start_helper(): "
			  "Failed to start audacious-arts-helper: %s", strerror(errno));
		close(sockets[0]);
		_exit(1);
	}
	close(sockets[0]);
	helperfd = sockets[1];

	if (helper_pid < 0)
	{
		g_message("artsxmms_start_helper(): "
			  "Failed to fork() helper process: %s", strerror(errno));
		close(sockets[1]);
		return -1;
	}

	return 0;
}

int artsxmms_open(AFormat fmt, int rate, int nch)
{
	if (artsxmms_start_helper() < 0)
		return 0;

	artsxmms_set_params(&input_params, fmt, rate, nch);
	artsxmms_set_params(&output_params, fmt, rate, nch);

	arts_convert_func = arts_get_convert_func(output_params.format);
	
	written = 0;
	paused = 0;
	helper_failed = FALSE;

	if (artsxmms_helper_init(&output_params)) {
		artsxmms_close();
		return 0;
	}
	artsxmms_set_volume(volume.left, volume.right);

	going = 1;
	return 1;
}

void artsxmms_get_volume(int *l, int *r)
{
	*l = volume.left;
	*r = volume.right;
}

void artsxmms_set_volume(int l, int r)
{
	int vol[2];
	volume.left = l;
	volume.right = r;
	vol[0] = l;
	vol[1] = r;
	helper_cmd_data(CMD_SET_VOLUME, 0, vol, sizeof(vol));
}
