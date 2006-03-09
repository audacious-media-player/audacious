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

#include "arts_helper.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>

#include <artsc.h>

#define FALSE 0
#define TRUE (!FALSE)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#ifdef WORDS_BIGENDIAN
#define INT16_FROM_LE(val) ((val & 0xff) << 8 | (val & 0xff00) >> 8)
#define INT16_TO_LE(val)  ((val & 0xff) << 8 | (val & 0xff00) >> 8)
#else
#define INT16_FROM_LE(val) (val)
#define INT16_TO_LE(val) (val)
#endif

/* This is not quite portable, but should be ok anyway */
typedef short int16;

static arts_stream_t handle;

static int going, paused, inited;
static struct params_info output_params;

struct {
	int left, right;
} volume = {100, 100};

struct params_info
{
	int resolution;
	int frequency;
	int channels;

	/* Cache */
	int bps;
};

static struct {
	unsigned int bsize, latency, psize;
} arts_params;

static struct {
	char *ptr;
	int size;
	int rd, wr;
} ring_buffer;

static void artsxmms_set_params(struct params_info *params, int resolution, int rate, int nch)
{
	params->frequency = rate;
	params->channels = nch;

	params->bps = rate * nch;
	params->resolution = resolution;
	if (resolution == 16)
		params->bps *= 2;
}

static int artsxmms_free(void)
{
	if (ring_buffer.rd > ring_buffer.wr)
		return ring_buffer.rd - ring_buffer.wr - 1;
	return ring_buffer.size - (ring_buffer.wr - ring_buffer.rd);
}

static int artsxmms_buffer_used(void)
{
	return ring_buffer.size - artsxmms_free();
}

static int artsxmms_get_output_latency(void) 
{
	int latency;
	long long w;
	
	if (!going)
		return 0;

	w = artsxmms_buffer_used();
	w += arts_params.bsize - arts_stream_get(handle, ARTS_P_BUFFER_SPACE);
	latency =  (w * 1000) / output_params.bps;

	if (!paused)
		latency += arts_params.latency;

	return latency;
}

static int artsxmms_playing(void)
{
	if (!going)
		return FALSE;
	
	if (!paused)
	{
		int t;
		t = arts_stream_get(handle, ARTS_P_BUFFER_SPACE);
		return t < arts_params.bsize - arts_params.psize;
	}

	return TRUE;
}

static void artsxmms_open_stream(struct params_info *params, int buffer_time)
{
	int buffersize = buffer_time * params->bps / 1000;
	handle = arts_play_stream(params->frequency, params->resolution,
				  params->channels, "XMMS");
	arts_params.bsize = arts_stream_set(handle, ARTS_P_BUFFER_SIZE,
					    buffersize);
	arts_params.latency = arts_stream_get(handle, ARTS_P_SERVER_LATENCY);
	arts_params.psize = arts_stream_get(handle, ARTS_P_PACKET_SIZE);
}

static void artsxmms_set_volume(int l, int r)
{
	volume.left = l;
	volume.right = r;
}

static void volume_adjust(void* data, int length)
{
	int i;

	if ((volume.left == 100 && volume.right == 100) ||
	    (output_params.channels == 1 &&
	     (volume.left == 100 || volume.right == 100)))
		return;

	if (output_params.resolution == 16)
	{
		int16 *ptr = data;
		if (output_params.channels == 2)
			for (i = 0; i < length; i += 4)
			{
				*ptr = INT16_TO_LE(INT16_FROM_LE(*ptr) *
						   volume.left / 100);
				ptr++;
				*ptr = INT16_TO_LE(INT16_FROM_LE(*ptr) *
						   volume.right / 100);
				ptr++;
			}
		else
		{
			int vol = MAX(volume.left, volume.right);
			for (i = 0; i < length; i += 2, ptr++)
			{
				*ptr = INT16_TO_LE(INT16_FROM_LE(*ptr) *
						   vol / 100);
			}
		}
	}
	else
	{
		unsigned char *ptr = data;
		if (output_params.channels == 2)
			for (i = 0; i < length; i += 2)
			{
				*ptr = *ptr * volume.left / 100;
				ptr++;
				*ptr = *ptr * volume.right / 100;
				ptr++;
			}
		else
		{
			int vol = MAX(volume.left, volume.right);
			for (i = 0; i < length; i++, ptr++)
			{
				*ptr = *ptr * vol / 100;
			}
		}
	}
}

static void artsxmms_write(char *ptr, int length)
{
	int cnt;

	/* FIXME: Check that length is not too large? */
	while (length > 0)
	{
		cnt = MIN(length, ring_buffer.size - ring_buffer.wr);
		memcpy(ring_buffer.ptr + ring_buffer.wr, ptr, cnt);
		ring_buffer.wr = (ring_buffer.wr + cnt) % ring_buffer.size;
		length -= cnt;
		ptr += cnt;
	}
}

