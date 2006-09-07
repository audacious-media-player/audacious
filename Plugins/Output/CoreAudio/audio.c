/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2001  Peter Alm, Mikael Alm, Olle Hallnas,
 *                           Thomas Nilsson and 4Front Technologies
 *  Copyright (C) 1999-2001  Haavard Kvaalen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "coreaudio.h"
#include "libaudacious/util.h"
#include <errno.h>
#include <CoreAudio/CoreAudio.h>

AudioDeviceID device_id;
AudioStreamBasicDescription device_format;
AudioStreamBasicDescription streamDesc;

//static gint fd = 0;
static float *buffer;
gboolean playing_flag;
static gboolean prebuffer, unpause, do_pause, remove_prebuffer;
static gint device_buffer_size;
static gint buffer_size, prebuffer_size;//, blk_size;
static gint buffer_index = 0;
static gint output_time_offset = 0;
static guint64 written = 0, output_total = 0;
static gint flush;
static gchar *device_name;

gint sample_multiplier, sample_size;

gboolean paused;


float left_volume, right_volume;
float base_pitch = 0.0;
float user_pitch = 0.0;
int   output_buf_length; // length of data in output buffer
short output_buf[OUTPUT_BUFSIZE];  /* buffer used to hold main output to dbfsd */
short cue_buf[OUTPUT_BUFSIZE];     /* buffer used to hold cue output to dbfsd */
short conv_buf[OUTPUT_BUFSIZE];    /* buffer used to hold format converted input */

/*
 * The format of the data from the input plugin
 * This will never change during a song. 
 */
struct format_info input;


/*
 * The format we get from the effect plugin.
 * This will be different from input if the effect plugin does
 * some kind of format conversion.
 */
struct format_info effect;


/*
 * The format of the data we actually send to the soundcard.
 * This might be different from effect if we need to resample or do
 * some other format conversion.
 */
struct format_info output;


static int osx_calc_bitrate(int osx_fmt, int rate, int channels)
{
	int bitrate = rate * channels;

	// for now we know output is stereo
	// fix this later

	if (osx_fmt == FMT_U16_BE || osx_fmt == FMT_U16_LE ||
	    osx_fmt == FMT_S16_BE || osx_fmt == FMT_S16_LE)
	{
		bitrate *= 2;
	}

	//printf("osx_calc_bitrate(): %d\n",bitrate);

	return bitrate;
}


static int osx_get_format(AFormat fmt)
{
	int format = 0;

	switch (fmt)
	{
		case FMT_U16_NE:
#ifdef WORDS_BIGENDIAN
			format = FMT_U16_BE;
#else
			format = FMT_U16_LE;
#endif
			break;
		case FMT_S16_NE:
#ifdef WORDS_BIGENDIAN
			format = FMT_S16_BE;
#else
			format = FMT_S16_LE;
#endif
			break;
		default:
			format = fmt;
			break;
	}

	return format;
}


OSStatus play_callback(AudioDeviceID inDevice, const AudioTimeStamp * inNow, const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime, AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime, void * inClientData)
{
	int i;
	long m, n, o;
	float * dest, tempfloat;
	float * src;
	int     src_size_bytes;
	int     src_size_float;
	int     num_output_samples;
	int     used_samples;

	src_size_bytes = outOutputData->mBuffers[0].mDataByteSize;
	src_size_float = src_size_bytes / sizeof(float);

	num_output_samples = MIN(buffer_index,src_size_float);

	//printf("play_callback(): num_output_samples %d, index %d\n",num_output_samples,buffer_index);

	// if we are prebuffering, zero the buffer
	if (prebuffer && (buffer_index < prebuffer_size))
	{
		//printf("prebuffering... %d samples left\n",prebuffer_size-buffer_index);
		num_output_samples = 0;
	}
	else
	{
		prebuffer = FALSE;
	}

	src = buffer;
	dest = outOutputData->mBuffers[0].mData;

	// copy available data to buffer and apply volume to each channel
	for (i = 0; i < num_output_samples/2; i++)
	{
		//tempfloat = *src;
		*dest = (*src) * left_volume;
		src++;
		dest++;

		*dest = (*src) * right_volume;
		src++;
		dest++;
	}

	// if less than a buffer's worth of data is ready, zero remainder of output buffer
	if (num_output_samples != src_size_float)
	{
		//printf("zeroing %d samples",(src_size_float - num_output_samples));

		dest = (float*)outOutputData->mBuffers[0].mData + num_output_samples;

		memset(dest,0,(src_size_float - num_output_samples) * sizeof(float));
	}
	
	// move unwritten data to beginning of buffer
	{
		dest = buffer;

		for (i = num_output_samples; i < buffer_index; i++)
		{
			*dest = *src;
			dest++;
			src++;
		}

		output_total += num_output_samples;
		buffer_index -= num_output_samples;
	}


	if (flush != -1)
	{
		osx_set_audio_params();
		output_time_offset = flush;
		written = ((guint64)flush * input.bps) / (1000 * sample_size);
		buffer_index = 0;
		output_total = 0;

		flush = -1;
		prebuffer = TRUE;
	}

	//printf("\n");

	return 0;
}


