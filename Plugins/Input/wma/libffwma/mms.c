/* 
 * Copyright (C) 2000-2001 major mms
 * 
 * This file is part of libmms
 * 
 * libmms is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * libmms is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * utility functions to handle communication with an mms server
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>

#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

#include "avcodec.h"
#include "bswap.h"
#include "mms.h"

#define LOG

/* 
 * mms specific types 
 */

#define MMS_PORT 1755

#define BUF_SIZE 102400

#define CMD_HEADER_LEN   48
#define CMD_BODY_LEN   1024

struct mms_s {

  int           s;

  char         *host;
  char         *path;
  char         *file;
  char         *url;

  /* command to send */
  char          scmd[CMD_HEADER_LEN+CMD_BODY_LEN];
  char         *scmd_body; /* pointer to &scmd[CMD_HEADER_LEN] */
  int           scmd_len; /* num bytes written in header */

  char          str[1024]; /* scratch buffer to built strings */

  /* receive buffer */
  char          buf[BUF_SIZE];
  int           buf_size;
  int           buf_read;

  uint8_t       asf_header[8192];
  int           asf_header_len;
  int           asf_header_read;
  int           seq_num;
  int           num_stream_ids;
  int           stream_ids[20];
  int           packet_length;

};

/* network/socket utility functions */

static int host_connect_attempt(struct in_addr ia, int port) {

  int                s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in sin;
  
  if (s==-1) {
    printf ("libmms: socket(): %s\n", strerror(errno));
    return -1;
  }

  sin.sin_family = AF_INET;	
  sin.sin_addr   = ia;
  sin.sin_port   = htons(port);
  
  if (connect(s, (struct sockaddr *)&sin, sizeof(sin))==-1 
      && errno != EINPROGRESS) {
    printf ("libmms: connect(): %s\n", strerror(errno));
    close(s);
    return -1;
  }	
	
  return s;
}

static int host_connect(const char *host, int port) {

  struct hostent *h;
  int i, s;
  
  h=gethostbyname(host);
  if (h==NULL) {
    printf ("libmms: unable to resolve '%s'.\n", host);
    return -1;
  }
	
  for (i=0; h->h_addr_list[i]; i++) {
    struct in_addr ia;
    memcpy (&ia, h->h_addr_list[i],4);
    s = host_connect_attempt(ia, port);
    if(s != -1)
      return s;
  }
  printf ("libmms: unable to connect to '%s'.\n", host);
  return -1;
}

static void put_32 (mms_t *this, uint32_t value) {

  this->scmd[this->scmd_len  ] = value % 256;
  value = value >> 8;
  this->scmd[this->scmd_len+1] = value % 256 ;
  value = value >> 8;
  this->scmd[this->scmd_len+2] = value % 256 ;
  value = value >> 8;
  this->scmd[this->scmd_len+3] = value % 256 ;

  this->scmd_len += 4;
}

static int send_data (int s, char *buf, int len) {
  int total;

  total=0;
  while (total<len){ 
    int n;

    n = write (s, &buf[total], len-total);
    if (n > 0)
      total += n;
    else if (n<0 && errno!=EAGAIN) 
      return total;
  }
  return total;
}

static uint32_t get_32 (char *cmd, int offset) {

  uint32_t ret;

  ret = cmd[offset] ;
  ret |= cmd[offset+1]<<8 ;
  ret |= cmd[offset+2]<<16 ;
  ret |= cmd[offset+3]<<24 ;

  return ret;
}

