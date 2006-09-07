/*

  Definitions for dbaudiolib.c

  Author:  Bob Dean
  Copyright (c) 1999


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

#ifndef __DB_AUDIOLIB_H_
#define __DB_AUDIOLIB_H_


#ifdef __cplusplus
extern "C"
{
#endif
	
#include <glib.h>
/* #include <dbchannel.h> */
	
#define  DBAUDIOLIB_VERSION "0.9.8";
	
	
	/* DBAudioLib Error Codes */
	/* when possible the actual system error code is left in errno, since it
	   more specifically describes the error */

	enum {
		ERROR_BASE = 50000,
		ERROR_NOT_IMPLEMENTED,    /* user tried to access functionality that has not yet been implemented. */
		ERROR_BAD_CHANNEL,        /* unknown channel type */
		ERROR_BAD_PARAM,          /* function recieved a bad parameter*/
		ERROR_NO_FREE_CHANNELS,   /* all channels are in use */
		ERROR_TOO_MUCH_DATA,      /* the client gave too muxh data to dbaudiolib, conversion would overrun the internal buffers */
		ERROR_BAD_SAMPLERATE,     /* the samplerate provided is not supported */
		ERROR_BAD_CHANNELTYPE,    /* the channeltype provided is not supported */
		ERROR_BAD_NUMCH,          /* the number of audio channels (stereo or mono) is not supported */
		ERROR_BAD_FORMAT,         /* the input audio format is not supported */
		ERROR_CHANNEL_IN_USE,     /* the requested channel index passed to DBAudio_Init is already in use */
		ERROR_BAD_CHANNEL_ID,     /* the requested channel index passed to DBAudio_Init is out of range */
		ERROR_TOO_LITTLE_DATA,    /* the client gave too little data to dbaudiolib - depreciated */
		ERROR_NOT_INITIALIZED,    /* attempted to use dbaudiolib without a call to DBAudio_Init first */
		ERROR_INIT_FAILURE        /* some part of the init process not related to dbmix failed */
	};


	/* enumeration of sampler state */
	enum sampler_state_e {SAMPLER_OFF,
						  SAMPLER_RECORD,
						  SAMPLER_PLAY_SINGLE,
						  SAMPLER_PLAY_LOOP,
						  SAMPLER_READY};
	
	typedef enum sampler_state_e sampler_state;

#define DBAUDIO_MAX_VOLUME             100
#define DBAUDIO_INTERNAL_MAX_VOLUME    128
#define DBAUDIO_MIN_VOLUME             0

#define DBMIX_COPYRIGHT "Copyright (c) 2002 by Robert Michael S Dean"
#define DBMIX_VERSION   "v0.9.8"

#define SUCCESS                     0
#define FAILURE                    -1

#ifndef TRUE
#define TRUE                        1
#endif

#ifndef FALSE
#define FALSE                       0
#endif

#define MONO                        1
#define STEREO                      2


	/*
	 * Message structure used to communicate within dbmix channels
	 */

#define DBMSG_NONE         0x00000000
#define DBMSG_ALL          (0xFFFFFFFF & ~DBMSG_SAMPLERLOAD & ~DBMSG_SAMPLERSAVE)
#define DBMSG_PAUSE        0x00000001
#define DBMSG_UNPAUSE      0x00000002
#define DBMSG_PLAY         0x00000004
#define DBMSG_STOP         0x00000008
#define DBMSG_EJECT        0x00000010
#define DBMSG_REWIND       0x00000020
#define DBMSG_FFORWARD     0x00000040
#define DBMSG_NEXT         0x00000080
#define DBMSG_PREV         0x00000100
#define DBMSG_MUTE         0x00000200
#define DBMSG_UNMUTE       0x00000400
#define DBMSG_SAMPLERSIZE  0x00000800
#define DBMSG_SAMPLERSAVE  0x00001000
#define DBMSG_SAMPLERLOAD  0x00002000
#define DBMSG_SAMPLERREC   0x00004000
#define DBMSG_SAMPLERSTOP  0x00008000
#define DBMSG_SAMPLERLOOP  0x00010000
#define DBMSG_SAMPLERONCE  0x00020000

	typedef struct dbfsd_msg_s
	{
		long int   msg_type;
		float      data;
		char *     datastr;
	} dbfsd_msg;

	/* enumeration of the different channels types */
	enum channel_type_e {PIPE_CHANNEL, SOCKET_CHANNEL};

	/* DBAudioLib Prototypes for statically linked libraries */
	int    DBAudio_Init(char * name, int fmt, int rte, int numch,
						enum channel_type_e type, int chindex);
	int    DBAudio_Ready();
	int    DBAudio_Write(char* buf, int len);
	int    DBAudio_Read(char * buf, int count);
	int    DBAudio_Close();
	int    DBAudio_Set_Volume(int left, int right);
	int    DBAudio_Get_Volume(int *left, int *right);
	int    DBAudio_Pause(int value);
	char * DBAudio_Get_Version();
	char * DBAudio_Get_Channel_Name(char * name);
	int    DBAudio_Set_Channel_Name(char * name);
	enum channel_type_e DBAudio_Get_Channel_Type();
	int    DBAudio_Set_Channel_Type(enum channel_type_e type);
	int    DBAudio_Cue_Enabled();
	int    DBAudio_Set_Cue(int flag);
	int    DBAudio_Get_Cue();
	int    DBAudio_Set_Rate(int rte);
	int    DBAudio_Get_Rate();
	int    DBAudio_Set_Channels(int numch);
	int    DBAudio_Get_Channels();
	int    DBAudio_Set_Format(int fmt);
	int    DBAudio_Get_Format();
	int    DBAudio_Get_Bufsize(int input_bufsize);
	void   DBAudio_perror(char *str);
	int    DBAudio_Set_Message_Handler(void(*message_handler)(dbfsd_msg msg), int msg_flags);
	int    DBAudio_Handle_Message_Queue();
	int    DBAudio_Set_Channel_Flag(unsigned int flag);
	int    DBAudio_Clear_Channel_Flag(unsigned int flag);
	unsigned int DBAudio_Get_Channel_Flags();
	int    DBAudio_Set_Mute(int value);
	int    DBAudio_Get_Mute();
	int    DBAudio_Sampler_Record();
	int    DBAudio_Sampler_Stop();
	int    DBAudio_Sampler_Loop();
	int    DBAudio_Sampler_Single();
	int    DBAudio_Sampler_Get_Offsets(int * start_offset, int * end_offset);
	int    DBAudio_Sampler_Set_Offsets(int start_offset, int end_offset);
	int    DBAudio_Sampler_Get_Size(int * size);
	int    DBAudio_Sampler_Save(char * filename);
	int    DBAudio_Sampler_Load(char * filename);
	sampler_state DBAudio_Sampler_Get_State();

