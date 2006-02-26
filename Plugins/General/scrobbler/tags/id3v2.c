/*
 *   libmetatag - A media file tag-reader library
 *   Copyright (C) 2003, 2004  Pipian
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/bmp_vfs.h"
#include "include/id3v2.h"
#include "include/endian.h"
#include "../fmt.h"
#include "../config.h"
#include "include/unicode.h"
#define BUFFER_SIZE 4096

id3_lookup_t id3v22_lookup[] = 
{
	{"BUF", ID3V22_BUF}, {"CNT", ID3V22_CNT}, {"COM", ID3V22_COM}, 
	{"CRA", ID3V22_CRA}, {"CRM", ID3V22_CRM}, {"ETC", ID3V22_ETC}, 
	{"EQU", ID3V22_EQU}, {"GEO", ID3V22_GEO}, {"IPL", ID3V22_IPL}, 
	{"LNK", ID3V22_LNK}, {"MCI", ID3V22_MCI}, {"MLL", ID3V22_MLL}, 
	{"PIC", ID3V22_PIC}, {"POP", ID3V22_POP}, {"REV", ID3V22_REV}, 
	{"RVA", ID3V22_RVA}, {"SLT", ID3V22_SLT}, {"STC", ID3V22_STC}, 
	{"TAL", ID3V22_TAL}, {"TBP", ID3V22_TBP}, {"TCM", ID3V22_TCM}, 
	{"TCO", ID3V22_TCO}, {"TCR", ID3V22_TCR}, {"TDA", ID3V22_TDA}, 
	{"TDY", ID3V22_TDY}, {"TEN", ID3V22_TEN}, {"TFT", ID3V22_TFT}, 
	{"TIM", ID3V22_TIM}, {"TKE", ID3V22_TKE}, {"TLA", ID3V22_TLA}, 
	{"TLE", ID3V22_TLE}, {"TMT", ID3V22_TMT}, {"TOA", ID3V22_TOA}, 
	{"TOF", ID3V22_TOF}, {"TOL", ID3V22_TOL}, {"TOR", ID3V22_TOR}, 
	{"TOT", ID3V22_TOT}, {"TP1", ID3V22_TP1}, {"TP2", ID3V22_TP2}, 
	{"TP3", ID3V22_TP3}, {"TP4", ID3V22_TP4}, {"TPA", ID3V22_TPA}, 
	{"TPB", ID3V22_TPB}, {"TRC", ID3V22_TRC}, {"TRD", ID3V22_TRD}, 
	{"TRK", ID3V22_TRK}, {"TSI", ID3V22_TSI}, {"TSS", ID3V22_TSS}, 
	{"TT1", ID3V22_TT1}, {"TT2", ID3V22_TT2}, {"TT3", ID3V22_TT3}, 
	{"TXT", ID3V22_TXT}, {"TXX", ID3V22_TXX}, {"TYE", ID3V22_TYE}, 
	{"UFI", ID3V22_UFI}, {"ULT", ID3V22_ULT}, {"WAF", ID3V22_WAF}, 
	{"WAR", ID3V22_WAR}, {"WAS", ID3V22_WAS}, {"WCM", ID3V22_WCM}, 
	{"WCP", ID3V22_WCP}, {"WPB", ID3V22_WPB}, {"WXX", ID3V22_WXX},
	{NULL, -1}
};

id3_lookup_t id3v24_lookup[] =
{
	{"AENC", ID3V24_AENC}, {"APIC", ID3V24_APIC}, {"ASPI", ID3V24_ASPI}, 
	{"COMM", ID3V24_COMM}, {"COMR", ID3V24_COMR}, {"ENCR", ID3V24_ENCR}, 
	{"EQU2", ID3V24_EQU2}, {"ETCO", ID3V24_ETCO}, {"GEOB", ID3V24_GEOB}, 
	{"GRID", ID3V24_GRID}, {"LINK", ID3V24_LINK}, {"MCDI", ID3V24_MCDI}, 
	{"MLLT", ID3V24_MLLT}, {"OWNE", ID3V24_OWNE}, {"PRIV", ID3V24_PRIV}, 
	{"PCNT", ID3V24_PCNT}, {"POPM", ID3V24_POPM}, {"POSS", ID3V24_POSS}, 
	{"RBUF", ID3V24_RBUF}, {"RVA2", ID3V24_RVA2}, {"RVRB", ID3V24_RVRB}, 
	{"SEEK", ID3V24_SEEK}, {"SIGN", ID3V24_SIGN}, {"SYLT", ID3V24_SYLT}, 
	{"SYTC", ID3V24_SYTC}, {"TALB", ID3V24_TALB}, {"TBPM", ID3V24_TBPM}, 
	{"TCOM", ID3V24_TCOM}, {"TCON", ID3V24_TCON}, {"TCOP", ID3V24_TCOP}, 
	{"TDEN", ID3V24_TDEN}, {"TDLY", ID3V24_TDLY}, {"TDOR", ID3V24_TDOR}, 
	{"TDRC", ID3V24_TDRC}, {"TDRL", ID3V24_TDRL}, {"TDTG", ID3V24_TDTG}, 
	{"TENC", ID3V24_TENC}, {"TEXT", ID3V24_TEXT}, {"TFLT", ID3V24_TFLT}, 
	{"TIPL", ID3V24_TIPL}, {"TIT1", ID3V24_TIT1}, {"TIT2", ID3V24_TIT2}, 
	{"TIT3", ID3V24_TIT3}, {"TKEY", ID3V24_TKEY}, {"TLAN", ID3V24_TLAN}, 
	{"TLEN", ID3V24_TLEN}, {"TMCL", ID3V24_TMCL}, {"TMED", ID3V24_TMED}, 
	{"TMOO", ID3V24_TMOO}, {"TOAL", ID3V24_TOAL}, {"TOFN", ID3V24_TOFN}, 
	{"TOLY", ID3V24_TOLY}, {"TOPE", ID3V24_TOPE}, {"TOWN", ID3V24_TOWN}, 
	{"TPE1", ID3V24_TPE1}, {"TPE2", ID3V24_TPE2}, {"TPE3", ID3V24_TPE3}, 
	{"TPE4", ID3V24_TPE4}, {"TPOS", ID3V24_TPOS}, {"TPRO", ID3V24_TPRO}, 
	{"TPUB", ID3V24_TPUB}, {"TRCK", ID3V24_TRCK}, {"TRSN", ID3V24_TRSN}, 
	{"TRSO", ID3V24_TRSO}, {"TSOA", ID3V24_TSOA}, {"TSOP", ID3V24_TSOP}, 
	{"TSOT", ID3V24_TSOT}, {"TSRC", ID3V24_TSRC}, {"TSSE", ID3V24_TSSE}, 
	{"TSST", ID3V24_TSST}, {"TXXX", ID3V24_TXXX}, {"UFID", ID3V24_UFID}, 
	{"USER", ID3V24_USER}, {"USLT", ID3V24_USLT}, {"WCOM", ID3V24_WCOM}, 
	{"WCOP", ID3V24_WCOP}, {"WOAF", ID3V24_WOAF}, {"WOAR", ID3V24_WOAR}, 
	{"WOAS", ID3V24_WOAS}, {"WORS", ID3V24_WORS}, {"WPAY", ID3V24_WPAY}, 
	{"WPUB", ID3V24_WPUB}, {"WXXX", ID3V24_WXXX}, {NULL, -1}
};

id3_lookup_t id3v23_lookup[] = 
{
	{"AENC", ID3V23_AENC}, {"APIC", ID3V23_APIC}, {"COMM", ID3V23_COMM}, 
	{"COMR", ID3V23_COMR}, {"ENCR", ID3V23_ENCR}, {"EQUA", ID3V23_EQUA}, 
	{"ETCO", ID3V23_ETCO}, {"GEOB", ID3V23_GEOB}, {"GRID", ID3V23_GRID}, 
	{"IPLS", ID3V23_IPLS}, {"LINK", ID3V23_LINK}, {"MCDI", ID3V23_MCDI}, 
	{"MLLT", ID3V23_MLLT}, {"OWNE", ID3V23_OWNE}, {"PRIV", ID3V23_PRIV}, 
	{"PCNT", ID3V23_PCNT}, {"POPM", ID3V23_POPM}, {"POSS", ID3V23_POSS}, 
	{"RBUF", ID3V23_RBUF}, {"RVAD", ID3V23_RVAD}, {"RVRB", ID3V23_RVRB}, 
	{"SYLT", ID3V23_SYLT}, {"SYTC", ID3V23_SYTC}, {"TALB", ID3V23_TALB}, 
	{"TBPM", ID3V23_TBPM}, {"TCOM", ID3V23_TCOM}, {"TCON", ID3V23_TCON}, 
	{"TCOP", ID3V23_TCOP}, {"TDAT", ID3V23_TDAT}, {"TDLY", ID3V23_TDLY}, 
	{"TENC", ID3V23_TENC}, {"TEXT", ID3V23_TEXT}, {"TFLT", ID3V23_TFLT}, 
	{"TIME", ID3V23_TIME}, {"TIT1", ID3V23_TIT1}, {"TIT2", ID3V23_TIT2}, 
	{"TIT3", ID3V23_TIT3}, {"TKEY", ID3V23_TKEY}, {"TLAN", ID3V23_TLAN}, 
	{"TLEN", ID3V23_TLEN}, {"TMED", ID3V23_TMED}, {"TOAL", ID3V23_TOAL}, 
	{"TOFN", ID3V23_TOFN}, {"TOLY", ID3V23_TOLY}, {"TOPE", ID3V23_TOPE}, 
	{"TORY", ID3V23_TORY}, {"TOWN", ID3V23_TOWN}, {"TPE1", ID3V23_TPE1}, 
	{"TPE2", ID3V23_TPE2}, {"TPE3", ID3V23_TPE3}, {"TPE4", ID3V23_TPE4}, 
	{"TPOS", ID3V23_TPOS}, {"TPUB", ID3V23_TPUB}, {"TRCK", ID3V23_TRCK}, 
	{"TRDA", ID3V23_TRDA}, {"TRSN", ID3V23_TRSN}, {"TRSO", ID3V23_TRSO}, 
	{"TSIZ", ID3V23_TSIZ}, {"TSRC", ID3V23_TSRC}, {"TSSE", ID3V23_TSSE}, 
	{"TYER", ID3V23_TYER}, {"TXXX", ID3V23_TXXX}, {"UFID", ID3V23_UFID}, 
	{"USER", ID3V23_USER}, {"USLT", ID3V23_USLT}, {"WCOM", ID3V23_WCOM}, 
	{"WCOP", ID3V23_WCOP}, {"WOAF", ID3V23_WOAF}, {"WOAR", ID3V23_WOAR}, 
	{"WOAS", ID3V23_WOAS}, {"WORS", ID3V23_WORS}, {"WPAY", ID3V23_WPAY}, 
	{"WPUB", ID3V23_WPUB}, {"WXXX", ID3V23_WXXX}, {NULL, -1}
};


typedef struct
{
	unsigned char *check;
	int count;
} resync_t;

typedef struct
{
	int unsync, extended, size;
	char version[2];
} id3header_t;

static resync_t *checkunsync(char *syncCheck, int size)
{
	int i, j;
	resync_t *sync;
	
	sync = malloc(sizeof(resync_t));

	sync->check = (unsigned char*)syncCheck;
	sync->count = 0;
	
	if(size == 0)
		size = strlen((char*)sync->check);

	for(i = 0; i < size; i++)
	{
		if(sync->check[i] == 0xFF && sync->check[i + 1] == 0x00)
		{
			for(j = i + 1; j < size - 1; j++)
				syncCheck[j] = syncCheck[j + 1];
			sync->check[j] = '\0';
			sync->count++;
		}
	}

	return sync;
}

static void unsync(char *data, char *bp)
{
	resync_t *unsynced;
	char *syncFix = NULL;
	int i;
	
	unsynced = checkunsync(data, 0);
	while(unsynced->count > 0)
	{
		if(syncFix != NULL)
			syncFix = realloc(syncFix, unsynced->count);
		else
			syncFix = malloc(unsynced->count);
		memcpy(syncFix, bp, unsynced->count);
		bp += unsynced->count;
		for(i = 0; i < unsynced->count; i++)
			data[4 - unsynced->count + i] = syncFix[i];
		free(unsynced);
		unsynced = checkunsync(data, 0);
	}
	free(unsynced);
	free(syncFix);
}

/*
 * Header:
 *
 * identifier: 3 bytes "ID3" ("3DI" if footer)
 * version: 2 bytes
 * flags: 1 byte
 * tag size: 4 bytes
 */