static void osx_setup_format(AFormat fmt, int rate, int nch)
{
	//printf("osx_setup_format(): fmt %d, rate %d, nch %d\n",fmt,rate,nch);

	effect.format.xmms = osx_get_format(fmt);
	effect.frequency = rate;
	effect.channels = nch;
	effect.bps = osx_calc_bitrate(fmt, rate, nch);

	output.format.osx = osx_get_format(fmt);
	output.frequency = rate;
	output.channels = nch;

	osx_set_audio_params();

	output.bps = osx_calc_bitrate(output.format.osx, output.frequency,output.channels);
}


gint osx_get_written_time(void)
{
	gint  retval;

	if (!playing_flag)
	{
		retval = 0;
	}
	else
	{
		retval = (written * sample_size * 1000) / effect.bps;
		retval = (int)((float)retval / user_pitch);
	}

	//printf("osx_get_written_time(): written time is %d\n",retval);

	return retval;
}


gint osx_get_output_time(void)
{
	gint retval;

	retval = output_time_offset + ((output_total * sample_size * 1000) / output.bps);
	retval = (int)((float)retval / user_pitch);
	
	//printf("osx_get_output_time(): time is %d\n",retval);

	return retval;
}


gint osx_playing(void)
{
	gint retval;

	retval = 0;

	if (!playing_flag)
	{
		retval = 0;
	}
	else
	{
		if (buffer_index == 0)
		{
			retval = FALSE;
		}
		else
		{
			retval = TRUE;
		}
	}

	//printf("osx_playing(): playing is now %d\n",playing_flag);

	return retval;
}


gint osx_free(void)
{
	gint bytes_free;

	if (remove_prebuffer && prebuffer)
	{
		prebuffer = FALSE;
		remove_prebuffer = FALSE;
	}

	if (prebuffer)
	{
		remove_prebuffer = TRUE;
	}

	// get number of free samples
	bytes_free = buffer_size - buffer_index;
	
	// adjust for mono
	if (input.channels == 1)
	{
		bytes_free /= 2;
	}

	// adjust by pitch conversion;
	bytes_free = (int)((float)bytes_free * base_pitch * user_pitch);

	// convert from number of samples to number of bytes
	bytes_free *= sample_size;

	return bytes_free;
}


void osx_write(gpointer ptr, int length)
{
	int count, offset = 0;
	int error;
	float tempfloat;
	float * dest;
	short * src, * tempbuf;
	int i;
	int num_samples;

	//printf("oss_write(): lenght: %d \n",length);

	remove_prebuffer = FALSE;

	//	//printf("written is now %d\n",(gint)written);

	// get number of samples
	num_samples = length / sample_size;

	// update amount of samples received
	written += num_samples;

	// step through audio 
	while (num_samples > 0)
	{
		// get # of samples to write to the buffer
		count = MIN(num_samples, osx_free()/sample_size);
		
		src = ptr+offset;

		if (dbconvert((char*)src,count * sample_size) == -1)
		{
			//printf("dbconvert error %d\n",errno);
		}
		else
		{
			src = output_buf;
			dest = (float*)(buffer + buffer_index);
			
			//printf("output_buf_length is %d\n",output_buf_length);

			for (i = 0; i < output_buf_length; i++)
			{
				tempfloat = ((float)*src)/32768.0;
				*dest = tempfloat;
				dest++;
				src++;
			}

			buffer_index += output_buf_length;
		}

		if (buffer_index > buffer_size)
		{
			//printf("BUFFER_INDEX > BUFFER_SIZE!!!!\n");
			exit(0);
		}

		num_samples -= count;
		offset += count;
	}

	//printf("buffer_index is now %d\n\n",buffer_index);
}


void osx_close(void)
{
	//printf("osx_close(): playing_flag is %d\n",playing_flag);

	if (!playing_flag)
	{
		return;
	}

	playing_flag = 0;

	// close audio device
	AudioDeviceStop(device_id, play_callback); 
	AudioDeviceRemoveIOProc(device_id, play_callback);

	g_free(device_name);

	//printf("osx_close(): playing_flag is now %d\n",playing_flag);
}


void osx_flush(gint time)
{
	//printf("osx_flush(): %d\n",time);

	flush = time;

	while (flush != -1)
	{
		xmms_usleep(10000);
	}
}


