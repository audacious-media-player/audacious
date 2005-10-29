/* mp4tags -- tool to set iTunes-compatible metadata tags
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * Contributed to MPEG4IP 
 * by Christopher League <league@contrapunctus.net>
 */

#include "mp4.h"
#include "mpeg4ip_getopt.h"

/* One-letter options -- if you want to rearrange these, change them
   here, immediately below in OPT_STRING, and in the help text. */
#define OPT_HELP    'h'
#define OPT_VERSION 'v'
#define OPT_ALBUM   'A'
#define OPT_ARTIST  'a'
#define OPT_TEMPO   'b'
#define OPT_COMMENT 'c'
#define OPT_DISK    'd'
#define OPT_DISKS   'D'
#define OPT_GENRE   'g'
#define OPT_GROUPING 'G'
#define OPT_SONG    's'
#define OPT_TRACK   't'
#define OPT_TRACKS  'T'
#define OPT_WRITER  'w'    /* _composer_ */
#define OPT_YEAR    'y'
#define OPT_REMOVE  'r'

#define OPT_STRING  "hvA:a:b:c:d:D:g:G:s:t:T:w:y:r:"

static const char* help_text =
"OPTION... FILE...\n"
"Adds or modifies iTunes-compatible tags on MP4 files.\n"
"\n"
"  -h, -help         Display this help text and exit\n"
"  -v, -version      Display version information and exit\n"
"  -A, -album    STR  Set the album title\n"
"  -a, -artist   STR  Set the artist information\n"
"  -b, -tempo    NUM  Set the tempo (beats per minute)\n"
"  -c, -comment  STR  Set a general comment\n"
"  -d, -disk     NUM  Set the disk number\n"
"  -D, -disks    NUM  Set the number of disks\n"
"  -g, -genre    STR  Set the genre name\n"
"  -G, -grouping STR  Set the grouping name\n"
"  -s, -song     STR  Set the song title\n"
"  -t, -track    NUM  Set the track number\n"
"  -T, -tracks   NUM  Set the number of tracks\n"
"  -w, -writer   STR  Set the composer information\n"
"  -y, -year     NUM  Set the year\n"
"  -r, -remove   STR  Remove tags by code (e.g. \"-r cs\"\n"
"                     removes the comment and song tags)\n"
"\n";


