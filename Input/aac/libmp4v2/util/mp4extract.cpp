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

// N.B. mp4extract just extracts tracks/samples from an mp4 file
// For many track types this is insufficient to reconsruct a valid
// elementary stream (ES). Use "mp4creator -extract=<trackId>" if
// you need the ES reconstructed. 

#include "mp4.h"
#include "mpeg4ip_getopt.h"

char* ProgName;
char* Mp4PathName;
char* Mp4FileName;

// forward declaration
void ExtractTrack(MP4FileHandle mp4File, MP4TrackId trackId, 
	bool sampleMode, MP4SampleId sampleId, char* dstFileName = NULL);

int main(int argc, char** argv)
{
	const char* usageString = 
		"[-l] [-t <track-id>] [-s <sample-id>] [-v [<level>]] <file-name>\n";
	bool doList = false;
	bool doSamples = false;
	MP4TrackId trackId = MP4_INVALID_TRACK_ID;
	MP4SampleId sampleId = MP4_INVALID_SAMPLE_ID;
	char* dstFileName = NULL;
	u_int32_t verbosity = MP4_DETAILS_ERROR;

fprintf(stderr, "You don't want to use this utility - use mp4creator --extract instead\n");
fprintf(stderr, "If you really want to use it, remove this warning and the exit call\n");
fprintf(stderr, "from the source file\n");
	exit(-1);
	/* begin processing command line */
	ProgName = argv[0];
	while (true) {
		int c = -1;
		int option_index = 0;
		static struct option long_options[] = {
			{ "list", 0, 0, 'l' },
			{ "track", 1, 0, 't' },
			{ "sample", 2, 0, 's' },
			{ "verbose", 2, 0, 'v' },
			{ "version", 0, 0, 'V' },
			{ NULL, 0, 0, 0 }
		};

		c = getopt_long_only(argc, argv, "lt:s::v::V",
			long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 'l':
			doList = true;
			break;
		case 's':
			doSamples = true;
			if (optarg) {
				if (sscanf(optarg, "%u", &sampleId) != 1) {
					fprintf(stderr, 
						"%s: bad sample-id specified: %s\n",
						 ProgName, optarg);
				}
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
			fprintf(stderr, "usage: %s %s", ProgName, usageString);
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
		fprintf(stderr, "usage: %s %s", ProgName, usageString);
		exit(1);
	}
	
	if (verbosity) {
		fprintf(stderr, "%s version %s\n", ProgName, MPEG4IP_VERSION);
	}

	/* point to the specified file names */
	Mp4PathName = argv[optind++];

	/* get dest file name for a single track */
	if (trackId && (argc - optind) > 0) {
		dstFileName = argv[optind++];
	}

	char* lastSlash = strrchr(Mp4PathName, '/');
	if (lastSlash) {
		Mp4FileName = lastSlash + 1;
	} else {
		Mp4FileName = Mp4PathName; 
	}

	/* warn about extraneous non-option arguments */
	if (optind < argc) {
		fprintf(stderr, "%s: unknown options specified, ignoring: ", ProgName);
		while (optind < argc) {
			fprintf(stderr, "%s ", argv[optind++]);
		}
		fprintf(stderr, "\n");
	}

	/* end processing of command line */


	MP4FileHandle mp4File = MP4Read(Mp4PathName, verbosity);

	if (!mp4File) {
		exit(1);
	}

	if (doList) {
		MP4Info(mp4File);
		exit(0);
	}

	if (trackId == 0) {
		u_int32_t numTracks = MP4GetNumberOfTracks(mp4File);

		for (u_int32_t i = 0; i < numTracks; i++) {
			trackId = MP4FindTrackId(mp4File, i);
			ExtractTrack(mp4File, trackId, doSamples, sampleId);
		}
	} else {
		ExtractTrack(mp4File, trackId, doSamples, sampleId, dstFileName);
	}

	MP4Close(mp4File);

	return(0);
}

void ExtractTrack(MP4FileHandle mp4File, MP4TrackId trackId, 
	bool sampleMode, MP4SampleId sampleId, char* dstFileName)
{
	char outFileName[PATH_MAX];
	int outFd = -1;
	int openFlags = O_WRONLY | O_TRUNC | OPEN_CREAT;

	if (!sampleMode) {
		if (dstFileName == NULL) {
			snprintf(outFileName, sizeof(outFileName), 
				"%s.t%u", Mp4FileName, trackId);
		} else {
			snprintf(outFileName, sizeof(outFileName), 
				"%s", dstFileName);
		}

		outFd = open(outFileName, openFlags, 0644);
		if (outFd == -1) {
			fprintf(stderr, "%s: can't open %s: %s\n",
				ProgName, outFileName, strerror(errno));
			return;
		}
	}

	MP4SampleId numSamples;

	if (sampleMode && sampleId != MP4_INVALID_SAMPLE_ID) {
		numSamples = sampleId;
	} else {
		sampleId = 1;
		numSamples = MP4GetTrackNumberOfSamples(mp4File, trackId);
	}

	u_int8_t* pSample;
	u_int32_t sampleSize;

	for ( ; sampleId <= numSamples; sampleId++) {
		int rc;

		// signals to ReadSample() that it should malloc a buffer for us
		pSample = NULL;
		sampleSize = 0;

		rc = MP4ReadSample(mp4File, trackId, sampleId, &pSample, &sampleSize);
		if (rc == 0) {
			fprintf(stderr, "%s: read sample %u for %s failed\n",
				ProgName, sampleId, outFileName);
			break;
		}

		if (sampleMode) {
			snprintf(outFileName, sizeof(outFileName), "%s.t%u.s%u",
				Mp4FileName, trackId, sampleId);

			outFd = open(outFileName, openFlags, 0644);

			if (outFd == -1) {
				fprintf(stderr, "%s: can't open %s: %s\n",
					ProgName, outFileName, strerror(errno));
				break;
			}
		}

		rc = write(outFd, pSample, sampleSize);
		if (rc == -1 || (u_int32_t)rc != sampleSize) {
			fprintf(stderr, "%s: write to %s failed: %s\n",
				ProgName, outFileName, strerror(errno));
			break;
		}

		free(pSample);

		if (sampleMode) {
			close(outFd);
			outFd = -1;
		}
	}

	if (outFd != -1) {
		close(outFd);
	}
}