static id3header_t *read_header(VFSFile *fp)
{
	id3header_t *id3_data = calloc(sizeof(id3header_t), 1);
	char id3_flags, cToInt[4];
	int bottom = 0;
	
	vfs_fread(cToInt, 1, 3, fp);
	if(strncmp(cToInt, "3DI", 3) == 0)
		bottom = 1;
	vfs_fread(id3_data->version, 1, 2, fp);
	vfs_fread(&id3_flags, 1, 1, fp);
	if((id3_flags & 0x80) == 0x80)
		id3_data->unsync = 1;
	if((id3_flags & 0x40) == 0x40 && id3_data->version[0] > 0x02)
		id3_data->extended = 1;
	vfs_fread(cToInt, 1, 4, fp);
	id3_data->size = synchsafe2int(cToInt);
	if(bottom == 1)
		vfs_fseek(fp, -10 - id3_data->size, SEEK_CUR);
	
#ifdef META_DEBUG
	if(id3_data->version[0] == 0x04)
	{
		pdebug("Version: ID3v2.4", META_DEBUG);
	}
	else if(id3_data->version[0] == 0x03)
	{
		pdebug("Version: ID3v2.3", META_DEBUG);
	}
	else if(id3_data->version[0] == 0x02)
	{
		pdebug("Version: ID3v2.2", META_DEBUG);
	}
#endif
	if(id3_data->version[0] < 0x02 || id3_data->version[0] > 0x04)
	{
		free(id3_data);
		return NULL;
	}
		
	return id3_data;
}