static void send_command (mms_t *this, int command, uint32_t switches, 
			  uint32_t extra, int length) {
  
  int        len8;
  int        i;

  len8 = (length + (length%8)) / 8;

  this->scmd_len = 0;

  put_32 (this, 0x00000001); /* start sequence */
  put_32 (this, 0xB00BFACE); /* #-)) */
  put_32 (this, length + 32);
  put_32 (this, 0x20534d4d); /* protocol type "MMS " */
  put_32 (this, len8 + 4);
  put_32 (this, this->seq_num); this->seq_num++;
  put_32 (this, 0x0);        /* unknown */
  put_32 (this, 0x0);
  put_32 (this, len8+2);
  put_32 (this, 0x00030000 | command); /* dir | command */
  put_32 (this, switches);
  put_32 (this, extra);

  /* memcpy (&cmd->buf[48], data, length); */

  if (send_data (this->s, this->scmd, length+48) != (length+48)) {
    printf ("libmms: send error\n");
  }

#ifdef LOG
  printf ("\nlibmms: ***************************************************\ncommand sent, %d bytes\n", length+48);

  printf ("start sequence %08x\n", get_32 (this->scmd,  0));
  printf ("command id     %08x\n", get_32 (this->scmd,  4));
  printf ("length         %8x \n", get_32 (this->scmd,  8));
  printf ("len8           %8x \n", get_32 (this->scmd, 16));
  printf ("sequence #     %08x\n", get_32 (this->scmd, 20));
  printf ("len8  (II)     %8x \n", get_32 (this->scmd, 32));
  printf ("dir | comm     %08x\n", get_32 (this->scmd, 36));
  printf ("switches       %08x\n", get_32 (this->scmd, 40));

  printf ("ascii contents>");
  for (i=48; i<(length+48); i+=2) {
    unsigned char c = this->scmd[i];

    if ((c>=32) && (c<=128))
      printf ("%c", c);
    else
      printf (".");
  }
  printf ("\n");

  printf ("libmms: complete hexdump of package follows:\n");
  for (i=0; i<(length+48); i++) {
    unsigned char c = this->scmd[i];

    printf ("%02x", c);

    if ((i % 16) == 15)
      printf ("\nlibmms: ");

    if ((i % 2) == 1)
      printf (" ");

  }
  printf ("\n");
#endif
}

static void string_utf16(char *dest, char *src, int len) {
  int i;

  memset (dest, 0, 1000);

  for (i=0; i<len; i++) {
    dest[i*2] = src[i];
    dest[i*2+1] = 0;
  }

  dest[i*2] = 0;
  dest[i*2+1] = 0;
}

static void print_answer (char *data, int len) {

#ifdef LOG
  int i;

  printf ("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\nanswer received, %d bytes\n", len);

  printf ("start sequence %08x\n", get_32 (data, 0));
  printf ("command id     %08x\n", get_32 (data, 4));
  printf ("length         %8x \n", get_32 (data, 8));
  printf ("len8           %8x \n", get_32 (data, 16));
  printf ("sequence #     %08x\n", get_32 (data, 20));
  printf ("len8  (II)     %8x \n", get_32 (data, 32));
  printf ("dir | comm     %08x\n", get_32 (data, 36));
  printf ("switches       %08x\n", get_32 (data, 40));

  for (i=48; i<len; i+=2) {
    unsigned char c = data[i];
    
    if ((c>=32) && (c<128))
      printf ("%c", c);
    else
      printf (" %02x ", c);
    
  }
  printf ("\n");
#endif

}  

static void get_answer (mms_t *this) {

  int   command = 0x1b;

  while (command == 0x1b) {
    int len;

    len = read (this->s, this->buf, BUF_SIZE) ;
    if (!len) {
      printf ("\nalert! eof\n");
      return;
    }

    print_answer (this->buf, len);

    command = get_32 (this->buf, 36) & 0xFFFF;

    if (command == 0x1b) 
      send_command (this, 0x1b, 0, 0, 0);
  }
}

static int receive (int s, char *buf, size_t count) {

  ssize_t  len, total;

  total = 0;

  while (total < count) {

    len = read (s, &buf[total], count-total);

    if (len<0) {
      perror ("read error:");
      return 0;
    }

    total += len;

#ifdef LOG
    if (len != 0) {
      printf ("[%d/%d]", total, count);
      fflush (stdout);
    }
#endif

  }

  return 1;

}

