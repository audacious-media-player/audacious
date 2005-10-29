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

// N.B. mp4clips clips tracks in an mp4 file
// without regard to sync samples aka "key frames"
// as a testing app the burden is on the user to choose
// an appropriate clip start time

#include "mp4.h"
#include "mpeg4ip_getopt.h"

char* ProgName;

// forward declaration
void ClipTrack(
	MP4FileHandle srcFile, 
	MP4TrackId trackId, 
	MP4FileHandle dstFile,
	float startTime,
	float duration);


int main(int argc, char** argv)
{
	char* usageString = 
		"usage: %s [-t <track-id>] [-v [<level>]] [-s <start>] -l <duration> <file-name>\n";
	char* srcFileName = NULL;
	char* dstFileName = NULL;
	MP4TrackId trackId = MP4_INVALID_TRACK_ID;
	u_int32_t verbosity = MP4_DETAILS_ERROR;
	float startTime = 0.0;
	float duration = 0.0;

	/* begin processing command line */
	ProgName = argv[0];
	while (true) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "length", 1, 0, 'l' },
			{ "start", 1, 0, 's' },
			{ "track", 1, 0, 't' },
			{ "verbose", 2, 0, 'v' },
			{ "version", 0, 0, 'V' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "l:s:t:v::V",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'l':
			if (sscanf(optarg, "%f", &duration) != 1) {
				fprintf(stderr, 
					"%s: bad length specified: %s\n",
					 ProgName, optarg);
			}
			break;
		case 's':
			if (sscanf(optarg, "%f", &startTime) != 1) {
				fprintf(stderr, 
					"%s: bad start time specified: %s\n",
					 ProgName, optarg);
			}
			break;
		case 't':
			if (sscanf(optarg, "%u", &trackId) != 1) {
				fprintf(stderr, 
					"%s: bad track-id specified: %s\n",
					 ProgName, optarg);
				exit(1);
			}
			break;
		case 'v':
			verbosity |= MP4_DETAILS_READ;
			if (optarg) {
				u_int32_t level;
				if (sscanf(optarg, "%u", &level) == 1) {
					if (level >= 2) {
						verbosity |= MP4_DETAILS_TABLE;
					} 
					if (level >= 3) {
						verbosity |= MP4_DETAILS_SAMPLE;
					} 
					if (level >= 4) {
						verbosity = MP4_DETAILS_ALL;
					}
				}
			}
			break;
		case '?':
			fprintf(stderr, usageString, ProgName);
			exit(0);
		case 'V':
		  fprintf(stderr, "%s - %s version %s\n", 
			  ProgName, MPEG4IP_PACKAGE, MPEG4IP_VERSION);
		  exit(0);
		default:
			fprintf(stderr, "%s: unknown option specified, ignoring: %c\n", 
				ProgName, c);
		}
	}

	/* check that we have at least one non-option argument */
	if ((argc - optind) < 1) {
		fprintf(stderr, usageString, ProgName);
		exit(1);
	}
	
	if (verbosity) {
		fprintf(stderr, "%s version %s\n", ProgName, MPEG4IP_VERSION);
	}

	/* point to the specified file name */
	srcFileName = argv[optind++];

	/* get dest file name */
	if ((argc - optind) > 0) {
		dstFileName = argv[optind++];
	}

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", ProgName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	if (duration == 0.0) {
		fprintf(stderr,
			"%s: please specify clip length with -l option\n",
			ProgName);
	}

	/* end processing of command line */


	MP4FileHandle srcFile = 
		MP4Modify(srcFileName, verbosity);

	if (!srcFile) {
		exit(1);
	}

	MP4FileHandle dstFile = 
		MP4_INVALID_FILE_HANDLE;
 
	if (dstFileName) {
		dstFile = MP4Create(dstFileName, verbosity);
	}

	if (trackId == MP4_INVALID_TRACK_ID) {
		u_int32_t numTracks = MP4GetNumberOfTracks(srcFile);

		for (u_int32_t i = 0; i < numTracks; i++) {
			trackId = MP4FindTrackId(srcFile, i);
			ClipTrack(srcFile, trackId, dstFile, startTime, duration);
		}
	} else {
		ClipTrack(srcFile, trackId, dstFile, startTime, duration);
	}

	MP4Close(srcFile);
	if (dstFile != MP4_INVALID_FILE_HANDLE) {
		MP4Close(dstFile);
	}

	return(0);
}

void ClipTrack(
	MP4FileHandle srcFile, 
	MP4TrackId trackId, 
	MP4FileHandle dstFile,
	float startTime,
	float duration)
{
	MP4Timestamp trackStartTime =
		MP4ConvertToTrackTimestamp(
			srcFile,
			trackId,
			(u_int64_t)(startTime * 1000),
			MP4_MSECS_TIME_SCALE);

	MP4Duration trackDuration =
		MP4ConvertToTrackDuration(
			srcFile,
			trackId,
			(u_int64_t)(duration * 1000),
			MP4_MSECS_TIME_SCALE);

	MP4EditId editId = 
		MP4AddTrackEdit(
			srcFile, 
			trackId,
			1,
			trackStartTime,
			trackDuration);

	if (editId == MP4_INVALID_EDIT_ID) {
		fprintf(stderr,
			"%s: can't create track edit\n",
			ProgName);
		return;
	}

	if (dstFile) {
		MP4CopyTrack(
			srcFile,
			trackId,
			dstFile,
			true);

		MP4DeleteTrackEdit(
			srcFile,
			trackId, 
			editId);
	}
}