static int id3_lookupframe(char *tag, int tagver)
{
	int i;

	switch (tagver) {
	case ID3v22:
		for (i = 0; id3v22_lookup[i].frameid; i++)
			if (!strcmp(tag, id3v22_lookup[i].frameid))
				return id3v22_lookup[i].code;
		return -1;
		break;
	case ID3v23:
		for (i = 0; id3v23_lookup[i].frameid; i++)
			if (!strcmp(tag, id3v23_lookup[i].frameid))
				return id3v23_lookup[i].code;
		return -1;
		break;
	case ID3v24:
		for (i = 0; id3v23_lookup[i].frameid; i++)
			if (!strcmp(tag, id3v24_lookup[i].frameid))
				return id3v24_lookup[i].code;
		return -1;
		break;
	}
	return -1;
}

static framedata_t *parseFrame(char **bp, char *end, id3header_t *id3_data)
{
	static unsigned char frameid[5];
	unsigned char frameflags[2] = "", cToInt[5];
	int framesize, frameidcode;
	framedata_t *framedata;

	/* TODO: Unsync, decompress, decrypt, grouping, data length */
	switch (id3_data->version[0]) {
	case 2:
		if (end - *bp < 6)
			return NULL;
		frameid[3] = 0;
		memcpy(frameid, *bp, 3);
		*bp += 3;
		frameidcode = id3_lookupframe((char*)frameid, ID3v22);
		memcpy(cToInt, *bp, 3);
		cToInt[3] = 0;
		if(id3_data->unsync)
			unsync((char*)cToInt, *bp);
		framesize = be24int(cToInt);
		*bp += 3;
		break;
	case 3:
		if (end - *bp < 10)
			return NULL;
		frameid[4] = 0;
		memcpy(frameid, *bp, 4);
		*bp += 4;
		frameidcode = id3_lookupframe((char*)frameid, ID3v23);
		memcpy(cToInt, *bp, 4);
		if(id3_data->unsync)
			unsync((char*)cToInt, *bp);
		framesize = be2int(cToInt);
		*bp += 4;

		memcpy(frameflags, *bp, 2);
		*bp += 2;
		break;
	case 4:
		if (end - *bp < 10)
			return NULL;
		frameid[4] = 0;
		memcpy(frameid, *bp, 4);
		*bp += 4;
		frameidcode = id3_lookupframe((char*)frameid, ID3v24);
		memcpy(cToInt, *bp, 4);
		framesize = synchsafe2int(cToInt);
		*bp += 4;

		memcpy(frameflags, *bp, 2);
		*bp += 2;
		break;
	default:
		/* Should not be reached */
		return NULL;
	}

	/* printf("found id:%s, size:%-8d\n", frameid, framesize); */
	if (framesize > end - *bp)
		return NULL;
	framedata = calloc(sizeof(framedata_t), 1);
	framedata->frameid = frameidcode;
	if(id3_data->unsync)
		frameflags[1] |= 0x02;
	framedata->flags = malloc(2);
	memcpy(framedata->flags, frameflags, 2);
	if(id3_data->version[0] == 0x04)
	{
		/*
		 * 4 bytes extra for compression or original size
		 * 1 byte extra for encyption
		 * 1 byte extra for grouping
		 */
		if((frameflags[1] & 0x08) == 0x08 ||
			(frameflags[1] & 0x01) == 0x01)
		{
			*bp += 4;
			framesize -= 4;
		}
		if((frameflags[1] & 0x04) == 0x04)
		{
			*bp++;
			framesize--;
		}
		if((frameflags[1] & 0x40) == 0x40)
		{
			*bp++;
			framesize--;
		}
	}
	else if(id3_data->version[0] == 0x03)
	{
		/*
		 * 4 bytes extra for compression or original size
		 * 1 byte extra for encyption
		 * 1 byte extra for grouping
		 */
		if((frameflags[1] & 0x80) == 0x80)
		{
			char tmp[4];
			
			memcpy(tmp, *bp, 4);
			*bp += 4;
			if((frameflags[1] & 0x02) == 0x02)
				unsync(tmp, *bp);
			framesize -= 4;
		}
		if((frameflags[1] & 0x40) == 0x40)
		{
			*bp++;
			framesize--;
		}
		if((frameflags[1] & 0x20) == 0x20)
		{
			*bp++;
			framesize--;
		}
	}
	framedata->len = framesize;
	framedata->data = malloc(framesize);
	memcpy(framedata->data, *bp, framesize);
	*bp += framesize;
	
	/* Parse text appropriately to UTF-8. */
	if(frameid[0] == 'T' && strcmp((char*)frameid, "TXXX") &&
		strcmp((char*)frameid, "TXX"))
	{
		unsigned char *ptr, *data = NULL, *utf = NULL;
		int encoding;
		
		ptr = framedata->data;

		if(framedata->len == 0)
		{
			framedata->data = realloc(framedata->data, 1);
			framedata->data[0] = '\0';
			return framedata;
		}
		
		encoding = *(ptr++);
		data = realloc(data, framedata->len);
		*(data + framedata->len - 1) = '\0';
		memcpy(data, ptr, framedata->len - 1);
		if((framedata->flags[1] & 0x02) == 0x02)
		{
			resync_t *unsync = checkunsync((char*)data, framedata->len);
			framedata->len -= unsync->count;
			free(unsync);
		}
		if(encoding == 0x00)
		{
			if(utf != NULL)
				free(utf);
			iso88591_to_utf8(data, framedata->len - 1, &utf);
		}
		else if(encoding == 0x01)
		{
			if(utf != NULL)
				free(utf);
			utf16bom_to_utf8(data, framedata->len - 1, &utf);
		}
		else if(encoding == 0x02)
		{
			if(utf != NULL)
				free(utf);
			utf16be_to_utf8(data, framedata->len - 1, &utf);
		}
		else if(encoding == 0x03)
		{
			utf = realloc(utf, framedata->len);
			strcpy((char*)utf, (char*)data);
		}
		framedata->len = strlen((char*)utf) + 1;
		framedata->data = realloc(framedata->data, framedata->len);
		strcpy((char*)framedata->data, (char*)utf);
		framedata->data[framedata->len - 1] = '\0';
		free(utf);
		free(data);
	}
	/* Or unsync. */
	else
	{
		unsigned char *data = NULL, *ptr;
		
		ptr = framedata->data;
		
		data = realloc(data, framedata->len);
		// *(data + framedata->len - 1) = '\0';
		memcpy(data, ptr, framedata->len);
		if((framedata->flags[1] & 0x02) == 0x02)
		{
			resync_t *unsync = checkunsync((char*)data, framedata->len);
			framedata->len -= unsync->count;
			free(unsync);
		}
		framedata->data = realloc(framedata->data, framedata->len);
		memcpy(framedata->data, data, framedata->len);
		free(data);
	}
	
	return framedata;
}