static void get_header (mms_t *this) {

  char  pre_header[8];
  int            i;

  this->asf_header_len = 0;

  while (1) {

    if (!receive (this->s, pre_header, 8)) {
      printf ("libmms: pre-header read failed\n");
      return ;
    }

#ifdef LOG    
    for (i=0; i<8; i++)
      printf ("libmms: pre_header[%d] = %02x (%d)\n",
	      i, pre_header[i], pre_header[i]);
#endif    

    if (pre_header[4] == 0x02) {
      
      int packet_len;
      
      packet_len = (pre_header[7] << 8 | pre_header[6]) - 8;

#ifdef LOG    
      printf ("libmms: asf header packet detected, len=%d\n",
	      packet_len);
#endif

      if (!receive (this->s, (char*)&this->asf_header[this->asf_header_len], packet_len)) {
	printf ("libmms: header data read failed\n");
	return;
      }

      this->asf_header_len += packet_len;

      if ( (this->asf_header[this->asf_header_len-1] == 1) 
	   && (this->asf_header[this->asf_header_len-2]==1)) {

	printf ("libmms: get header packet finished\n");

	return;

      } 

    } else {

      int packet_len, command;

      if (!receive (this->s, (char *) &packet_len, 4)) {
	printf ("packet_len read failed\n");
	return;
      }
      
      packet_len = get_32 ((char *)&packet_len, 0) + 4;
      
#ifdef LOG    
      printf ("command packet detected, len=%d\n",
	      packet_len);
#endif
      
      if (!receive (this->s, this->buf, packet_len)) {
	printf ("command data read failed\n");
	return ;
      }
      
      command = get_32 (this->buf, 24) & 0xFFFF;
      
#ifdef LOG    
      printf ("command: %02x\n", command);
#endif
      
      if (command == 0x1b) 
	send_command (this, 0x1b, 0, 0, 0);
      
    }

    printf ("mms: get header packet succ\n");
  }
}

static void interp_header (mms_t *this) {

  int i;

  this->packet_length = 0;

  /*
   * parse header
   */

  i = 30;
  while (i<this->asf_header_len) {
    
    uint64_t guid_1, guid_2, length;

    guid_2 = (uint64_t)this->asf_header[i] | ((uint64_t)this->asf_header[i+1]<<8) 
      | ((uint64_t)this->asf_header[i+2]<<16) | ((uint64_t)this->asf_header[i+3]<<24)
      | ((uint64_t)this->asf_header[i+4]<<32) | ((uint64_t)this->asf_header[i+5]<<40)
      | ((uint64_t)this->asf_header[i+6]<<48) | ((uint64_t)this->asf_header[i+7]<<56);
    i += 8;

    guid_1 = (uint64_t)this->asf_header[i] | ((uint64_t)this->asf_header[i+1]<<8) 
      | ((uint64_t)this->asf_header[i+2]<<16) | ((uint64_t)this->asf_header[i+3]<<24)
      | ((uint64_t)this->asf_header[i+4]<<32) | ((uint64_t)this->asf_header[i+5]<<40)
      | ((uint64_t)this->asf_header[i+6]<<48) | ((uint64_t)this->asf_header[i+7]<<56);
    i += 8;
    
#ifdef LOG    
    printf ("guid found: %016llx%016llx\n", guid_1, guid_2);
#endif

    length = (uint64_t)this->asf_header[i] | ((uint64_t)this->asf_header[i+1]<<8) 
      | ((uint64_t)this->asf_header[i+2]<<16) | ((uint64_t)this->asf_header[i+3]<<24)
      | ((uint64_t)this->asf_header[i+4]<<32) | ((uint64_t)this->asf_header[i+5]<<40)
      | ((uint64_t)this->asf_header[i+6]<<48) | ((uint64_t)this->asf_header[i+7]<<56);

    i += 8;

    if ( (guid_1 == 0x6cce6200aa00d9a6) && (guid_2 == 0x11cf668e75b22630) ) {
      printf ("header object\n");
    } else if ((guid_1 == 0x6cce6200aa00d9a6) && (guid_2 == 0x11cf668e75b22636)) {
      printf ("data object\n");
    } else if ((guid_1 == 0x6553200cc000e48e) && (guid_2 == 0x11cfa9478cabdca1)) {

      this->packet_length = get_32((char*)this->asf_header, i+92-24);

#ifdef LOG    
      printf ("file object, packet length = %d (%d)\n",
	      this->packet_length, get_32((char*)this->asf_header, i+96-24));
#endif


    } else if ((guid_1 == 0x6553200cc000e68e) && (guid_2 == 0x11cfa9b7b7dc0791)) {

      int stream_id = this->asf_header[i+48] | this->asf_header[i+49] << 8;

#ifdef LOG    
      printf ("stream object, stream id: %d\n", stream_id);
#endif

      this->stream_ids[this->num_stream_ids] = stream_id;
      this->num_stream_ids++;
      

      /*
	} else if ((guid_1 == 0x) && (guid_2 == 0x)) {
	printf ("??? object\n");
      */
    } else {
      printf ("unknown object\n");
    }

#ifdef LOG    
    printf ("length    : %lld\n", length);
#endif

    i += length-24;

  }
}


