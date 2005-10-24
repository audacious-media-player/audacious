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
#include "mp4av.h"
#include "mp4av_h264.h"
char* ProgName;
char* Mp4PathName;
char* Mp4FileName;
static void ParseH264 (uint8_t *bptr, uint32_t blen, 
		       uint32_t len_size, bool dump_off)
{
  uint8_t *fptr = bptr;
  while (blen > len_size) {
    uint32_t nal_len = 0;
    switch (len_size) {
    case 1: nal_len = *bptr; break;
    case 2: nal_len = (bptr[0] << 8) | bptr[1]; break;
    case 3: nal_len = (bptr[0] << 16) | (bptr[1] << 8) | bptr[2]; break;
    case 4: nal_len = (bptr[0] << 24) | (bptr[1] << 16) | (bptr[2] << 8) | bptr[3]; break;
    }
    bptr += len_size;
    blen -= len_size;
    uint8_t nal_type = *bptr & 0x1f;
    switch (nal_type) {
    case H264_NAL_TYPE_NON_IDR_SLICE: printf(" NIDR-"); break;
    case H264_NAL_TYPE_DP_A_SLICE: printf(" DPA-"); break;
    case H264_NAL_TYPE_DP_B_SLICE: printf(" DPB-"); break;
    case H264_NAL_TYPE_DP_C_SLICE: printf(" DPC-"); break;
    case H264_NAL_TYPE_IDR_SLICE: printf(" IDR-"); break;
    case H264_NAL_TYPE_SEI: printf(" SEI"); break;
    case H264_NAL_TYPE_SEQ_PARAM: printf(" SEQ"); break;
    case H264_NAL_TYPE_PIC_PARAM: printf(" PIC"); break;
    case H264_NAL_TYPE_ACCESS_UNIT: printf(" AU"); break;
    case H264_NAL_TYPE_END_OF_SEQ: printf(" EOS"); break;
    case H264_NAL_TYPE_END_OF_STREAM: printf(" EOST"); break;
    case H264_NAL_TYPE_FILLER_DATA: printf(" FILL"); break;
    default: printf(" UNK(0x%x)", nal_type); break;
    }
    if (h264_nal_unit_type_is_slice(nal_type)) {
      uint8_t slice_type;
      h264_find_slice_type(bptr, nal_len, &slice_type, true);
      printf("%s", h264_get_slice_name(slice_type));
    }
    uint32_t off =  bptr - fptr - len_size;
    if (dump_off) printf("(%u)", off);
    bptr += nal_len;
    if (nal_len > blen) {
      blen = 0;
    } else 
      blen -= nal_len;
  }
}