static id3v2_t *readFrames(char *bp, char *end, id3header_t *id3_data)
{
	id3v2_t *id3v2 = calloc(sizeof(id3v2_t), 1);

	while(bp < end)
	{
		framedata_t *framedata = NULL;

		if(*bp == '\0')
			break;
		
		framedata = parseFrame(&bp, end, id3_data);
		
		id3v2->items = realloc(id3v2->items, (id3v2->numitems + 1) * 
			sizeof(framedata_t *));
		id3v2->items[id3v2->numitems++] = framedata;
	}
	id3v2->version = id3_data->version[0];
	
	return id3v2;
}

/*unsigned char *ID3v2_parseText(framedata_t *frame)
{
	unsigned char *data = NULL, *utf = NULL, *ptr;
	char encoding;
	
	ptr = frame->data;
	
	encoding = *(ptr++);
	data = realloc(data, frame->len);
	memset(data, '\0', frame->len);
	memcpy(data, ptr, frame->len - 1);
	if((frame->flags[1] & 0x02) == 0x02)
	{
		resync_t *unsync = checkunsync(data, 0);
		free(unsync);
	}
	if(encoding == 0x00)
	{
		if(utf != NULL)
			free(utf);
		iso88591_to_utf8(data, &utf);
	}
	else if(encoding == 0x01)
	{
		if(utf != NULL)
			free(utf);
		utf16bom_to_utf8(data, frame->len - 1, &utf);
	}
	else if(encoding == 0x02)
	{
		if(utf != NULL)
			free(utf);
		utf16be_to_utf8(data, frame->len - 1, &utf);
	}
	else if(encoding == 0x03)
	{
		utf = realloc(utf, strlen(data) + 1);
		strcpy(utf, data);
	}
	free(data);
	
	return utf;
}

unsigned char *ID3v2_getData(framedata_t *frame)
{
	unsigned char *data = NULL, *ptr;
	
	ptr = frame->data;
	
	data = realloc(data, frame->len + 1);
	memset(data, '\0', frame->len + 1);
	memcpy(data, ptr, frame->len);
	if((frame->flags[1] & 0x02) == 0x02)
	{
		resync_t *unsync = checkunsync(data, frame->len);
		free(unsync);
	}
	
	return data;
}*/

