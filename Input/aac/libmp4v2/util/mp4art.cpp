/*
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
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May wmay@cisco.com (from mp4info.cpp)
 */

#include "mp4.h"
#include "mpeg4ip_getopt.h"

static void strip_filename (const char *name, char *buffer)
{
  const char *suffix, *slash;
  if (name != NULL) {
    suffix = strrchr(name, '.');
    slash = strrchr(name, '/');
    if (slash == NULL) slash = name;
    else slash++;
    if (suffix == NULL) 
      suffix = slash + strlen(slash);
    memcpy(buffer, slash, suffix - slash);
    buffer[suffix - slash] = '\0';
  } else {
    strcpy(buffer, "out");
  }
}

int main(int argc, char** argv)
{
  const char* usageString = 
    "<file-name>\n";

  /* begin processing command line */
  char* ProgName = argv[0];
  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "version", 0, 0, 'V' },
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "V",
			 long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case '?':
      fprintf(stderr, "usage: %s %s", ProgName, usageString);
      exit(0);
    case 'V':
      fprintf(stderr, "%s - %s version %s\n", ProgName, 
	      MPEG4IP_PACKAGE, MPEG4IP_VERSION);
      exit(0);
    default:
      fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
	      ProgName, c);
    }
  }

  /* check that we have at least one non-option argument */
  if ((argc - optind) < 1) {
    fprintf(stderr, "usage: %s %s", ProgName, usageString);
    exit(1);
  }

  /* end processing of command line */
  printf("%s version %s\n", ProgName, MPEG4IP_VERSION);

  while (optind < argc) {
    char *mp4FileName = argv[optind++];
    MP4FileHandle mp4file = MP4Read(mp4FileName);
    if (mp4file != MP4_INVALID_FILE_HANDLE) {
      uint8_t *art;
      uint32_t art_size;

      if (MP4GetMetadataCoverArt(mp4file, &art, &art_size)) {
	char filename[MAXPATHLEN];
	strip_filename(mp4FileName, filename);
	strcat(filename, ".png");
	struct stat fstat;
	if (stat(filename, &fstat) == 0) {
	  fprintf(stderr, "file %s already exists\n", filename);
	} else {
	  FILE *ofile = fopen(filename, FOPEN_WRITE_BINARY);
	  if (ofile != NULL) {
	    fwrite(art, art_size, 1, ofile);
	    fclose(ofile);
	    printf("created file %s\n", filename);
	  } else {
	    fprintf(stderr, "couldn't create file %s\n", filename);
	  }
	}
	  
	free(art);
      } else {
	fprintf(stderr, "art not available for %s\n", mp4FileName);
      }
      MP4Close(mp4file);
    }
  }

  return(0);
}

