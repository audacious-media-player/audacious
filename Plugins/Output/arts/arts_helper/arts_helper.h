/*
 *  aRts ouput plugin for xmms
 *
 *  Copyright (C) 2000,2003,2004  Haavard Kvaalen <havardk@xmms.org>
 *
 *  Licenced under GNU GPL version 2.
 *
 *  Audacious port by Giacomo Lozito from develia.org
 *
 */

struct command
{
	int cmd;
	int data;
	int data_length;
};

struct response
{
	int cmd;
	int status;
	int data;
};

#define HELPER_VERSION 0x000700

struct init_data
{
	int version;
	int resolution, rate, nchannels;
	int buffer_time;
};

enum {
	CMD_INIT = 1,
	CMD_QUIT,
	CMD_PAUSE,
	CMD_FLUSH,
	CMD_SET_VOLUME,
	CMD_WRITE,
	CMD_FREE,
	CMD_GET_OUTPUT_LATENCY,
	CMD_QUERY_PLAYING,
};

enum {
	STATUS_OK = 0,
	STATUS_FAILED,
	STATUS_UNKNOWN,
};