int findID3v2(VFSFile *fp)
{
	unsigned char tag_buffer[BUFFER_SIZE], *bp = tag_buffer;
	int pos, search = -1, i, status = 0, charsRead;
	
	charsRead = vfs_fread(tag_buffer, 1, 10, fp);
	pos = 0;
	bp = tag_buffer;
	while(status == 0 && !feof(fp))
	{
		if(search == -1)
		{
			if((strncmp((char*)bp, "ID3", 3) == 0 ||
				strncmp((char*)bp, "3DI", 3) == 0))
					status = 1;
			else
			{
				vfs_fseek(fp, 3, SEEK_END);
				charsRead = vfs_fread(tag_buffer, 1, 3, fp);
				search = -2;
			}
		}
		else
		{
			if(search == -2)
			{
				bp = tag_buffer;
				pos = vfs_ftell(fp);
				if((strncmp((char*)bp, "ID3", 3) == 0 ||
					strncmp((char*)bp, "3DI", 3) == 0)) status = 1;
				search = 1;
			}
			if(status != 1)
			{
				pos = vfs_ftell(fp) - BUFFER_SIZE;
				vfs_fseek(fp, pos, SEEK_SET);
				charsRead = vfs_fread(tag_buffer, 1, BUFFER_SIZE,
						fp);

				bp = tag_buffer;
				for(i = 0; i < charsRead - 3 && status == 0;
					i++)
				{
					bp++;
					if((strncmp((char*)bp, "ID3", 3) == 0 ||
						strncmp((char*)bp, "3DI", 3) == 0))
							status = 1;
				}
				if(status == 1)
					pos += bp - tag_buffer;
				pos -= BUFFER_SIZE - 9;
				if((pos < -BUFFER_SIZE + 9 || ferror(fp)) &&
					status != 1)
						status = -1;
			}
		}
		/*
		 * An ID3v2 tag can be detected with the following pattern:
		 *
		 * $49 44 33 yy yy xx zz zz zz zz
		 *
		 * Where yy is less than $FF, xx is the 'flags' byte and zz is 
		 * less than $80.
		 */
		if(status == 1 && *(bp + 3) < 0xFF && *(bp + 4) < 0xFF &&
			*(bp + 6) < 0x80 && *(bp + 7) < 0x80 &&
			*(bp + 8) < 0x80 && *(bp + 9) < 0x80)
				status = 1;
		else if(status != -1)
			status = 0;
		if(search == 0)
			search = -1;
	}
	if(status < 0 || feof(fp))
		return -1;
	else
		return pos;
}

