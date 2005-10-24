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

	u_int32_t verbosity = MP4_DETAILS_ALL;
	char* fileName = argv[1];

	// open the mp4 file, and read meta-info
	MP4FileHandle mp4File = MP4Read(fileName, verbosity);

	u_int8_t profileLevel = MP4GetVideoProfileLevel(mp4File);

	// get a handle on the first video track
	MP4TrackId trackId = MP4FindTrackId(mp4File, 0, "video");

	// gather the crucial track information 

	u_int32_t timeScale = MP4GetTrackTimeScale(mp4File, trackId);

	// note all times and durations 
	// are in units of the track time scale

	MP4Duration trackDuration = MP4GetTrackDuration(mp4File, trackId);

	MP4SampleId numSamples = MP4GetTrackNumberOfSamples(mp4File, trackId);

	u_int32_t maxSampleSize = MP4GetTrackMaxSampleSize(mp4File, trackId);

	u_int8_t* pConfig;
	u_int32_t configSize = 0;

	MP4GetTrackESConfiguration(mp4File, trackId, &pConfig, &configSize);

	// initialize decoder with Elementary Stream (ES) configuration

	// done with our copy of ES configuration
	free(pConfig);


	// now consecutively read and display the track samples

	u_int8_t* pSample = (u_int8_t*)malloc(maxSampleSize);
	u_int32_t sampleSize;
	MP4Timestamp sampleTime;
	MP4Duration sampleDuration;
	MP4Duration sampleRenderingOffset;
	bool isSyncSample;

	for (MP4SampleId sampleId = 1; sampleId <= numSamples; sampleId++) {

		// give ReadSample our own buffer, and let it know how big it is
		sampleSize = maxSampleSize;

		// read next sample from video track
		MP4ReadSample(mp4File, trackId, sampleId, 
			&pSample, &sampleSize,
			&sampleTime, &sampleDuration, &sampleRenderingOffset, 
			&isSyncSample);

		// convert timestamp and duration from track time to milliseconds
		u_int64_t myTime = MP4ConvertFromTrackTimestamp(mp4File, trackId, 
			sampleTime, MP4_MSECS_TIME_SCALE);

		u_int64_t myDuration = MP4ConvertFromTrackDuration(mp4File, trackId,
			sampleDuration, MP4_MSECS_TIME_SCALE);

		// decode frame and display it
	}

	// close mp4 file
	MP4Close(mp4File);


	// Note to seek to time 'when' in the track
	// use MP4GetSampleIdFromTime(MP4FileHandle hFile, 
	//		MP4Timestamp when, bool wantSyncSample)
	// 'wantSyncSample' determines if a sync sample is desired or not
	// e.g.
	// MP4Timestamp when = 
	//	MP4ConvertToTrackTimestamp(mp4File, trackId, 30, MP4_SECS_TIME_SCALE);
	// MP4SampleId newSampleId = MP4GetSampleIdFromTime(mp4File, when, true);
	// MP4ReadSample(mp4File, trackId, newSampleId, ...);
	// 
	// Note that start time for sample may be later than 'when'

	exit(0);
}

