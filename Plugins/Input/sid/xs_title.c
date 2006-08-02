/*  
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   Titlestring handling
   
   Programmed and designed by Matti 'ccr' Hamalainen <ccr@tnsp.org>
   (C) Copyright 1999-2005 Tecnic Software productions (TNSP)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
*/
#include "xs_title.h"
#include "xs_support.h"
#include "xs_config.h"
#include "libaudacious/titlestring.h"


/*
 * Create a title string based on given information and settings.
 */
#define VPUTCH(MCH)	\
if (iIndex < XS_BUF_SIZE) tmpBuf[iIndex++] = MCH;
#define VPUTSTR(MSTR) {						\
	if (MSTR) {						\
		if ((iIndex + strlen(MSTR) + 1) < XS_BUF_SIZE) {	\
			strcpy(&tmpBuf[iIndex], MSTR);		\
			iIndex += strlen(MSTR); 		\
			} else					\
			iIndex = XS_BUF_SIZE;			\
		}						\
	}


gchar *xs_make_titlestring(gchar * pcFilename, gint iSubTune, gint nSubTunes, gint iSidModel,
			   const gchar * formatString, const gchar * infoString0,
			   const gchar * infoString1, const gchar * infoString2)
{
	gchar *tmpFilename, *tmpFilePath, *tmpFileExt, *pcStr, *pcResult, tmpStr[XS_BUF_SIZE], tmpBuf[XS_BUF_SIZE];
	gint iIndex;
#ifdef HAVE_XMMSEXTRA
	TitleInput *ptInput;
#endif

	/* Split the filename into path */
	tmpFilePath = g_strdup(pcFilename);
	tmpFilename = xs_strrchr(tmpFilePath, '/');
	if (tmpFilename)
		tmpFilename[1] = 0;

	/* Filename */
	tmpFilename = xs_strrchr(pcFilename, '/');
	if (tmpFilename)
		tmpFilename = g_strdup(tmpFilename + 1);
	else
		tmpFilename = g_strdup(pcFilename);

	tmpFileExt = xs_strrchr(tmpFilename, '.');
	tmpFileExt[0] = 0;

	/* Extension */
	tmpFileExt = xs_strrchr(pcFilename, '.');


#ifdef HAVE_XMMSEXTRA
	/* Check if the titles are overridden or not */
	if (!xs_cfg.titleOverride) {
		/* Use generic XMMS titles */
		/* XMMS_NEW_TITLEINPUT(ptInput);
		 * We duplicate and add typecast to the code here due to XMMS's braindead headers
		 */
		ptInput = (TitleInput *) g_malloc0(sizeof(TitleInput));
		ptInput->__size = XMMS_TITLEINPUT_SIZE;
		ptInput->__version = XMMS_TITLEINPUT_VERSION;

		/* Create the input fields */
		ptInput->file_name = tmpFilename;
		ptInput->file_ext = tmpFileExt;
		ptInput->file_path = tmpFilePath;

		ptInput->track_name = g_strdup(infoString0);
		ptInput->track_number = iSubTune;
		ptInput->album_name = NULL;
		ptInput->performer = g_strdup(infoString1);
		ptInput->date = g_strdup((iSidModel == XS_SIDMODEL_6581) ? "SID6581" : "SID8580");

		ptInput->year = 0;
		ptInput->genre = g_strdup("SID-tune");
		ptInput->comment = g_strdup(infoString2);

		/* Create the string */
		pcResult = xmms_get_titlestring(xmms_get_gentitle_format(), ptInput);

		/* Dispose all allocated memory */
		g_free(ptInput->track_name);
		g_free(ptInput->performer);
		g_free(ptInput->comment);
		g_free(ptInput->date);
		g_free(ptInput->genre);
		g_free(ptInput);
	} else
#endif
	{
		/* Create the string */
		pcStr = xs_cfg.titleFormat;
		iIndex = 0;
		while (*pcStr && (iIndex < XS_BUF_SIZE)) {
			if (*pcStr == '%') {
				pcStr++;
				switch (*pcStr) {
				case '%':
					VPUTCH('%');
					break;
				case 'f':
					VPUTSTR(tmpFilename);
					break;
				case 'F':
					VPUTSTR(tmpFilePath);
					break;
				case 'e':
					VPUTSTR(tmpFileExt);
					break;
				case 'p':
					VPUTSTR(infoString1);
					break;
				case 't':
					VPUTSTR(infoString0);
					break;
				case 'c':
					VPUTSTR(infoString2);
					break;
				case 's':
					VPUTSTR(formatString);
					break;
				case 'm':
					switch (iSidModel) {
					case XS_SIDMODEL_6581:
						VPUTSTR("6581");
						break;
					case XS_SIDMODEL_8580:
						VPUTSTR("8580");
						break;
					default:
						VPUTSTR("Unknown");
						break;
					}
					break;
				case 'n':
					snprintf(tmpStr, XS_BUF_SIZE, "%i", iSubTune);
					VPUTSTR(tmpStr);
					break;
				case 'N':
					snprintf(tmpStr, XS_BUF_SIZE, "%i", nSubTunes);
					VPUTSTR(tmpStr);
					break;
				}
			} else {
				VPUTCH(*pcStr);
			}
			pcStr++;
		}

		tmpBuf[iIndex] = 0;

		/* Make resulting string */
		pcResult = g_strdup(tmpBuf);
	}

	/* Free temporary strings */
	g_free(tmpFilename);
	g_free(tmpFilePath);

	return pcResult;
}
