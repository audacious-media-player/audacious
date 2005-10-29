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
#if 0
#include "mp4util.h"
#endif

main(int argc, char** argv)
{
#if 1
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		exit(1);
	}

	u_int32_t verbosity = MP4_DETAILS_ALL;

	MP4FileHandle mp4File = MP4Create(argv[1], verbosity);

	if (!mp4File) {
		exit(1);
	}

	printf("Created skeleton\n");
	MP4Dump(mp4File);

	MP4SetODProfileLevel(mp4File, 1);
	MP4SetSceneProfileLevel(mp4File, 1);
	MP4SetVideoProfileLevel(mp4File, 1);
	MP4SetAudioProfileLevel(mp4File, 1);
	MP4SetGraphicsProfileLevel(mp4File, 1);

	MP4TrackId odTrackId = 
		MP4AddODTrack(mp4File);

	MP4TrackId bifsTrackId = 
		MP4AddSceneTrack(mp4File);

	MP4TrackId videoTrackId = 
#if 0
		MP4AddVideoTrack(mp4File, 90000, 3000, 320, 240);
#else
	MP4AddH264VideoTrack(mp4File, 90000, 3000, 320, 240, 
			     1, 2, 3, 1);
	static uint8_t pseq[] = { 0, 1, 2, 3, 4, 5, 6,7, 8, 9 };

	MP4AddH264SequenceParameterSet(mp4File, videoTrackId, pseq, 10);
	MP4AddH264SequenceParameterSet(mp4File, videoTrackId, pseq, 6);
	MP4AddH264PictureParameterSet(mp4File, videoTrackId, pseq, 7);
	MP4AddH264PictureParameterSet(mp4File, videoTrackId, pseq, 8);
	MP4AddH264PictureParameterSet(mp4File, videoTrackId, pseq, 7);

#endif

	MP4TrackId videoHintTrackId = 
		MP4AddHintTrack(mp4File, videoTrackId);

	MP4TrackId audioTrackId = 
		MP4AddAudioTrack(mp4File, 44100, 1152);

	MP4TrackId audioHintTrackId = 
		MP4AddHintTrack(mp4File, audioTrackId);

	printf("Added tracks\n");
	MP4Dump(mp4File);

	MP4Close(mp4File);

	//	MP4MakeIsmaCompliant(argv[1], verbosity);

	exit(0);
#else
   uint8_t *bin = NULL;

   for (uint32_t ix = 4; ix < 1024; ix++) {
     printf("pass %d\n", ix);
     bin = (uint8_t *)malloc(ix);
     for (uint32_t jx = 0; jx < ix; jx++) {
       bin[jx] = ((uint32_t)random()) >> 24;
     }
     char *test;
     test = MP4ToBase64(bin, ix);
     uint8_t *ret;
     uint32_t retsize;
     ret = Base64ToBinary(test, strlen(test), &retsize);
     if (retsize != ix) {
       printf("return size not same %d %d\n", ix, retsize);
       exit(0);
     }
     if (memcmp(ret, bin, ix) != 0) {
       printf("memory not same\n");
       exit(0);
     }
     free(test);
     free(ret);
     free(bin);
   }
   return 0;
#endif
}

