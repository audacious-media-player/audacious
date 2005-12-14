#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "string.h"

#include "timidity.h"
#include "timidity_internal.h"
#include "common.h"

struct _MidIStream
{
  MidIStreamReadFunc read;
  MidIStreamCloseFunc close;
  void *ctx;
};

typedef struct StdIOContext
{
  FILE *fp;
  int autoclose;
} StdIOContext;

size_t
stdio_istream_read (void *ctx, void *ptr, size_t size, size_t nmemb)
{
  return fread (ptr, size, nmemb, ((StdIOContext *) ctx)->fp);
}

int
stdio_istream_close (void *ctx)
{
  int ret = 0;
  if (((StdIOContext *) ctx)->autoclose)
    ret = fclose (((StdIOContext *) ctx)->fp);
  free (ctx);
  return ret;
}

typedef struct MemContext
{
  sint8 *base;
  sint8 *current;
  sint8 *end;
  int autofree;
} MemContext;

size_t
mem_istream_read (void *ctx, void *ptr, size_t size, size_t nmemb)
{
  MemContext *c;
  size_t count;

  c = (MemContext *) ctx;
  count = nmemb;

  if (c->current + count * size > c->end)
    count = (c->end - c->current) / size;

  memcpy (ptr, c->current, count * size);
  c->current += count * size;

  return count;
}

int
mem_istream_close (void *ctx)
{
  if (((MemContext *) ctx)->autofree)
    free (((MemContext *) ctx)->base);
  free (ctx);
  return 0;
}

MidIStream *
mid_istream_open_fp (FILE * fp, int autoclose)
{
  StdIOContext *ctx;
  MidIStream *stream;

  stream = safe_malloc (sizeof (MidIStream));
  if (stream == NULL)
    return NULL;

  ctx = safe_malloc (sizeof (StdIOContext));
  if (ctx == NULL)
    {
      free (stream);
      return NULL;
    }
  ctx->fp = fp;
  ctx->autoclose = autoclose;

  stream->ctx = ctx;
  stream->read = stdio_istream_read;
  stream->close = stdio_istream_close;

  return stream;
}

MidIStream *
mid_istream_open_file (const char *file)
{
  FILE *fp;

  fp = fopen (file, "rb");
  if (fp == NULL)
    return NULL;

  return mid_istream_open_fp (fp, 1);
}

MidIStream *
mid_istream_open_mem (void *mem, size_t size, int autofree)
{
  MemContext *ctx;
  MidIStream *stream;

  stream = safe_malloc (sizeof (MidIStream));
  if (stream == NULL)
    return NULL;

  ctx = safe_malloc (sizeof (MemContext));
  if (ctx == NULL)
    {
      free (stream);
      return NULL;
    }
  ctx->base = mem;
  ctx->current = mem;
  ctx->end = ((sint8 *) mem) + size;
  ctx->autofree = autofree;

  stream->ctx = ctx;
  stream->read = mem_istream_read;
  stream->close = mem_istream_close;

  return stream;
}

MidIStream *
mid_istream_open_callbacks (MidIStreamReadFunc read,
			    MidIStreamCloseFunc close, void *context)
{
  MidIStream *stream;

  stream = safe_malloc (sizeof (MidIStream));
  if (stream == NULL)
    return NULL;

  stream->ctx = context;
  stream->read = read;
  stream->close = close;

  return stream;
}

size_t
mid_istream_read (MidIStream * stream, void *ptr, size_t size, size_t nmemb)
{
  return stream->read (stream->ctx, ptr, size, nmemb);
}

void
mid_istream_skip (MidIStream * stream, size_t len)
{
  size_t c;
  char tmp[1024];
  while (len > 0)
    {
      c = len;
      if (c > 1024)
	c = 1024;
      len -= c;
      if (c != mid_istream_read (stream, tmp, 1, c))
	{
	  DEBUG_MSG ("mid_istream_skip error\n");
	}
    }
}

int
mid_istream_close (MidIStream * stream)
{
  int ret = stream->close (stream->ctx);
  free (stream);
  return ret;
}
