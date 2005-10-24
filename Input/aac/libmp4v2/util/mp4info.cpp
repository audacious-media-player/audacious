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
 * Copyright (C) Cisco Systems Inc. 2001-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4.h"
#include "mpeg4ip_getopt.h"


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

		printf("%s:\n", mp4FileName);

		char* info = MP4FileInfo(mp4FileName);

		if (!info) {
			fprintf(stderr, 
				"%s: can't open %s\n", 
				ProgName, mp4FileName);
			continue;
		}

		fputs(info, stdout);
		MP4FileHandle mp4file = MP4Read(mp4FileName);
		if (mp4file != MP4_INVALID_FILE_HANDLE) {
		  char *value;
		  uint16_t numvalue, numvalue2;
		  if (MP4GetMetadataName(mp4file, &value) && value != NULL) {
		    fprintf(stdout, " Metadata Name: %s\n", value);
		    free(value);
		  }
		  if (MP4GetMetadataArtist(mp4file, &value) && value != NULL) {
		    fprintf(stdout, " Metadata Artist: %s\n", value);
		    free(value);
		  }
		  if (MP4GetMetadataWriter(mp4file, &value) && value != NULL) {
		    fprintf(stdout, " Metadata Writer: %s\n", value);
		    free(value);
		  }
		  if (MP4GetMetadataComment(mp4file, &value) && value != NULL) {
		    fprintf(stdout, " Metadata Comment: %s\n", value);
		    free(value);
		  }
		  if (MP4GetMetadataTool(mp4file, &value) && value != NULL) {
		    fprintf(stdout, " Metadata Tool: %s\n", value);
		    free(value);
		  }
		  if (MP4GetMetadataYear(mp4file, &value) && value != NULL) {
		    fprintf(stdout, " Metadata Year: %s\n", value);
		    free(value);
		  }
		  if (MP4GetMetadataAlbum(mp4file, &value) && value != NULL) {
		    fprintf(stdout, " Metadata Album: %s\n", value);
		    free(value);
		  }
		  if (MP4GetMetadataTrack(mp4file, &numvalue, &numvalue2)) {
		    fprintf(stdout, " Metadata track: %u of %u\n", numvalue,
			    numvalue2);
		  }
		  if (MP4GetMetadataDisk(mp4file, &numvalue, &numvalue2)) {
		    fprintf(stdout, " Metadata Disk: %u of %u\n", numvalue,
			    numvalue2);
		  }
		  if (MP4GetMetadataGenre(mp4file, &value) && value != NULL) {
		    fprintf(stdout, " Metadata Genre: %s\n", value);
		    free(value);
		  }
		  if (MP4GetMetadataGrouping(mp4file, &value) && value != NULL) {
		    fprintf(stdout, " Metadata Grouping: %s\n", value);
		    free(value);
		  }
		  if (MP4GetMetadataTempo(mp4file, &numvalue)) {
		    fprintf(stdout, " Metadata Tempo: %u\n", numvalue);
		  }
		  MP4Close(mp4file);
		}
		free(info);
	}

	return(0);
}

