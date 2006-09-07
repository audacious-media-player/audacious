/*

  Author:  Bob Dean
  Copyright (c) 1999 - 2004

  The functionality in this file is modified from DBAudio_Write.c. 
  part of the DBMix project which is also released under the GPL.
  It is used here both as licensed under the GPL and additionally 
  by permission of the original author (which is me).

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public Licensse as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/shm.h>
#include <glib.h>
#include <math.h>
#include <string.h>
	
#include "coreaudio.h"
#include "dbaudiolib.h"

	extern int errno;
	
	extern gboolean paused;
	extern float left_volume, right_volume;

	extern float base_pitch;
	extern float user_pitch;
	

	float local_pitch;
	float sample1, sample2;
	float float_index;
	
	extern signed short output_buf[];  /* buffer used to hold main output to dbfsd */
	extern signed short cue_buf[];     /* buffer used to hold cue output to dbfsd */
	extern signed short conv_buf[];    /* buffer used to hold format converted input */
	extern int output_buf_length;
	
	int outlen;
	int sampleindex;
	int num_channels;
	int format;


	extern struct format_info input;

	
	/*
	  dbconvert - given a buf of length len, write the data to the
	  channel associated with this instance.

	  On success, the number of bytes written is returned. Otherwise
	  -1, or FAILURE, is returned, and errno is set accordingly.

	  Hopefully this function returns values in the same fashion
	  as other basic I/O functions such as read() and write()

	  Variables: 
	  count is the number of bytes written during a loop iteration
	  totalcount is the total number of bytes written
	  left_gain and right_gain are percentages used to adjust the 
	  output signal volume
	  tempbuf is a temporary pointer used in the volume operation
	  temp_chbuf is a pointer to the buffer to be written to during
	  shared memory mode.
	*/

	int dbconvert(char* buf, int len)
	{
		int left_gain,right_gain;
		int left_cue_gain, right_cue_gain;
		signed short * tempbuf, *tempbuf2, * tempoutbuf;
		char * tempcharbuf;
		int incr_flag, count;
		int intindex;
		float buflen, gain1, gain2;
		int   index1,index2, output_sample;
		int i;
		int stereo_multiplier,format_multiplier;
		int tempsize;
		int sampler_flag;
		//enum sampler_state_e local_sampler_state;

		/* check parameters */
		if (buf == NULL) {errno = ERROR_BAD_PARAM; return FAILURE;}
		//if (ch == NULL)  {errno = ERROR_NOT_INITIALIZED; return FAILURE;}
		if (len < 0)     {errno = ERROR_BAD_PARAM; return FAILURE;}
	
		//DBAudio_Handle_Message_Queue();
		
		/* remember sampler state as it may change during
		   the course of the function */
#ifdef COMPILE_SAMPLER
		local_sampler_state = ch->sampler_state;
#endif

		if (paused)
		{
			//printf("convert: pauseed\n");
			return 0;
		}

		/* get pitch */
		local_pitch = base_pitch * user_pitch;

		//printf("convert: local pitch is %.2f, base %.2f user %.2f\n",local_pitch,base_pitch,user_pitch);
		buflen = len / 2.0;
	
		//printf("convert: buflen %.2f len is %d\n",buflen,len);

		/* calculate buffer space needed to convert the data to 
		   44.1kHz 16bit stereo*/
		switch (input.channels)
		{
			case 1:  stereo_multiplier = 2; break;
			case 2:  stereo_multiplier = 1; break;
			default: errno = ERROR_BAD_NUMCH; return FAILURE;
		}
		
		//printf("convert: format %d, %d %d %d %d\n",input.format.xmms,FMT_U8,FMT_S8,FMT_S16_LE,FMT_S16_BE);

		switch (input.format.xmms)
		{
			case FMT_U8:     format_multiplier = 2; break;
			case FMT_S8:     format_multiplier = 2; break;
			case FMT_S16_LE: format_multiplier = 1; break;
			case FMT_S16_BE: format_multiplier = 1; break;
			case FMT_S16_NE: format_multiplier = 1; break;
			default: errno = ERROR_BAD_FORMAT; return FAILURE;
		}
		
		/* return error if the needed output space is greater than the
		   output buffer */
		if (ceil((buflen * (float)stereo_multiplier * 
				  (float)format_multiplier) / local_pitch) > (float)(OUTPUT_BUFSIZE))
		{
			errno = ERROR_TOO_MUCH_DATA; 
			return FAILURE;
		}
		
		/* init local variables */
		intindex = 0;
		incr_flag = 0;
		sampleindex = 0;
		gain1 = gain2 = 0.0;
		sample1 = sample2 = 0.0;
		sampler_flag = 0;

		left_gain = (int)(128.0 * left_volume);
		right_gain = (int)(128.0 * right_volume);

	
#ifdef COMPILE_CUE
		left_cue_gain = ch->cue_left_gain;
		right_cue_gain = ch->cue_right_gain;

		/* calculate gain percentages */
		if (ch->mute == TRUE)
		{
			left_gain = right_gain = 0;
		}
		else
		{
			left_gain = ch->left_gain * sysdata->left_balance;
			right_gain = ch->right_gain * sysdata->right_balance;
	
			/* cut volume if mic is being used */
			if (sysdata->talkover_enabled && !(MIC_ENABLED))
			{
				left_gain = left_gain >> DB_TALKOVER_DIVISOR_POWER;
				right_gain = right_gain >> DB_TALKOVER_DIVISOR_POWER;
			}
		}
#endif

#ifdef COMPILE_SAMPLER
		switch (local_sampler_state)
		{
			case SAMPLER_PLAY_SINGLE:
			case SAMPLER_PLAY_LOOP:				

				if (ch->sampler_size == 0)
				{
					ch->sampler_state = SAMPLER_OFF;
					len = 0;
					goto done;
				}
				
				/* tempsize - amount of data available in buffer to read */
				tempsize = (ch->sampler_endoffset - ch->sampler_readoffset);

				sampler_flag = 1;

				/* if we are in loop mode and loop over end of buffer, 
                   get data from start of buffer */
				if ((tempsize < len) && (local_sampler_state == SAMPLER_PLAY_LOOP))
				{
					/* copy portion at end of buffer */
					memcpy(conv_buf,(ch->sampler_buf + ch->sampler_readoffset),tempsize);
					/* copy portion at beginning of buffer */
					memcpy(conv_buf+tempsize,ch->sampler_buf+ch->sampler_startoffset,(len - tempsize));
					/* update variables */
					/* read offset is now amount to write, minus the overflow, plus the startoffset */
					ch->sampler_readoffset = len - tempsize + ch->sampler_startoffset;
					tempsize = len;
				}
				else
				{
					/* if we are in simgle play mode and out of data, reset state
                       and exit */
					if ((tempsize <= 0) && (local_sampler_state == SAMPLER_PLAY_SINGLE))
					{
						ch->sampler_state = SAMPLER_READY;
						goto done;
					}

					/* get full buffers worth of data from somewhere in middle of 
                       sampler buffer  */
					if (tempsize > len) tempsize = len;

					memcpy(conv_buf,(ch->sampler_buf + ch->sampler_readoffset),tempsize);

					ch->sampler_readoffset += tempsize;
				}


				/* update function state variables */
				buflen = (tempsize / 2);
				tempbuf = conv_buf;

				break;
			default:
#endif
				{
					/* convert input data into 44.1 KHz 16 bit stereo */
					tempbuf = (signed short *) buf;
					
					/* convert mono input to stereo */
					if (input.channels == 1)
					{
						//printf("convert: data is mono\n");

						tempbuf2 = conv_buf;
						
						if ((input.format.xmms == FMT_U8) || (input.format.xmms == FMT_S8))
						{
							tempcharbuf = buf;
							
							for (i = 0; i < buflen*2.0; i++)
							{
								*tempbuf2 = *tempcharbuf; tempbuf2++;
								*tempbuf2 = *tempcharbuf; tempbuf2++; tempcharbuf++;
							}
						} 
						else 
						{
							for (i = 0; i < buflen; i++)
							{
								*tempbuf2 = *tempbuf; tempbuf2++;
								*tempbuf2 = *tempbuf; tempbuf2++; tempbuf++;
							}
						}
						
						buflen *=2.0;
						tempbuf = conv_buf;
					}
					else
					{
						//printf("convet: data is stereo\n");
					}
					
		//printf("convert: buflen %.2f\n",buflen);


					/* convert 8 bit input to 16 bit input */
					if ((input.format.xmms != FMT_S16_LE) && (input.format.xmms != FMT_S16_BE) 
						&& (input.format.xmms != FMT_S16_NE))
					{
						switch (input.format.xmms)
						{
							case FMT_U8:
							{
								//printf("convert: converting unsigned 8 bit\n");
								
								tempbuf2 = conv_buf;
								buflen *= 2.0;
								
								/* if data was mono, then it is already in conv_buf */
								if (input.channels == 1)
								{
									for (i = 0; i < buflen; i++)
									{
										*tempbuf = (*tempbuf2 - 127) << 8;
										tempbuf++; tempbuf2++;
									}
								}
								else
								{   /* data is 8 bit stereo, and is in buf not conv_buf*/
									tempcharbuf = buf;
									for (i = 0; i < len; i++)
									{
										*tempbuf = (*tempcharbuf - 127) << 8;
										tempbuf++; tempcharbuf++;
									}			
								}
								
								tempbuf = conv_buf;
								break;
							}
							case FMT_S8:
							{
								//printf("convert: converting signed 8 bit\n");

								tempbuf2 = conv_buf;
								buflen *= 2.0;
								
								/* if data was mono, then it is already in conv_buf */
								if (input.channels == 1)
								{
									for (i = 0; i < buflen; i++)
									{
										*tempbuf = *tempbuf2 << 8;
										tempbuf++; tempbuf2++;
									}
								}
								else
								{   /* data is 8 bit stereo, and is in buf not conv_buf*/
									tempcharbuf = buf;
									for (i = 0; i < len; i++)
									{
										*tempbuf = *tempcharbuf << 8;
										tempbuf++; tempcharbuf++;
									}			
								}
								
								tempbuf = conv_buf;
								break;
							}
							default: 
							{
								errno = ERROR_BAD_FORMAT; 
								//ch->writing = 0;
								return FAILURE;
							}
						}
					}	
				} /* end default case*/

		//printf("convert: buflen %.2f\n",buflen);



#ifdef COMPILE_SAMPLER
		} /* end switch sampler_state */

		/* copy buffer to sample buffer if sampler state is record */
		if (local_sampler_state == SAMPLER_RECORD)
		{
			tempsize = 0;

			/* get amount of data to copy */
			if ((ch->sampler_size + (buflen * 2)) > ch->sampler_bufsize)
			{
				tempsize = ch->sampler_bufsize - ch->sampler_size;
			}
			else
			{
				tempsize = (buflen * 2);
			}

			/* change state if buffer is full */
			if (tempsize == 0)
			{
				ch->sampler_state = SAMPLER_READY;
			}
			
			/* copy data */
			memcpy(((ch->sampler_buf) + (ch->sampler_size)),tempbuf,tempsize);

			/* update sampler state variables */
			ch->sampler_size += tempsize;
			ch->sampler_endoffset = ch->sampler_size;
		}
#endif

		if (local_pitch == 1.0)
		{
			//printf("convert: pitch optimization buflen %.2f *2 %.2f\n",buflen,buflen*2);
			//printf("tempbuf is 0x%x, output_buf is 0x%x\n",tempbuf,output_buf);
			memcpy(output_buf,tempbuf,buflen*2);
			
			outlen = buflen*2;
			tempbuf = output_buf;
			output_buf_length = buflen;
			
			goto done;
		}

		/* calculate pitch shifted signal using basic linear interpolation
		   the theory is this:
		   you have two known samples, and want to calculate the value of a new sample
		   in between them.  The new sample will contain a percentage of the first sample
		   and a percentage of the second sample.  These percentages are porportional
		   to the distance between the new sample and each of the knwon samples.
		   The "position" of the new sample is determined by the float index */

		tempoutbuf = output_buf;

		while (intindex < buflen)
		{
			/* calculate sample percentages (amplitude) */
			intindex = floor(float_index);
			gain2 = float_index - intindex;
			gain1 = 1.0 - gain2;

			/* get index of first sample pair */
			intindex = intindex << 1;
			
			/* check incr_flag to see if we should be operatiing 
			   on the left or right channel sample */
			if (incr_flag) 
			{ 
				float_index += local_pitch; 
				incr_flag = 0; 
				intindex++;
			}
			else
			{
				incr_flag = 1;
			}
			
			index1 = intindex;
			
			/* get the first "known" sample*/
			sample1 = tempbuf[index1];
			index2 = index1 + 2; 
			
			/* get the second "known" sample */
			if (index2 < (buflen))
			{
				sample2 = tempbuf[index2];
			}
			else
				/* if index2 is beyond the length of the input buffer,
                   then cheat to prevent audio pops/snaps/etc */
			{
				*tempoutbuf = sample1;
				sampleindex++;
				break;
			}
			
			/* create the new sample */
			output_sample = (((float)sample1 * gain1) + ((float)sample2 * gain2));
			
			if (output_sample > 32767)  {output_sample = 32767;}
			if (output_sample < -32767)	{output_sample = -32767;}
			
			*tempoutbuf = output_sample;
			tempoutbuf++;
			
			sampleindex++;
		}

		/* update global variables */
		outlen = (sampleindex-1) << 1;
		
		float_index = float_index - floor(float_index);
		
		tempbuf = output_buf;

		output_buf_length = sampleindex - 1;
		
		/* 	if (outlen < PIPE_BUF)
			{errno = ERROR_TOO_LITTLE_DATA; return FAILURE;} */

	apply_gain:

	
	done:

		//ch->writing = 0;

		if (sampler_flag)
		{
			return 0;
		}
		else
		{
			return len;
		}
	}

#ifdef __cplusplus
}
#endif