id3v2_t *readID3v2(char *filename)
{
	VFSFile *fp;
	id3v2_t *id3v2 = NULL;
	int pos;
	
	fp = vfs_fopen(filename, "rb");

	if(!fp)
	{
		pdebug("Couldn't open file!", META_DEBUG);
		return NULL;
	}
	
	vfs_fseek(fp, 0, SEEK_SET);

	pdebug("Searching for tag...", META_DEBUG);
	pos = findID3v2(fp);
	if(pos > -1)
	{
		id3header_t *id3_data;
		char *tag_buffer, *bp;
		
		/* Found the tag. */
		vfs_fseek(fp, pos, SEEK_SET);
		pdebug("Found ID3v2 tag...", META_DEBUG);
		/* Read the header */
		id3_data = read_header(fp);
		if(id3_data == NULL)
		{
			pdebug("Or not.", META_DEBUG);
			vfs_fclose(fp);
			return NULL;
		}
		/* Read tag into buffer */
		tag_buffer = malloc(id3_data->size);
		vfs_fread(tag_buffer, 1, id3_data->size, fp);
		bp = tag_buffer;
		/* Skip extended header */
		if(id3_data->extended)
		{
			char cToInt[4];
			
			memcpy(cToInt, bp, 4);
			bp += 4;
			if(id3_data->version[0] == 0x03 &&
				id3_data->unsync == 1)
					unsync(cToInt, bp);
			if(id3_data->version[0] > 0x03)
				bp += synchsafe2int(cToInt);
			else
				bp += be2int(cToInt);
		}
		/* Read frames into id3v2_t */
		id3v2 = readFrames(bp, tag_buffer + id3_data->size, id3_data);

		free(tag_buffer);
		free(id3_data);
	}
	
	vfs_fclose(fp);
	
	return id3v2;
}

void freeID3v2(id3v2_t *id3v2)
{
	int i;
	
	for(i = 0; i < id3v2->numitems; i++)
	{
		framedata_t *frame;
		
		frame = id3v2->items[i];
		free(frame->flags);
		free(frame->data);
		free(frame);
	}
	free(id3v2->items);
	free(id3v2);
}