int 
main(int argc, char** argv)
{
  static struct option long_options[] = {
    { "help",    0, 0, OPT_HELP    },
    { "version", 0, 0, OPT_VERSION },
    { "album",   1, 0, OPT_ALBUM   },
    { "artist",  1, 0, OPT_ARTIST  },
    { "comment", 1, 0, OPT_COMMENT },
    { "disk",    1, 0, OPT_DISK    },
    { "disks",   1, 0, OPT_DISKS   },
    { "genre",   1, 0, OPT_GENRE   },
    { "grouping",1, 0, OPT_GROUPING},
    { "song",    1, 0, OPT_SONG    },
    { "tempo",   1, 0, OPT_TEMPO   },
    { "track",   1, 0, OPT_TRACK   },
    { "tracks",  1, 0, OPT_TRACKS  },
    { "writer",  1, 0, OPT_WRITER  },
    { "year",    1, 0, OPT_YEAR    },
    { "remove",  1, 0, OPT_REMOVE  },
    { NULL,      0, 0, 0 }
  };

  /* Sparse arrays of tag data: some space is wasted, but it's more
     convenient to say tags[OPT_SONG] than to enumerate all the
     metadata types (again) as a struct. */
  char *tags[UCHAR_MAX];
  int nums[UCHAR_MAX];

  memset(tags, 0, sizeof(tags));
  memset(nums, 0, sizeof(nums));

  /* Any modifications requested? */
  int mods = 0;

  /* Option-processing loop. */
  int c = getopt_long_only(argc, argv, OPT_STRING, long_options, NULL);
  while (c != -1) {
    int r = 2;
    switch(c) {
      /* getopt() returns '?' if there was an error.  It already
         printed the error message, so just return. */
    case '?':
      return 1;

      /* Help and version requests handled here. */
    case OPT_HELP:
      fprintf(stderr, "usage %s %s", argv[0], help_text);
      return 0;
    case OPT_VERSION:
      fprintf(stderr, "%s - %s version %s\n", argv[0], MPEG4IP_PACKAGE, 
	      MPEG4IP_VERSION);
      return 0;

      /* Numeric arguments: convert them using sscanf(). */
    case OPT_DISK: case OPT_DISKS:
    case OPT_TRACK: case OPT_TRACKS:
    case OPT_TEMPO:
      r = sscanf(optarg, "%d", &nums[c]);
      if (r < 1) {
        fprintf(stderr, "%s: option requires numeric argument -- %c\n",
                argv[0], c);
        return 2;
      }
      /* Break not, lest ye be broken.  :) */
      /* All arguments: all valid options end up here, and we just
         stuff the string pointer into the tags[] array. */
    default:
      tags[c] = optarg;
      mods++;
    } /* end switch */
    
    c = getopt_long_only(argc, argv, OPT_STRING, long_options, NULL);
  } /* end while */

  /* Check that we have at least one non-option argument */
  if ((argc - optind) < 1) {
    fprintf(stderr, 
            "%s: You must specify at least one MP4 file.\n", 
            argv[0]);
    fprintf(stderr, "usage %s %s", argv[0], help_text);
    return 3;
  }

  /* Check that we have at least one requested modification.  Probably
     it's useful instead to print the metadata if no modifications are
     requested? */
  if (!mods) {
    fprintf(stderr, 
            "%s: You must specify at least one tag modification.\n", 
            argv[0]);
    fprintf(stderr, "usage %s %s", argv[0], help_text);
    return 4;
  }

  /* Loop through the non-option arguments, and modify the tags as
     requested. */ 
  while (optind < argc) {
    char *mp4 = argv[optind++];

    MP4FileHandle h = MP4Modify(mp4);
    if (h == MP4_INVALID_FILE_HANDLE) {
      fprintf(stderr, "Could not open '%s'... aborting\n", mp4);
      return 5;
    }

    /* Remove any tags */
    if( tags[OPT_REMOVE] ) {
      for(const char *p = tags[OPT_REMOVE]; *p; p++ ) {
        switch(*p) {
        case OPT_ALBUM:   MP4DeleteMetadataAlbum(h); break;
        case OPT_ARTIST:  MP4DeleteMetadataArtist(h); break;
        case OPT_COMMENT: MP4DeleteMetadataComment(h); break;
        case OPT_DISK:    MP4DeleteMetadataDisk(h); break;
        case OPT_DISKS:   MP4DeleteMetadataDisk(h); break;
        case OPT_GENRE:   MP4DeleteMetadataGenre(h); break;
        case OPT_GROUPING:MP4DeleteMetadataGrouping(h); break;
        case OPT_SONG:    MP4DeleteMetadataName(h); break;
        case OPT_WRITER:  MP4DeleteMetadataWriter(h); break;
        case OPT_YEAR:    MP4DeleteMetadataYear(h); break;
        case OPT_TEMPO:   MP4DeleteMetadataTempo(h); break;
        case OPT_TRACK:   MP4DeleteMetadataTrack(h); break;
        case OPT_TRACKS:  MP4DeleteMetadataTrack(h); break;
        }
      }
    }

    /* Track/disk numbers need to be set all at once, but we'd like to
       allow users to just specify -T 12 to indicate that all existing
       track numbers are out of 12.  This means we need to look up the
       current info if it is not being set. */
    uint16_t n0, m0, n1, m1;

    if (tags[OPT_TRACK] || tags[OPT_TRACKS]) {
      n0 = m0 = 0;
      MP4GetMetadataTrack(h, &n0, &m0);
      n1 = tags[OPT_TRACK]? nums[OPT_TRACK] : n0;
      m1 = tags[OPT_TRACKS]? nums[OPT_TRACKS] : m0;
      MP4SetMetadataTrack(h, n1, m1);
    }
    if (tags[OPT_DISK] || tags[OPT_DISKS]) {
      n0 = m0 = 0;
      MP4GetMetadataDisk(h, &n0, &m0);
      n1 = tags[OPT_DISK]? nums[OPT_DISK] : n0;
      m1 = tags[OPT_DISKS]? nums[OPT_DISKS] : m0;
      MP4SetMetadataDisk(h, n1, m1);
    }
    
    /* Set the other relevant attributes */
    for (int i = 0;  i < UCHAR_MAX;  i++) {
      if (tags[i]) {
        switch(i) {
        case OPT_ALBUM:   MP4SetMetadataAlbum(h, tags[i]); break;
        case OPT_ARTIST:  MP4SetMetadataArtist(h, tags[i]); break;
        case OPT_COMMENT: MP4SetMetadataComment(h, tags[i]); break;
        case OPT_GENRE:   MP4SetMetadataGenre(h, tags[i]); break;
        case OPT_GROUPING:MP4SetMetadataGrouping(h, tags[i]); break;
        case OPT_SONG:    MP4SetMetadataName(h, tags[i]); break;
        case OPT_WRITER:  MP4SetMetadataWriter(h, tags[i]); break;
        case OPT_YEAR:    MP4SetMetadataYear(h, tags[i]); break;
        case OPT_TEMPO:   MP4SetMetadataTempo(h, nums[i]); break;
        }
      }
    }

    MP4Close(h);
  } /* end while optind < argc */

  return 0;
}