mms_t *mms_connect (const char *url_) {

  mms_t *this;
  char  *url, *host, *hostend;
  char  *path, *file;
  int    hostlen, s, len, i;

  /* parse url*/

  if (strncasecmp (url_, "mms://",6)) {
    printf ("libmms: invalid url >%s< (should be mms:// - style)\n", url_);
    return NULL;
  }

  url = strdup (url_);

  /* extract hostname */

  hostend = strchr(&url[6],'/');
  if (!hostend) {
    printf ("libmms: invalid url >%s<, failed to find hostend\n", url);
    return NULL;
  }
  hostlen = hostend - url - 6;
  host = av_malloc (hostlen+1);
  strncpy (host, &url[6], hostlen);
  host[hostlen]=0;

  /* extract path and file */

  path = url+hostlen+7;
  file = strrchr (url, '/');

  /* 
   * try to connect 
   */

  s = host_connect (host, MMS_PORT);
  if (s == -1) {
    printf ("libmms: failed to connect\n");
    free (host);
    free (url);
    return NULL;
  }

  this = (mms_t*) av_malloc (sizeof (mms_t));

  this->url             = url;
  this->host            = host;
  this->path            = path;
  this->file            = file;
  this->s               = s;
  this->seq_num         = 0;
  this->scmd_body       = &this->scmd[CMD_HEADER_LEN];
  this->asf_header_len  = 0;
  this->asf_header_read = 0;
  this->num_stream_ids  = 0;
  this->packet_length   = 0;
  this->buf_size        = 0;
  this->buf_read        = 0;

  /*
   * let the negotiations begin...
   */

  /* cmd1 */
  
  sprintf (this->str, "\034\003NSPlayer/7.0.0.1956; {33715801-BAB3-9D85-24E9-03B90328270A}; Host: %s",
	   this->host);
  string_utf16 (this->scmd_body, this->str, strlen(this->str)+2);

  send_command (this, 1, 0, 0x0004000b, strlen(this->str) * 2+8);

  len = read (this->s, this->buf, BUF_SIZE) ;
  if (len>0)
    print_answer (this->buf, len);
  else {
    printf ("libmms: read failed: %s\n", strerror(errno));

  }

  /* cmd2 */

  string_utf16 (&this->scmd_body[8], "\002\000\\\\192.168.0.129\\TCP\\1037\0000", 
		28);
  memset (this->scmd_body, 0, 8);
  send_command (this, 2, 0, 0, 28*2+8);

  len = read (this->s, this->buf, BUF_SIZE) ;
  if (len)
    print_answer (this->buf, len);

  /* 0x5 */

  string_utf16 (&this->scmd_body[8], path, strlen(path));
  memset (this->scmd_body, 0, 8);
  send_command (this, 5, 0, 0, strlen(path)*2+12);

  get_answer (this);

  /* 0x15 */

  memset (this->scmd_body, 0, 40);
  this->scmd_body[32] = 2;

  send_command (this, 0x15, 1, 0, 40);

  this->num_stream_ids = 0;

  get_header (this);
  interp_header (this);

  /* 0x33 */

  memset (this->scmd_body, 0, 40);

  for (i=1; i<this->num_stream_ids; i++) {
    this->scmd_body [ (i-1) * 6 + 2 ] = 0xFF;
    this->scmd_body [ (i-1) * 6 + 3 ] = 0xFF;
    this->scmd_body [ (i-1) * 6 + 4 ] = this->stream_ids[i];
    this->scmd_body [ (i-1) * 6 + 5 ] = 0x00;
  }

  send_command (this, 0x33, this->num_stream_ids, 
		0xFFFF | this->stream_ids[0] << 16, 
		(this->num_stream_ids-1)*6+2);

  get_answer (this);

  /* 0x07 */

  memset (this->scmd_body, 0, 40);

  for (i=8; i<16; i++)
    this->scmd_body[i] = 0xFF;

  this->scmd_body[20] = 0x04;

  send_command (this, 0x07, 1, 
		0xFFFF | this->stream_ids[0] << 16, 
		24);

  return this;
}

