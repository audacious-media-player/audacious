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

#include "mpeg4ip.h"
#include <arpa/inet.h>
#include "mp4.h"

// forward declarations
static bool AssembleSdp(
	MP4FileHandle mp4File, 
	const char* sdpFileName,
	const char* destIpAddress);

static bool InitSockets(
	u_int32_t numSockets, 
	int* pSockets, 
	const char* destIpAddress);

static u_int64_t GetUsecTime();

// globals
char* ProgName;
u_int16_t UdpBasePort = 20000;
u_int32_t MulticastTtl = 2;	// increase value if necessary

const u_int32_t SecsBetween1900And1970 = 2208988800U;


// the main show
int main(int argc, char** argv)
{
	// since we're a test program
	// keep command line processing to a minimum
	// and assume some defaults
	ProgName = argv[0];
	char* sdpFileName = "./mp4broadcaster.sdp";
	char* destIpAddress = "224.1.2.3";

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file>\n", ProgName);
		exit(1);
	}

	char* mp4FileName = argv[1];
	u_int32_t verbosity = 
		MP4_DETAILS_ERROR | MP4_DETAILS_READ 
		| MP4_DETAILS_SAMPLE | MP4_DETAILS_HINT;

	// open the mp4 file
	MP4FileHandle mp4File = MP4Read(mp4FileName, verbosity);

	if (mp4File == MP4_INVALID_FILE_HANDLE) {
		// library will print an error message
		exit(1);
	}

	// check for hint tracks
	u_int32_t numHintTracks = 
		MP4GetNumberOfTracks(mp4File, MP4_HINT_TRACK_TYPE);

	if (numHintTracks == 0) {
		fprintf(stderr, "%s: File %s does not contain any hint tracks", 
			ProgName, mp4FileName);
		exit(2);
	}

	// assemble and write out sdp file
	AssembleSdp(mp4File, sdpFileName, destIpAddress);

	// create two UDP sockets for each track that will be streamed
	// one for the RTP media, one for the RTCP control
	int* udpSockets = new int[numHintTracks * 2]; 

	if (!InitSockets(numHintTracks, udpSockets, destIpAddress)) {
		fprintf(stderr, "%s: Couldn't create UDP sockets\n",
			ProgName);
		exit(3);
	}

	// initialize RTCP packet
	u_int32_t ssrc = random();
	char rtcpCName[256]; 
	char* username = getlogin();
	char hostname[256];
	gethostname(hostname, sizeof(hostname));
	snprintf(rtcpCName, sizeof(rtcpCName), "%s@%s", username, hostname);

	u_int8_t rtcpPacket[512];
	u_int16_t rtcpPacketLength = 38;

	// RTCP SR
	rtcpPacket[0] = 0x80;
	rtcpPacket[1] = 0xC8;
	rtcpPacket[2] = 0x00;
	rtcpPacket[3] = 0x06;
	*(u_int32_t*)&rtcpPacket[4] = htonl(ssrc);

	// RTCP SDES CNAME
	rtcpPacket[28] = 0x80;
	rtcpPacket[29] = 0xCA;
	*(u_int32_t*)&rtcpPacket[32] = htonl(ssrc);

	rtcpPacket[36] = 0x01;
	rtcpPacket[37] = strlen(rtcpCName);
	strcpy((char*)&rtcpPacket[38], rtcpCName);
	rtcpPacketLength += strlen(rtcpCName) + 1;

	// pad with zero's to 32 bit boundary
	while (rtcpPacketLength % 4 != 0) {
		rtcpPacket[rtcpPacketLength++] = 0; 
	}

	*(u_int16_t*)&rtcpPacket[30] = ntohs((rtcpPacketLength - 32) / 4);

	// initialize per track variables that we will use in main loop
	MP4TrackId* hintTrackIds = new MP4TrackId[numHintTracks]; 
	MP4SampleId* nextHintIds = new MP4SampleId[numHintTracks]; 
	MP4SampleId* maxHintIds = new MP4SampleId[numHintTracks]; 
	u_int64_t* nextHintTimes = new MP4Timestamp[numHintTracks];
	u_int32_t* packetsSent = new u_int32_t[numHintTracks];
	u_int32_t* bytesSent = new u_int32_t[numHintTracks];

	u_int32_t i;

	for (i = 0; i < numHintTracks; i++) {
		hintTrackIds[i] = MP4FindTrackId(mp4File, i, MP4_HINT_TRACK_TYPE);
		nextHintIds[i] = 1;
		maxHintIds[i] = MP4GetTrackNumberOfSamples(mp4File, hintTrackIds[i]);
		nextHintTimes[i] = (u_int64_t)-1;
		packetsSent[i] = 0;
		bytesSent[i] = 0;
	}

	// remember the starting time
	u_int64_t start = GetUsecTime();
	u_int64_t lastSR = 0;

	// main loop to stream data
	while (true) {
		u_int32_t nextTrackIndex = (u_int32_t)-1;
		u_int64_t nextTime = (u_int64_t)-1;

		// find the next hint to send
		for (i = 0; i < numHintTracks; i++) {
			if (nextHintIds[i] > maxHintIds[i]) {
				// have finished this track
				continue;
			}

			// need to get the time of the next hint
			if (nextHintTimes[i] == (u_int64_t)-1) {
				MP4Timestamp hintTime =
					MP4GetSampleTime(mp4File, hintTrackIds[i], nextHintIds[i]);

				nextHintTimes[i] = 
					MP4ConvertFromTrackTimestamp(mp4File, hintTrackIds[i],
						hintTime, MP4_USECS_TIME_SCALE);
			}

			// check if this track's next hint is the earliest yet
			if (nextHintTimes[i] > nextTime) {
				continue;
			}

			// make this our current choice for the next hint
			nextTime = nextHintTimes[i];
			nextTrackIndex = i;
		}

		// check exit condition, i.e all hints for all tracks have been used
		if (nextTrackIndex == (u_int32_t)-1) {
			break;
		}

		// wait until the correct time to send next hint
		// we assume we're not going to fall behind for testing purposes
		// in a real application some skipping of samples might be needed

		u_int64_t now = GetUsecTime();
		int64_t waitTime = (start + nextTime) - now;
		if (waitTime > 0) {
			usleep(waitTime);
		}

		now = GetUsecTime();

		// emit RTCP Sender Reports every 5 seconds for all media streams
		// not quite what a real app would do, but close enough for testing
		if (now - lastSR >= 5000000) {
			for (i = 0; i < numHintTracks; i++) {
				now = GetUsecTime();

				u_int32_t ntpSecs =
					(now / MP4_USECS_TIME_SCALE) + SecsBetween1900And1970;
				*(u_int32_t*)&rtcpPacket[8] = 
					htonl(ntpSecs);

				u_int32_t usecs = now % MP4_USECS_TIME_SCALE;
				u_int32_t ntpUSecs =
					(usecs << 12) + (usecs << 8) - ((usecs * 3650) >> 6); 
				*(u_int32_t*)&rtcpPacket[12] = 
					htonl(ntpUSecs);

				MP4Timestamp rtpStart =
					MP4GetRtpTimestampStart(mp4File, hintTrackIds[i]);

				MP4Timestamp rtpOffset =
					MP4ConvertToTrackTimestamp(mp4File, hintTrackIds[i],
						now - start, MP4_USECS_TIME_SCALE);

				*(u_int32_t*)&rtcpPacket[16] =
					 htonl(rtpStart + rtpOffset); 

				*(u_int32_t*)&rtcpPacket[20] =
					htonl(packetsSent[i]);
				*(u_int32_t*)&rtcpPacket[24] =
					htonl(bytesSent[i]);

				send(udpSockets[i * 2 + 1], rtcpPacket, rtcpPacketLength, 0);
			}

			lastSR = now;
		}

		// send all the packets for this hint
		// since this is just a test program 
		// we don't attempt to smooth out the transmission of the packets

		u_int16_t numPackets;

		MP4ReadRtpHint(
			mp4File, 
			hintTrackIds[nextTrackIndex], 
			nextHintIds[nextTrackIndex],
			&numPackets);

		// move along in this hint track
		nextHintIds[nextTrackIndex]++;
		nextHintTimes[nextTrackIndex] = (u_int64_t)-1; 

		u_int16_t packetIndex;

		for (packetIndex = 0; packetIndex < numPackets; packetIndex++) {
			u_int8_t* pPacket = NULL;
			u_int32_t packetSize;

			// get the packet from the library
			MP4ReadRtpPacket(
				mp4File, 
				hintTrackIds[nextTrackIndex], 
				packetIndex,
				&pPacket,
				&packetSize,
				ssrc);

			if (pPacket == NULL) {
				// error, but forge on
				continue;
			}

			// send it out via UDP
			send(udpSockets[nextTrackIndex * 2], pPacket, packetSize, 0);

			// free packet buffer
			free(pPacket);

			bytesSent[nextTrackIndex] += packetSize - 12;
		}

		packetsSent[nextTrackIndex] += numPackets;
	}

	// main loop finished

	// close the UDP sockets
	for (i = 0; i < numHintTracks; i++) {
		close(udpSockets[i]);
	}

	// close mp4 file
	MP4Close(mp4File);

	// free up memory
	delete [] hintTrackIds;
	delete [] nextHintIds;
	delete [] maxHintIds;
	delete [] nextHintTimes;
	delete [] packetsSent;
	delete [] bytesSent;

	exit(0);
}

