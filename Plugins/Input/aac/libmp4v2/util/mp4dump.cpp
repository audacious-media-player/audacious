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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4.h"
#include "mpeg4ip_getopt.h"

int main(int argc, char** argv)
{
	char* usageString = "[-verbose=[<level>]] <file-name>\n";
	u_int32_t verbosity = MP4_DETAILS_ERROR;
	bool dumpImplicits = false;

	/* begin processing command line */
	char* progName = argv[0];
	while (true) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "verbose", 2, 0, 'v' },
			{ "version", 0, 0, 'V' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "v::V",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'v':
			verbosity |= MP4_DETAILS_TABLE;
			if (optarg) {
				u_int32_t level;
				if (sscanf(optarg, "%u", &level) == 1) {
					if (level >= 2) {
						dumpImplicits = true;
					} 
					if (level >= 3) {
						verbosity = MP4_DETAILS_ALL;
					}
				}
			}
			break;
		case '?':
			fprintf(stderr, "usage: %s %s", progName, usageString);
			exit(0);
		case 'V':
		  fprintf(stderr, "%s - %s version %s\n", 
			  progName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
		  exit(0);
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
				progName, c);
		}
	}

	/* check that we have at least one non-option argument */
	if ((argc - optind) < 1) {
		fprintf(stderr, "usage: %s %s\n", progName, usageString);
		exit(1);
	}

	/* point to the specified file names */
	char* mp4FileName = argv[optind++];

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", progName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */
	if (verbosity != 0) {
	  fprintf(stdout, "%s version %s\n", progName, MPEG4IP_VERSION);
	}

	MP4FileHandle mp4File = MP4Read(mp4FileName, verbosity);

	if (!mp4File) {
		exit(1);
	}

	MP4Dump(mp4File, stdout, dumpImplicits);

	MP4Close(mp4File);

	return(0);
}