static int get_media_packet (mms_t *this) {

  char  pre_header[8];
  int            i;

  if (!receive (this->s, pre_header, 8)) {
    printf ("pre-header read failed\n");
    return 0;
  }

#ifdef LOG
  for (i=0; i<8; i++)
    printf ("pre_header[%d] = %02x (%d)\n",
	    i, pre_header[i], pre_header[i]);
#endif

  if (pre_header[4] == 0x04) {

    int packet_len;

    packet_len = (pre_header[7] << 8 | pre_header[6]) - 8;

#ifdef LOG
    printf ("asf media packet detected, len=%d\n",
	    packet_len);
#endif

    if (!receive (this->s, this->buf, packet_len)) {
      printf ("media data read failed\n");
      return 0;
    }

    /* implicit padding (with "random" data)*/
    this->buf_size = this->packet_length;

  } else {

    int packet_len, command;

    if (!receive (this->s, (char *)&packet_len, 4)) {
      printf ("packet_len read failed\n");
      return 0;
    }

    packet_len = get_32 ((char *)&packet_len, 0) + 4;

#ifdef LOG
    printf ("command packet detected, len=%d\n",
	    packet_len);
#endif

    if (!receive (this->s, this->buf, packet_len)) {
      printf ("command data read failed\n");
      return 0;
    }

    if ( (pre_header[7] != 0xb0) || (pre_header[6] != 0x0b)
	 || (pre_header[5] != 0xfa) || (pre_header[4] != 0xce) ) {

      printf ("missing signature\n");
      exit (1);

    }

    command = get_32 (this->buf, 24) & 0xFFFF;

#ifdef LOG
    printf ("command: %02x\n", command);
#endif

    if (command == 0x1b) 
      send_command (this, 0x1b, 0, 0, 0);
    else if (command == 0x1e) {

      printf ("libmms: everything done. Thank you for downloading a media file containing proprietary and patentend technology.\n");

      return 0;
    } else if (command != 0x05) {
      printf ("unknown command %02x\n", command);
      exit (1);
    }
  }

#ifdef LOG
  printf ("get media packet succ\n");
#endif

  return 1;
}

int mms_read (mms_t *this, char *data, int len) {

  int total;

  total = 0;

  while (total < len) {

#ifdef LOG
    printf ("libmms: read, got %d / %d bytes\n", total, len);
#endif

    if (this->asf_header_read < this->asf_header_len) {

      int n, bytes_left ;

      bytes_left = this->asf_header_len - this->asf_header_read ;

      if (len<bytes_left)
	n = len;
      else
	n = bytes_left;

      memcpy (&data[total], &this->asf_header[this->asf_header_read], n);

      this->asf_header_read += n;
      total += n;
    } else {

      int n, bytes_left ;

      bytes_left = this->buf_size - this->buf_read;

      while (!bytes_left) {
	
	this->buf_read = 0;

	if (!get_media_packet (this)) {
	  printf ("libmms: get_media_packet failed\n");
	  return total;
	}
	bytes_left = this->buf_size - this->buf_read;
      }
      

      if (len<bytes_left)
	n = len;
      else
	n = bytes_left;

      memcpy (&data[total], &this->buf[this->buf_read], n);

      this->buf_read += n;
      total += n;
    }
  }

  return total;
}

void mms_close (mms_t *this) {

  if (this->s >= 0) {
    close(this->s);
  }

  free (this->host);
  free (this->url);
  free (this);
}
