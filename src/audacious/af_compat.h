/*
 * MPlayer libaf compatibility stuff
 */

#ifndef AUDACIOUS_AF_COMPAT_H
#define AUDACIOUS_AF_COMPAT_H

#include <glib.h>
#include "main.h"

/* Number of channels */
#ifndef AF_NCH
#define AF_NCH 6
#endif

/* Format */
#define AF_FORMAT_BE		(0<<0) // Big Endian
#define AF_FORMAT_LE		(1<<0) // Little Endian
#define AF_FORMAT_F		(1<<2) // Foating point
#define AF_FORMAT_32BIT		(3<<3)
#define AF_FORMAT_FLOAT_LE	(AF_FORMAT_F|AF_FORMAT_32BIT|AF_FORMAT_LE)
#define AF_FORMAT_FLOAT_BE	(AF_FORMAT_F|AF_FORMAT_32BIT|AF_FORMAT_BE)

#if G_BYTE_ORDER == G_BIG_ENDIAN // Native endian of cpu
#define AF_FORMAT_FLOAT_NE AF_FORMAT_FLOAT_BE
#else
#define AF_FORMAT_FLOAT_NE AF_FORMAT_FLOAT_LE
#endif

#define AF_MSG_INFO	 0 ///< Important information

#define af_msg(a,...) AUDDBG(__VA_ARGS__);

/* Control */
#define AF_CONTROL_SET			0x00000000
#define AF_CONTROL_GET			0x00000001

#define AF_CONTROL_MANDATORY		0x10000000
#define AF_CONTROL_OPTIONAL		0x20000000
#define AF_CONTROL_FILTER_SPECIFIC	0x40000000

#define AF_CONTROL_REINIT  		0x00000100 | AF_CONTROL_MANDATORY
#define AF_CONTROL_EQUALIZER_GAIN 	0x00001C00 | AF_CONTROL_FILTER_SPECIFIC

/* Return values */
#define AF_DETACH   2
#define AF_OK       1
#define AF_TRUE     1
#define AF_FALSE    0
#define AF_UNKNOWN -1
#define AF_ERROR   -2
#define AF_FATAL   -3

/* Audio data chunk */
typedef struct af_data_s
{
  void* audio;  /* data buffer */
  int len;      /* buffer length */
  int rate;	/* sample rate */
  int nch;	/* number of channels */
  int format;	/* format */
  int bps; 	/* bytes per sample */
} af_data_t;

struct af_instance_s;
/* Audio filter information not specific for current instance, but for
   a specific filter */
typedef struct af_info_s
{
  const char *info;
  const char *name;
  const char *author;
  const char *comment;
  const int flags;
  int (*open)(struct af_instance_s* vf);
} af_info_t;

/* Linked list of audio filters */
typedef struct af_instance_s
{
  af_info_t* info;
  int (*control)(struct af_instance_s* af, int cmd, void* arg);
  void (*uninit)(struct af_instance_s* af);
  af_data_t* (*play)(struct af_instance_s* af, af_data_t* data);
  void* setup;	  // setup data for this specific instance and filter
  af_data_t* data; // configuration for outgoing data stream
  struct af_instance_s* next;
  struct af_instance_s* prev;
  double delay; /* Delay caused by the filter, in units of bytes read without
		 * corresponding output */
  double mul; /* length multiplier: how much does this instance change
		 the length of the buffer. */
}af_instance_t;

/*********************************************
  Extended control used with arguments that operates on only one
  channel at the time
*/
typedef struct af_control_ext_s{
  void* arg;	// Argument
  int	ch;	// Chanel number
}af_control_ext_t;

#endif /* AUDACIOUS_AF_COMPAT_H */