static bool AssembleSdp(
	MP4FileHandle mp4File, 
	const char* sdpFileName,
	const char* destIpAddress)
{
	// open the destination sdp file
	FILE* sdpFile = fopen(sdpFileName, "w");

	if (sdpFile == NULL) {
		fprintf(stderr, 
			"%s: couldn't open sdp file %s\n",
			ProgName, sdpFileName);
		return false;
	}

	// add required header fields
	fprintf(sdpFile,
		"v=0\015\012"
		"o=- 1 1 IN IP4 127.0.0.1\015\012"
		"s=mp4broadcaster\015\012"
		"e=NONE\015\012"
		"c=IN IP4 %s/%u\015\012"
		"b=RR:0\015\012",
		destIpAddress,
		MulticastTtl);

	// add session level info from mp4 file
	fprintf(sdpFile, "%s", 
		MP4GetSessionSdp(mp4File));

	// add per track info
	u_int32_t numHintTracks = 
		MP4GetNumberOfTracks(mp4File, MP4_HINT_TRACK_TYPE);

	for (u_int32_t i = 0; i < numHintTracks; i++) {
		MP4TrackId hintTrackId = 
			MP4FindTrackId(mp4File, i, MP4_HINT_TRACK_TYPE);

		if (hintTrackId == MP4_INVALID_TRACK_ID) {
			continue;
		}

		// get track sdp info from mp4 file
		const char* mediaSdp =
			MP4GetHintTrackSdp(mp4File, hintTrackId);

		// since we're going to broadcast instead of use RTSP for on-demand
		// we need to fill in the UDP port numbers for the media
		const char* portZero = strchr(mediaSdp, '0');
		if (portZero == NULL) {
			continue;	// mal-formed sdp
		}
		fprintf(sdpFile, "%.*s%u%s",
			portZero - mediaSdp,
			mediaSdp,
			UdpBasePort + (i * 2),
			portZero + 1);
	}

	fclose(sdpFile);

	return true;
}