void osx_pause(short p)
{
	//printf("osx_pause(): %d\n",p);

	if (p == TRUE)
	{
		if (AudioDeviceStop(device_id, play_callback))
		{
			//printf("failed to stop audio device.\n");
		}

		//printf("PAUSED!\n");
	}
	else
	{
		if (AudioDeviceStart(device_id, play_callback))
		{
			//printf("failed to start audio device.\n");
		}

		//printf("UNPAUSED!\n");
	}

	paused = p;
}


void osx_set_audio_params(void)
{
	int stereo_multiplier, format_multiplier;
	int frag, stereo, ret;
	struct timeval tv;
	fd_set set;

	//printf("osx_set_audio_params(): fmt %d, freq %d, nch %d\n",output.format.osx,output.frequency,output.channels);

	// set audio format 

	// set num channels

	switch (input.channels)
	{
		case 1:  stereo_multiplier = 2; break;
		case 2:  stereo_multiplier = 1; break;
		default: stereo_multiplier = 1; break;
	}
	
	switch (input.format.xmms)
	{
		case FMT_U8:    
		case FMT_S8:
			format_multiplier = 2;
			sample_size = 1;
			break;
		case FMT_S16_LE:
		case FMT_S16_BE:
		case FMT_S16_NE:
			format_multiplier = 1;
			sample_size = 2;
			break;
		default: format_multiplier = 1; break;
	}

	sample_multiplier = stereo_multiplier * format_multiplier;

	base_pitch = input.frequency / device_format.mSampleRate;

	//printf("sample multiplier is now %d, base pitch %.2f\n",sample_multiplier,base_pitch);
}


gint osx_open(AFormat fmt, gint rate, gint nch)
{
	char s[32];
	long m;
	long size;
	char device_name[128];

	//printf("\nosx_open(): fmt %d, rate %d, nch %d\n",fmt,rate,nch);

	// init conversion variables
	base_pitch = 1.0;
	user_pitch = 1.0;

	// open audio device

	size = sizeof(device_id);

	if (AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &device_id))
	{
		//printf("failed to open default audio device");
		return -1;
	}

	//printf("opened audio device\n");

	size = 128;

	if (AudioDeviceGetProperty(device_id,1,0,kAudioDevicePropertyDeviceName,&size,device_name))
	{
		//printf("could not get device name\n");
		return -1;
	}

	//printf("device name is: \"%s\"\n",device_name);

	size = sizeof(device_format);

	if (AudioDeviceGetProperty(device_id, 0, 0, kAudioDevicePropertyStreamFormat, &size, &device_format))
	{
		//printf("failed to get audio format!\n");
		return -1;
	}

	//fprintf(stderr, "got format:  sample rate %f, %ld channels and %ld-bit sample\n",
	//		device_format.mSampleRate,device_format.mChannelsPerFrame,device_format.mBitsPerChannel);

	if (device_format.mFormatID != kAudioFormatLinearPCM)
	{
		//printf("audio format isn't PCM\n");
		return -1;
	}

	//printf("format is PCM\n");

	input.format.xmms = fmt;
	input.frequency = rate;
	input.channels = nch;
	input.bps = osx_calc_bitrate(osx_get_format(fmt),rate,nch);

	osx_setup_format(osx_get_format(fmt),device_format.mSampleRate,device_format.mChannelsPerFrame);

	//set audio buffer size
	{
		device_buffer_size = 4096 * sizeof(float);
		size = sizeof(gint);

		if (AudioDeviceSetProperty(device_id,0,0,0,kAudioDevicePropertyBufferSize,size,&device_buffer_size))
		{
			//printf("failed to set device buffer size\n");
		}

		//printf("buffer size set to %d\n",device_buffer_size);
	}

	buffer_size = 11 * 4096;
	prebuffer_size = 4096;

	buffer = (float *) g_malloc0(buffer_size*sizeof(float));

	//printf("created buffer of size %d, prebuffer is %d\n",buffer_size,prebuffer_size);

	flush = -1;
	prebuffer = TRUE;

	buffer_index = output_time_offset = written = output_total = 0;

	paused = FALSE;

	do_pause = FALSE;
	unpause = FALSE;
	remove_prebuffer = FALSE;

	playing_flag = 1;

	if (AudioDeviceAddIOProc(device_id, play_callback, NULL))
	{
		//printf("failed to add IO Proc callback\n");
		osx_close();
		return -1;
	}

	//printf("added callback\n");

	if (AudioDeviceStart(device_id,play_callback))
	{
		osx_close();
		//printf("failed to start audio device.\n");
		exit(0);
	}

	//printf("started audio device\n");

	return 1;
}