static void artsxmms_write_arts(void)
{
	int ret, cnt, space;
	char *ptr;

	if (ring_buffer.wr == ring_buffer.rd || paused)
		return;
	
	space = arts_stream_get(handle, ARTS_P_BUFFER_SPACE);

	while (space > 0 && ring_buffer.wr != ring_buffer.rd)
	{
		if (ring_buffer.wr > ring_buffer.rd)
			cnt = MIN(space, ring_buffer.wr - ring_buffer.rd);
		else
			cnt = MIN(space, ring_buffer.size - ring_buffer.rd);

		ptr = ring_buffer.ptr + ring_buffer.rd;

		volume_adjust(ptr, cnt);
		ret = arts_write(handle, ptr, cnt);
		if (ret < 0)
		{
			/* FIXME: handle this better? */
			fprintf(stderr, "artsxmms_write(): write error: %s\n",
				arts_error_text(ret));
			return;
		}

		ring_buffer.rd = (ring_buffer.rd + cnt) % ring_buffer.size;
		space -= cnt;
	}
}

static void artsxmms_close(void)
{
	going = 0;
	arts_close_stream(handle);
	arts_free();
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
	return count - left;
}


static int init_ring_buffer(int size)
{
	free(ring_buffer.ptr);
	/* Make the ring buffer always end on a sample boundary */
	size -= size % 4;
	ring_buffer.size = size;
	ring_buffer.ptr = malloc(size);
	ring_buffer.rd = 0;
	ring_buffer.wr = 0;
	if (ring_buffer.ptr == NULL)
		return -1;
	return 0;
}

static int helper_init(struct init_data *init)
{
	int buffer_time = MAX(init->buffer_time, 50);
	if (init->version != HELPER_VERSION) {
		fprintf(stderr,
			"Fatal: Version mismatch between arts output plugin and\n"
			"       audacious-arts-helper program.\n");
		return -1;
	}
	if (!inited)
		return -1;
	artsxmms_set_params(&output_params, init->resolution, init->rate,
			    init->nchannels);

	if (init_ring_buffer((buffer_time * 2 * output_params.bps) / 1000))
		return -1;

	if (handle)
		arts_close_stream(handle);
	artsxmms_open_stream(&output_params, buffer_time);

	going = 1;
	return 0;
}

static int process_cmd(int fd)
{
	struct command inp;
	struct response outp;
	void *data = NULL;
	int retval = 0;

	if (read_all(fd, &inp, sizeof(inp)) != sizeof(inp)) {
		fprintf(stderr, "read short, giving up\n");
		return -1;
	}
	if (inp.data_length > 0) {
		data = malloc(inp.data_length);
		if (data == NULL)
			return -1;
		if (read_all(fd, data, inp.data_length) != inp.data_length) {
			fprintf(stderr, "data read short, giving up\n");
			return -1;
		}
	}
	outp.cmd = inp.cmd;
	outp.status = STATUS_OK;
	outp.data = 0;
/*  	fprintf(stderr, "Recieved %d; ", inp.cmd); */
	switch (inp.cmd) {
		case CMD_QUIT:
			artsxmms_close();
			retval = 1;
			break;
		case CMD_INIT:
			if (inp.data_length != sizeof (struct init_data))
				outp.status = STATUS_FAILED;
			else if (helper_init(data))
				outp.status = STATUS_FAILED;
			break;
		case CMD_PAUSE:
			paused = inp.data;
			break;
		case CMD_SET_VOLUME: {
			int *vol = data;
			if (inp.data_length < 2 * sizeof(int)) {
				outp.status = STATUS_FAILED;
				break;
			}
			artsxmms_set_volume(vol[0], vol[1]);
			break;
		}
		case CMD_WRITE:
			artsxmms_write(data, inp.data_length);
			break;
		case CMD_FREE:
			outp.data = artsxmms_free();
			break;
		case CMD_GET_OUTPUT_LATENCY:
			outp.data = artsxmms_get_output_latency();
			break;
		case CMD_QUERY_PLAYING:
			outp.data = artsxmms_playing();
			break;
		default:
			outp.status = STATUS_UNKNOWN;
			fprintf(stderr, "Unknown command %d\n", inp.cmd);
	}
	free(data);
	if (write_all(fd, &outp, sizeof (outp)) != sizeof (outp))
		return -1;
	return retval;
}


static int main_loop(int fd)
{
	int retval = 0, sr;
	struct timeval timeout;
	fd_set rdfs;

	for (;;) {
		FD_ZERO(&rdfs);
		FD_SET(fd, &rdfs);
		timeout.tv_sec = 0;
		timeout.tv_usec = 20000;
		sr = select(fd + 1, &rdfs, NULL, NULL, &timeout);
		if (sr < 0) {
			fprintf(stderr, "audacious-arts-helper select failed: %s\n",
				strerror(errno));
			retval = -1;
			break;
		} else if (sr) {
			int p = process_cmd(fd);
			if (p < 0) {
				fprintf(stderr, "cmd failed\n");
				retval = 1;
				break;
			} else if (p)
				break;
		}

		artsxmms_write_arts();
	}
	return retval;
}

int main(int argc, char **argv)
{
	int fd, err, ret;

	if (argc != 2 || (fd = atoi(argv[1])) < 1)
	{
		fprintf(stderr, "Usage: audacious-arts-helper fd\n");
		return 1;
	}

	inited = 1;

	if ((err = arts_init()) != 0)
	{
		fprintf(stderr, "artsxmms_open(): Unable to initialize aRts: %s\n",
			  arts_error_text(err));
		inited = 0;
	}

	ret = main_loop(fd);
	close(fd);
/* 	fprintf(stderr, "helper exits\n"); */
	return ret < 0;
}