/* structure to hold audiolib functions plugin style for dynamically linked
    libraries... 
	As functions are added, they are placed at the end of the struct so as
    not to break clients using older versions of the library
*/
	typedef struct
	{ 
		int    (*DBAudio_Init)(char * name, int fmt, int rte, int numch,
							   enum channel_type_e type, int chindex);
		int    (*DBAudio_Ready)();
		int    (*DBAudio_Write)(char* buf, int len);
		int    (*DBAudio_Read)(char * buf, int count);
		int    (*DBAudio_Close)();
		int    (*DBAudio_Set_Volume)(int left, int right);
		int    (*DBAudio_Get_Volume)(int *left, int *right);
		int    (*DBAudio_Pause)(int value);
		char * (*DBAudio_Get_Version)();
		char * (*DBAudio_Get_Channel_Name)(char * name);
		int    (*DBAudio_Set_Channel_Name)(char * name);
		enum channel_type_e (*DBAudio_Get_Channel_Type)();
		int    (*DBAudio_Set_Channel_Type)(enum channel_type_e type);
		int    (*DBAudio_Cue_Enabled)();
		int    (*DBAudio_Set_Rate)(int rte);
		int    (*DBAudio_Get_Rate)();
		int    (*DBAudio_Set_Channels)(int numch);
		int    (*DBAudio_Get_Channels)();
		int    (*DBAudio_Set_Format)(int fmt);
		int    (*DBAudio_Get_Format)();
		int    (*DBAudio_Set_Cue)(int flag);
		int    (*DBAudio_Get_Cue)();
		int    (*DBAudio_Get_Bufsize)(int input_bufsize);	
		void   (*DBAudio_perror)(char *str);
		int    (*DBAudio_Set_Message_Handler)(void(*message_handler)(dbfsd_msg msg),int msg_flags);
		int    (*DBAudio_Handle_Message_Queue)();
		int    (*DBAudio_Set_Channel_Flag)(unsigned int flag);
		int    (*DBAudio_Clear_Channel_Flag)(unsigned int flag);
		unsigned int (* DBAudio_Get_Channel_Flags)();
		int    (*DBAudio_Set_Mute)(int value);
		int    (*DBAudio_Get_Mute)();
		int    (*DBAudio_Sampler_Record)();
		int    (*DBAudio_Sampler_Stop)();
		int    (*DBAudio_Sampler_Loop)();
		int    (*DBAudio_Sampler_Single)();
		int    (*DBAudio_Sampler_Get_Offsets)(int * start_offset, int * end_offset);
		int    (*DBAudio_Sampler_Set_Offsets)(int start_offset, int end_offset);
		int    (*DBAudio_Sampler_Get_Size)(int * size);
		int    (*DBAudio_Sampler_Save)(char * filename);
		int    (*DBAudio_Sampler_Load)(char * filename);
		sampler_state (*DBAudio_Sampler_Get_State)();
	} DBAudioLibFunctions;
	
	DBAudioLibFunctions * DBAudio_Get_Functions();
	
	
#ifdef __cplusplus
}
#endif

#endif /* __DB_AUDIOLIB_H_ */
