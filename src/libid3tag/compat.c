/* C code produced by gperf version 2.7 */
/* Command-line: gperf -tCcTonD -K id -N id3_compat_lookup -s -3 -k * compat.gperf  */
/*
 * libid3tag - ID3 tag manipulation library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Id: compat.gperf,v 1.11 2004/01/23 09:41:32 rob Exp 
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include <stdlib.h>
# include <string.h>

# ifdef HAVE_ASSERT_H
#  include <assert.h>
# endif

# include "id3tag.h"
# include "compat.h"
# include "frame.h"
# include "field.h"
# include "parse.h"
# include "ucs4.h"

# define EQ(id)    #id, 0
# define OBSOLETE    0, 0
# define TX(id)    #id, translate_##id

static id3_compat_func_t translate_TCON;

#define TOTAL_KEYWORDS 73
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 4
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 84
/* maximum key range = 84, duplicates = 10 */

#ifdef __GNUC__
__inline
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned char asso_values[] =
    {
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 22,
      21, 27, 26, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85,  9,  3,  0, 27, 16,
       6, 30, 85, 15, 85, 22,  2, 15,  4,  1,
       0, 30, 13, 17, 22,  0, 24,  5, 31, 25,
      15, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85, 85, 85, 85, 85,
      85, 85, 85, 85, 85, 85
    };
  register int hval = 0;

  switch (len)
    {
      default:
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
const struct id3_compat *
id3_compat_lookup (str, len)
     register const char *str;
     register unsigned int len;
{
  static const struct id3_compat wordlist[] =
    {
      {"POP",  EQ(POPM)  /* Popularimeter */},
      {"WCP",  EQ(WCOP)  /* Copyright/legal information */},
      {"WPB",  EQ(WPUB)  /* Publishers official webpage */},
      {"BUF",  EQ(RBUF)  /* Recommended buffer size */},
      {"PIC",  EQ(APIC)  /* Attached picture */},
      {"COM",  EQ(COMM)  /* Comments */},
      {"IPL",  EQ(TIPL)  /* Involved people list */},
      {"MLL",  EQ(MLLT)  /* MPEG location lookup table */},
      {"WAF",  EQ(WOAF)  /* Official audio file webpage */},
      {"WCM",  EQ(WCOM)  /* Commercial information */},
      {"UFI",  EQ(UFID)  /* Unique file identifier */},
      {"CRA",  EQ(AENC)  /* Audio encryption */},
      {"TCO",  TX(TCON)  /* Content type */},
      {"ULT",  EQ(USLT)  /* Unsynchronised lyric/text transcription */},
      {"TOL",  EQ(TOLY)  /* Original lyricist(s)/text writer(s) */},
      {"TBP",  EQ(TBPM)  /* BPM (beats per minute) */},
      {"TPB",  EQ(TPUB)  /* Publisher */},
      {"CNT",  EQ(PCNT)  /* Play counter */},
      {"TCON", TX(TCON)  /* Content type */},
      {"WAR",  EQ(WOAR)  /* Official artist/performer webpage */},
      {"LNK",  EQ(LINK)  /* Linked information */},
      {"CRM",  OBSOLETE  /* Encrypted meta frame [obsolete] */},
      {"TOF",  EQ(TOFN)  /* Original filename */},
      {"MCI",  EQ(MCDI)  /* Music CD identifier */},
      {"TPA",  EQ(TPOS)  /* Part of a set */},
      {"WAS",  EQ(WOAS)  /* Official audio source webpage */},
      {"TOA",  EQ(TOPE)  /* Original artist(s)/performer(s) */},
      {"TAL",  EQ(TALB)  /* Album/movie/show title */},
      {"TLA",  EQ(TLAN)  /* Language(s) */},
      {"IPLS", EQ(TIPL)  /* Involved people list */},
      {"TCR",  EQ(TCOP)  /* Copyright message */},
      {"TRC",  EQ(TSRC)  /* ISRC (international standard recording code) */},
      {"TOR",  EQ(TDOR)  /* Original release year [obsolete] */},
      {"TCM",  EQ(TCOM)  /* Composer */},
      {"ETC",  EQ(ETCO)  /* Event timing codes */},
      {"STC",  EQ(SYTC)  /* Synchronised tempo codes */},
      {"TLE",  EQ(TLEN)  /* Length */},
      {"SLT",  EQ(SYLT)  /* Synchronised lyric/text */},
      {"TEN",  EQ(TENC)  /* Encoded by */},
      {"TP2",  EQ(TPE2)  /* Band/orchestra/accompaniment */},
      {"TP1",  EQ(TPE1)  /* Lead performer(s)/soloist(s) */},
      {"TOT",  EQ(TOAL)  /* Original album/movie/show title */},
      {"EQU",  OBSOLETE  /* Equalization [obsolete] */},
      {"RVA",  OBSOLETE  /* Relative volume adjustment [obsolete] */},
      {"GEO",  EQ(GEOB)  /* General encapsulated object */},
      {"TP4",  EQ(TPE4)  /* Interpreted, remixed, or otherwise modified by */},
      {"TP3",  EQ(TPE3)  /* Conductor/performer refinement */},
      {"TFT",  EQ(TFLT)  /* File type */},
      {"TIM",  OBSOLETE  /* Time [obsolete] */},
      {"REV",  EQ(RVRB)  /* Reverb */},
      {"TSI",  OBSOLETE  /* Size [obsolete] */},
      {"EQUA", OBSOLETE  /* Equalization [obsolete] */},
      {"TSS",  EQ(TSSE)  /* Software/hardware and settings used for encoding */},
      {"TRK",  EQ(TRCK)  /* Track number/position in set */},
      {"TDA",  OBSOLETE  /* Date [obsolete] */},
      {"TMT",  EQ(TMED)  /* Media type */},
      {"TKE",  EQ(TKEY)  /* Initial key */},
      {"TORY", EQ(TDOR)  /* Original release year [obsolete] */},
      {"TRD",  OBSOLETE  /* Recording dates [obsolete] */},
      {"TYE",  OBSOLETE  /* Year [obsolete] */},
      {"TT2",  EQ(TIT2)  /* Title/songname/content description */},
      {"TT1",  EQ(TIT1)  /* Content group description */},
      {"WXX",  EQ(WXXX)  /* User defined URL link frame */},
      {"TIME", OBSOLETE  /* Time [obsolete] */},
      {"TSIZ", OBSOLETE  /* Size [obsolete] */},
      {"TT3",  EQ(TIT3)  /* Subtitle/description refinement */},
      {"TRDA", OBSOLETE  /* Recording dates [obsolete] */},
      {"RVAD", OBSOLETE  /* Relative volume adjustment [obsolete] */},
      {"TDY",  EQ(TDLY)  /* Playlist delay */},
      {"TXT",  EQ(TEXT)  /* Lyricist/text writer */},
      {"TYER", OBSOLETE  /* Year [obsolete] */},
      {"TDAT", OBSOLETE  /* Date [obsolete] */},
      {"TXX",  EQ(TXXX)  /* User defined text information frame */}
    };

  static const short lookup[] =
    {
        -1,    0,   -1,  -53,   -2,    1,  -49,   -2,
         2,    3,   -1,  -46,   -2,  -43,   -2,    4,
         5,    6,   -1,    7, -163,   10,   11,   12,
        13, -161,   17, -159,  -77,   22,   23,  -80,
        26,  -85,   29,  -87,   32,   33,   34,   35,
        36,   37,   38,   39,   40,   41, -155,   44,
        45,   46,   47,   -1,   48,   49,   50,   51,
        52,   53,   54,   55,   56,   57,   58,   59,
        -1,   60,   61,   62,   63,   64,   -1, -151,
        -1,   67,   68,   69,   70,   -8,   -2,   -1,
        71,  -31,   -2,   -1,   72,  -55,   -2,  -59,
        -3,  -65,   -2
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist[index].id;

              if (*str == *s && !strncmp (str + 1, s + 1, len - 1))
                return &wordlist[index];
            }
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register const struct id3_compat *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
              register const struct id3_compat *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  register const char *s = wordptr->id;

                  if (*str == *s && !strncmp (str + 1, s + 1, len - 1))
                    return wordptr;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}

static
int translate_TCON(struct id3_frame *frame, char const *oldid,
		   id3_byte_t const *data, id3_length_t length)
{
  id3_byte_t const *end;
  enum id3_field_textencoding encoding;
  id3_ucs4_t *string = 0, *ptr, *endptr;
  int result = 0;

  /* translate old TCON syntax into multiple strings */

  assert(frame->nfields == 2);

  encoding = ID3_FIELD_TEXTENCODING_ISO_8859_1;

  end = data + length;

  if (id3_field_parse(&frame->fields[0], &data, end - data, &encoding) == -1)
    goto fail;

  string = id3_parse_string(&data, end - data, encoding, 0);
  if (string == 0)
    goto fail;

  ptr = string;
  while (*ptr == '(') {
    if (*++ptr == '(')
      break;

    endptr = ptr;
    while (*endptr && *endptr != ')')
      ++endptr;

    if (*endptr)
      *endptr++ = 0;

    if (id3_field_addstring(&frame->fields[1], ptr) == -1)
      goto fail;

    ptr = endptr;
  }

  if (*ptr && id3_field_addstring(&frame->fields[1], ptr) == -1)
    goto fail;

  if (0) {
  fail:
    result = -1;
  }

  if (string)
    free(string);

  return result;
}

/*
 * NAME:	compat->fixup()
 * DESCRIPTION:	finish compatibility translations
 */
int id3_compat_fixup(struct id3_tag *tag)
{
  struct id3_frame *frame;
  unsigned int index;
  id3_ucs4_t timestamp[17] = { 0 };
  int result = 0;

  /* create a TDRC frame from obsolete TYER/TDAT/TIME frames */

  /*
   * TYE/TYER: YYYY
   * TDA/TDAT: DDMM
   * TIM/TIME: HHMM
   *
   * TDRC: yyyy-MM-ddTHH:mm
   */

  index = 0;
  while ((frame = id3_tag_findframe(tag, ID3_FRAME_OBSOLETE, index++))) {
    char const *id;
    id3_byte_t const *data, *end;
    id3_length_t length;
    enum id3_field_textencoding encoding;
    id3_ucs4_t *string;

    id = id3_field_getframeid(&frame->fields[0]);
    assert(id);

    if (strcmp(id, "TYER") != 0 && strcmp(id, "YTYE") != 0 &&
	strcmp(id, "TDAT") != 0 && strcmp(id, "YTDA") != 0 &&
	strcmp(id, "TIME") != 0 && strcmp(id, "YTIM") != 0)
      continue;

    data = id3_field_getbinarydata(&frame->fields[1], &length);
    assert(data);

    if (length < 1)
      continue;

    end = data + length;

    encoding = id3_parse_uint(&data, 1);
    string   = id3_parse_string(&data, end - data, encoding, 0);

    if (string == 0)
      continue;

    if (id3_ucs4_length(string) < 4) {
      free(string);
      continue;
    }

    if (strcmp(id, "TYER") == 0 ||
	strcmp(id, "YTYE") == 0) {
      timestamp[0] = string[0];
      timestamp[1] = string[1];
      timestamp[2] = string[2];
      timestamp[3] = string[3];
    }
    else if (strcmp(id, "TDAT") == 0 ||
	     strcmp(id, "YTDA") == 0) {
      timestamp[4] = '-';
      timestamp[5] = string[2];
      timestamp[6] = string[3];
      timestamp[7] = '-';
      timestamp[8] = string[0];
      timestamp[9] = string[1];
    }
    else {  /* TIME or YTIM */
      timestamp[10] = 'T';
      timestamp[11] = string[0];
      timestamp[12] = string[1];
      timestamp[13] = ':';
      timestamp[14] = string[2];
      timestamp[15] = string[3];
    }

    free(string);
  }

  if (timestamp[0]) {
    id3_ucs4_t *strings;

    frame = id3_frame_new("TDRC");
    if (frame == 0)
      goto fail;

    strings = timestamp;

    if (id3_field_settextencoding(&frame->fields[0],
				  ID3_FIELD_TEXTENCODING_ISO_8859_1) == -1 ||
	id3_field_setstrings(&frame->fields[1], 1, &strings) == -1 ||
	id3_tag_attachframe(tag, frame) == -1) {
      id3_frame_delete(frame);
      goto fail;
    }
  }

  if (0) {
  fail:
    result = -1;
  }

  return result;
}