static bool InitSockets(
	u_int32_t numSocketPairs, 
	int* pSockets, 
	const char* destIpAddress)
{
	u_int32_t i;


	for (i = 0; i < numSocketPairs * 2; i++) {
		// create the socket
		pSockets[i] = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (pSockets[i] < 0) {
			return false;
		}

		// allow local listeners to multicast
		int reuse = 1;
		setsockopt(pSockets[i], SOL_SOCKET, SO_REUSEADDR,
			&reuse, sizeof(reuse));
#ifdef SO_REUSEPORT
		// not all implementations have this option
		setsockopt(pSockets[i], SOL_SOCKET, SO_REUSEPORT,
			&reuse, sizeof(reuse));
#endif

		// get a local source address
		struct sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_port = 0;
		sin.sin_addr.s_addr = INADDR_ANY;

		if (bind(pSockets[i], (struct sockaddr*)&sin, sizeof(sin))) {
			return false;
		}

		// bind to the destination address
		sin.sin_port = htons(UdpBasePort + i);
		inet_aton(destIpAddress, &sin.sin_addr);

		if (connect(pSockets[i], (struct sockaddr*)&sin, sizeof(sin)) < 0) {
			return false;
		}

		// set the multicast time to live
		setsockopt(pSockets[i], IPPROTO_IP, IP_MULTICAST_TTL,
			&MulticastTtl, sizeof(MulticastTtl));
	}

	return true;
}

// get absolute time expressed in microseconds
static u_int64_t GetUsecTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000) + tv.tv_usec;
}