static void ParseMpeg4 (uint8_t *bptr, uint32_t blen, bool dump_off)
{
  uint8_t *fptr = bptr;
  while (blen > 4) {
    if (bptr[0] == 0 &&
	bptr[1] == 0 &&
	bptr[2] == 1) {
      if (bptr[3] > 0 && bptr[3] < MP4AV_MPEG4_VOL_START) {
	printf(" VDOS");
      } else if (bptr[3] < 0x2f) {
	printf(" VOL");
      } else if (bptr[3] == MP4AV_MPEG4_VOSH_START) {
	printf(" VOSH");
      } else if (bptr[3] == MP4AV_MPEG4_VOSH_END) {
	printf(" VOSHE");
      } else if (bptr[3] == MP4AV_MPEG4_USER_DATA_START) {
	printf(" UD");
      } else if (bptr[3] == MP4AV_MPEG4_GOV_START) {
	printf(" GOV");
      } else if (bptr[3] == 0xB4) {
	printf(" VSE");
      } else if (bptr[3] == MP4AV_MPEG4_VO_START) {
	printf(" VOS");
      } else if (bptr[3] == MP4AV_MPEG4_VOP_START) {
	int type = MP4AV_Mpeg4GetVopType(bptr, blen);
	switch (type) {
	case VOP_TYPE_I: printf(" VOP-I"); break;
	case VOP_TYPE_P: printf(" VOP-P"); break;
	case VOP_TYPE_B: printf(" VOP-B"); break;
	case VOP_TYPE_S: printf(" VOP-S"); break;
	}
      } else printf(" 0x%x", bptr[3]);
      uint32_t off = bptr - fptr;
      if (dump_off) printf("(%u)", off);
    }
    bptr++;
    blen--;
  }
}
static void DumpTrack (MP4FileHandle mp4file, MP4TrackId tid, 
		       bool dump_off, bool dump_rend)
{
  uint32_t numSamples;
  MP4SampleId sid;
  uint8_t *buffer;
  uint32_t max_frame_size;
  uint32_t timescale;
  uint64_t msectime;
  const char *media_data_name;
  uint32_t len_size = 0;

  numSamples = MP4GetTrackNumberOfSamples(mp4file, tid);
  max_frame_size = MP4GetTrackMaxSampleSize(mp4file, tid) + 4;
  media_data_name = MP4GetTrackMediaDataName(mp4file, tid);
  if (strcasecmp(media_data_name, "avc1") == 0) {
    MP4GetTrackH264LengthSize(mp4file, tid, &len_size);
  }
  buffer = (uint8_t *)malloc(max_frame_size);
  if (buffer == NULL) {
    printf("couldn't get buffer\n");
    return;
  }

  timescale = MP4GetTrackTimeScale(mp4file, tid);
  printf("mp4file %s, track %d, samples %d, timescale %d\n", 
	 Mp4FileName, tid, numSamples, timescale);

  for (sid = 1; sid <= numSamples; sid++) {
    MP4Timestamp sampleTime;
    MP4Duration sampleDuration, sampleRenderingOffset;
    bool isSyncSample = FALSE;
    bool ret;
    u_int8_t *temp;
    uint32_t this_frame_size = max_frame_size;
    temp = buffer;
    ret = MP4ReadSample(mp4file, 
			tid,
			sid,
			&temp,
			&this_frame_size,
			&sampleTime,
			&sampleDuration,
			&sampleRenderingOffset,
			&isSyncSample);

    msectime = sampleTime;
    msectime *= TO_U64(1000);
    msectime /= timescale;

    printf("sampleId %6d, size %5u time "U64"("U64")",
	  sid,  MP4GetSampleSize(mp4file, tid, sid), 
	   sampleTime, msectime);
    if (dump_rend) printf(" %6"U64F, sampleRenderingOffset);
    if (strcasecmp(media_data_name, "mp4v") == 0) {
      ParseMpeg4(temp, this_frame_size, dump_off);
    } else if (strcasecmp(media_data_name, "avc1") == 0) {
      ParseH264(temp, this_frame_size, len_size, dump_off);
    }
    printf("\n");
  }
}

int main(int argc, char** argv)
{
  const char* usageString = 
    "[options] mp4file where:\n"
    "\t--track(-t)= <track-id> - display track id\n"
    "\t--dump-offset(-d) - dump offset within sample\n"
    "\t--rendering-offset(-r) - dump rendering offset\n"
    "\t--verbose=<level> - mp4 file verbosity\n"
    "\t--version(-V) - display version\n";
  MP4TrackId trackId = MP4_INVALID_TRACK_ID;
  u_int32_t verbosity = MP4_DETAILS_ERROR;
  bool dump_offset = false;
  bool dump_rend = false;
  /* begin processing command line */
  ProgName = argv[0];
  while (true) {
    int c = -1;
    int option_index = 0;
    static struct option long_options[] = {
      { "track", 1, 0, 't' },
      { "verbose", 2, 0, 'v' },
      { "version", 0, 0, 'V' },
      { "dump-offset", 0, 0, 'd'},
      { "rendering-offset", 0, 0, 'r'},
      { "help", 0, 0, '?'},
      { NULL, 0, 0, 0 }
    };

    c = getopt_long_only(argc, argv, "t:v::V?",
			 long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 'd': dump_offset = true; break;
    case 'r': dump_rend = true; break;
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
  
  if (trackId == MP4_INVALID_TRACK_ID) {
    u_int32_t numTracks = MP4GetNumberOfTracks(mp4File, MP4_VIDEO_TRACK_TYPE);
    printf("tracks %d\n", numTracks);
    for (u_int32_t ix = 0; ix < numTracks; ix++) {
      trackId = MP4FindTrackId(mp4File, ix, MP4_VIDEO_TRACK_TYPE);
      DumpTrack(mp4File, trackId, dump_offset, dump_rend);
    }
  } else {
    DumpTrack(mp4File, trackId, dump_offset, dump_rend);
  }

  MP4Close(mp4File);

  return(0);
}

