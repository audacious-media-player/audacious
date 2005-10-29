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

main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		exit(1);
	}

	u_int32_t verbosity = 0 /* MP4_DETAILS_ALL */;

	MP4FileHandle mp4File = MP4Create(argv[1], verbosity);

	if (!mp4File) {
		exit(1);
	}

	MP4TrackId urlTrackId = 
#if 0
		MP4AddTrack(mp4File, "URLF");
#else
	MP4AddHrefTrack(mp4File, 90000, MP4_INVALID_DURATION);
#endif
	printf("urlTrackId %d\n", urlTrackId);

	u_int8_t i;
	char url[128];

	for (i = 1; i <= 5; i++) {
		sprintf(url, "http://server.com/foo/bar%u.html", i);

		MP4WriteSample(mp4File, urlTrackId, 
			(u_int8_t*)url, strlen(url) + 1, (MP4Duration)i);
	}

	MP4Close(mp4File);

	mp4File = MP4Read(argv[1], verbosity);

	// check that we can find the track again
#if 0
	urlTrackId = MP4FindTrackId(mp4File, 0, "URLF");
#else
	urlTrackId = MP4FindTrackId(mp4File, 0, MP4_CNTL_TRACK_TYPE);
#endif
	printf("urlTrackId %d\n", urlTrackId);
	
	for (i = 1; i <= 5; i++) {
		u_int8_t* pSample = NULL;
		u_int32_t sampleSize = 0;
		MP4Duration duration;
		bool rc;

		rc = MP4ReadSample(mp4File, urlTrackId, i,
			&pSample, &sampleSize, NULL, &duration);

		if (rc) {
			printf("Sample %i duration "D64": %s\n", 
				i, duration, pSample);
			free(pSample);
		} else {
			printf("Couldn't read sample %i\n", i);
		}
	}

	MP4Close(mp4File);

	exit(0);
}

