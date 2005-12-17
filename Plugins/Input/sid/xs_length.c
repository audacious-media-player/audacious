/*  
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   Get song length from SLDB for PSID/RSID files
   
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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "xs_length.h"
#include "xs_support.h"
#include "xs_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


/* Database handling functions
 */
static t_xs_sldb_node *xs_sldb_node_new(void)
{
	t_xs_sldb_node *pResult;

	/* Allocate memory for new node */
	pResult = (t_xs_sldb_node *) g_malloc0(sizeof(t_xs_sldb_node));
	if (!pResult)
		return NULL;

	return pResult;
}


static void xs_sldb_node_free(t_xs_sldb_node * pNode)
{
	if (pNode) {
		/* Nothing much to do here ... */
		g_free(pNode);
	}
}


/* Insert given node to db linked list
 */
static void xs_sldb_node_insert(t_xs_sldb * db, t_xs_sldb_node * pNode)
{
	assert(db);

	if (db->pNodes) {
		/* The first node's pPrev points to last node */
		LPREV = db->pNodes->pPrev;	/* New node's prev = Previous last node */
		db->pNodes->pPrev->pNext = pNode;	/* Previous last node's next = New node */
		db->pNodes->pPrev = pNode;	/* New last node = New node */
		LNEXT = NULL;	/* But next is NULL! */
	} else {
		db->pNodes = pNode;	/* First node ... */
		LPREV = pNode;	/* ... it's also last */
		LNEXT = NULL;	/* But next is NULL! */
	}
}


/* Parses a time-entry in SLDB format
 */
static gint32 xs_sldb_gettime(gchar * pcStr, size_t * piPos)
{
	gint32 iResult, iTemp;

	/* Check if it starts with a digit */
	if (isdigit(pcStr[*piPos])) {
		/* Get minutes-field */
		iResult = 0;
		while (isdigit(pcStr[*piPos]))
			iResult = (iResult * 10) + (pcStr[(*piPos)++] - '0');

		iResult *= 60;

		/* Check the field separator char */
		if (pcStr[*piPos] == ':') {
			/* Get seconds-field */
			(*piPos)++;
			iTemp = 0;
			while (isdigit(pcStr[*piPos]))
				iTemp = (iTemp * 10) + (pcStr[(*piPos)++] - '0');

			iResult += iTemp;
		} else
			iResult = -2;
	} else
		iResult = -1;

	/* Ignore and skip the possible attributes */
	while (pcStr[*piPos] && !isspace(pcStr[*piPos]))
		(*piPos)++;

	return iResult;
}


/* Read database to memory
 */
gint xs_sldb_read(t_xs_sldb * db, gchar * dbFilename)
{
	FILE *inFile;
	gchar inLine[XS_BUF_SIZE];
	size_t lineNum, linePos;
	gboolean iOK;
	t_xs_sldb_node *tmpNode;
	assert(db);

	/* Try to open the file */
	if ((inFile = fopen(dbFilename, "ra")) == NULL) {
		XSERR("Could not open SongLengthDB '%s'\n", dbFilename);
		return -1;
	}

	/* Read and parse the data */
	lineNum = 0;

	while (!feof(inFile)) {
		fgets(inLine, XS_BUF_SIZE, inFile);
		inLine[XS_BUF_SIZE - 1] = 0;
		lineNum++;

		/* Check if it is datafield */
		if (isxdigit(inLine[0])) {
			/* Check the length of the hash */
			linePos = 0;
			while (isxdigit(inLine[linePos]))
				linePos++;

			if (linePos != XS_MD5HASH_LENGTH_CH) {
				XSERR("Invalid hash in SongLengthDB file '%s' line #%d!\n", dbFilename, lineNum);
			} else {
				/* Allocate new node */
				if ((tmpNode = xs_sldb_node_new()) == NULL) {
					XSERR("Error allocating new node. Fatal error.\n");
					exit(5);
				}

				/* Get hash value */
#if (XS_MD5HASH_LENGTH != 16)
#error Mismatch in hashcode length. Fix here.
#endif
				sscanf(&inLine[0], "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
				       (guint *) & (tmpNode->md5Hash[0]), (guint *) & (tmpNode->md5Hash[1]),
				       (guint *) & (tmpNode->md5Hash[2]), (guint *) & (tmpNode->md5Hash[3]),
				       (guint *) & (tmpNode->md5Hash[4]), (guint *) & (tmpNode->md5Hash[5]),
				       (guint *) & (tmpNode->md5Hash[6]), (guint *) & (tmpNode->md5Hash[7]),
				       (guint *) & (tmpNode->md5Hash[8]), (guint *) & (tmpNode->md5Hash[9]),
				       (guint *) & (tmpNode->md5Hash[10]), (guint *) & (tmpNode->md5Hash[11]),
				       (guint *) & (tmpNode->md5Hash[12]), (guint *) & (tmpNode->md5Hash[13]),
				       (guint *) & (tmpNode->md5Hash[14]), (guint *) & (tmpNode->md5Hash[15]));

				/* Get playtimes */
				if (inLine[linePos] != 0) {
					if (inLine[linePos] != '=') {
						XSERR("'=' expected in SongLengthDB file '%s' line #%d, column #%d\n",
						      dbFilename, lineNum, linePos);

						xs_sldb_node_free(tmpNode);
					} else {
						/* First playtime is after '=' */
						linePos++;
						iOK = TRUE;

						while ((linePos < strlen(inLine)) && iOK) {
							xs_findnext(inLine, &linePos);

							if (tmpNode->nLengths < XS_STIL_MAXENTRY) {
								tmpNode->sLengths[tmpNode->nLengths] =
								    xs_sldb_gettime(inLine, &linePos);
								tmpNode->nLengths++;
							} else
								iOK = FALSE;
						}

						/* Add an node to db in memory */
						if (iOK)
							xs_sldb_node_insert(db, tmpNode);
						else
							xs_sldb_node_free(tmpNode);
					}
				}
			}
		} else if ((inLine[0] != ';') && (inLine[0] != '[')) {
			XSERR("Invalid line in SongLengthDB file '%s' line #%d\n", dbFilename, lineNum);
		}

	}			/* while */

	/* Close the file */
	fclose(inFile);

	return 0;
}


/* Compare two given MD5-hashes.
 * Return: 0 if equal
 *         negative if testHash1 < testHash2
 *         positive if testHash1 > testHash2
 */
static gint xs_sldb_cmphash(t_xs_md5hash testHash1, t_xs_md5hash testHash2)
{
	register gint i, res = 0;

	/* Compute difference of hashes */
	for (i = 0; (i < XS_MD5HASH_LENGTH) && (!res); i++)
		res = (testHash1[i] - testHash2[i]);

	return res;
}


/* Get node from db index via binary search
 */
static t_xs_sldb_node *xs_sldb_get_node(t_xs_sldb * db, t_xs_md5hash pHash)
{
	gint iStartNode, iEndNode, iQNode, r, i;
	gboolean iFound;
	t_xs_sldb_node *pResult;

	/* Check the database pointers */
	if (!db || !db->pNodes || !db->ppIndex)
		return NULL;

	/* Look-up via index using binary search */
	pResult = NULL;
	iStartNode = 0;
	iEndNode = (db->n - 1);
	iQNode = (iEndNode / 2);
	iFound = FALSE;

	while ((!iFound) && ((iEndNode - iStartNode) > XS_BIN_BAILOUT)) {
		r = xs_sldb_cmphash(pHash, db->ppIndex[iQNode]->md5Hash);
		if (r < 0) {
			/* Hash was in the <- LEFT side */
			iEndNode = iQNode;
			iQNode = iStartNode + ((iEndNode - iStartNode) / 2);
		} else if (r > 0) {
			/* Hash was in the RIGHT -> side */
			iStartNode = iQNode;
			iQNode = iStartNode + ((iEndNode - iStartNode) / 2);
		} else
			iFound = TRUE;
	}

	/* If not found already */
	if (!iFound) {
		/* Search the are linearly */
		iFound = FALSE;
		i = iStartNode;
		while ((i <= iEndNode) && (!iFound)) {
			if (xs_sldb_cmphash(pHash, db->ppIndex[i]->md5Hash) == 0)
				iFound = TRUE;
			else
				i++;
		}

		/* Check the result */
		if (iFound)
			pResult = db->ppIndex[i];

	} else {
		/* Found via binary search */
		pResult = db->ppIndex[iQNode];
	}

	return pResult;
}


/* Compare two nodes
 */
static gint xs_sldb_cmp(const void *pNode1, const void *pNode2)
{
	/* We assume here that we never ever get NULL-pointers or similar */
	return xs_sldb_cmphash((*(t_xs_sldb_node **) pNode1)->md5Hash, (*(t_xs_sldb_node **) pNode2)->md5Hash);
}


/* (Re)create index
 */
gint xs_sldb_index(t_xs_sldb * db)
{
	t_xs_sldb_node *pCurr;
	gint i;
	assert(db);

	/* Free old index */
	if (db->ppIndex) {
		g_free(db->ppIndex);
		db->ppIndex = NULL;
	}

	/* Get size of db */
	pCurr = db->pNodes;
	db->n = 0;
	while (pCurr) {
		db->n++;
		pCurr = pCurr->pNext;
	}

	/* Check number of nodes */
	if (db->n > 0) {
		/* Allocate memory for index-table */
		db->ppIndex = (t_xs_sldb_node **) g_malloc(sizeof(t_xs_sldb_node *) * db->n);
		if (!db->ppIndex)
			return -1;

		/* Get node-pointers to table */
		i = 0;
		pCurr = db->pNodes;
		while (pCurr && (i < db->n)) {
			db->ppIndex[i++] = pCurr;
			pCurr = pCurr->pNext;
		}

		/* Sort the indexes */
		qsort(db->ppIndex, db->n, sizeof(t_xs_sldb_node *), xs_sldb_cmp);
	}

	return 0;
}


/* Free a given song-length database
 */
void xs_sldb_free(t_xs_sldb * db)
{
	t_xs_sldb_node *pCurr, *pNext;

	if (!db)
		return;

	/* Free the memory allocated for nodes */
	pCurr = db->pNodes;
	while (pCurr) {
		pNext = pCurr->pNext;
		xs_sldb_node_free(pCurr);
		pCurr = pNext;
	}

	db->pNodes = NULL;

	/* Free memory allocated for index */
	if (db->ppIndex) {
		g_free(db->ppIndex);
		db->ppIndex = NULL;
	}

	/* Free structure */
	db->n = 0;
	g_free(db);
}


/* Compute md5hash of given SID-file
 */
typedef struct
{
	gchar magicID[4];	/* "PSID" / "RSID" magic identifier */
	guint16 version,	/* Version number */
	 dataOffset,		/* Start of actual c64 data in file */
	 loadAddress,		/* Loading address */
	 initAddress,		/* Initialization address */
	 playAddress,		/* Play one frame */
	 nSongs,		/* Number of subsongs */
	 startSong;		/* Default starting song */
	guint32 speed;		/* Speed */
	gchar sidName[32];	/* Descriptive text-fields, ASCIIZ */
	gchar sidAuthor[32];
	gchar sidCopyright[32];
} t_xs_psidv1_header;


typedef struct
{
	guint16 flags;		/* Flags */
	guint8 startPage, pageLength;
	guint16 reserved;
} t_xs_psidv2_header;


static gint xs_get_sid_hash(gchar * pcFilename, t_xs_md5hash hash)
{
	FILE *inFile;
	t_xs_md5state inState;
	t_xs_psidv1_header psidH;
	t_xs_psidv2_header psidH2;
#ifdef XS_BUF_DYNAMIC
	guint8 *songData;
#else
	guint8 songData[XS_SIDBUF_SIZE];
#endif
	guint8 ib8[2], i8;
	gint iIndex, iRes;

	/* Try to open the file */
	if ((inFile = fopen(pcFilename, "rb")) == NULL)
		return -1;

	/* Read PSID header in */
	xs_rd_str(inFile, psidH.magicID, sizeof(psidH.magicID));
	if ((psidH.magicID[0] != 'P' && psidH.magicID[0] != 'R') ||
	    (psidH.magicID[1] != 'S') || (psidH.magicID[2] != 'I') || (psidH.magicID[3] != 'D')) {
		fclose(inFile);
		return -2;
	}

	psidH.version = xs_rd_be16(inFile);
	psidH.dataOffset = xs_rd_be16(inFile);
	psidH.loadAddress = xs_rd_be16(inFile);
	psidH.initAddress = xs_rd_be16(inFile);
	psidH.playAddress = xs_rd_be16(inFile);
	psidH.nSongs = xs_rd_be16(inFile);
	psidH.startSong = xs_rd_be16(inFile);
	psidH.speed = xs_rd_be32(inFile);

	xs_rd_str(inFile, psidH.sidName, sizeof(psidH.sidName));
	xs_rd_str(inFile, psidH.sidAuthor, sizeof(psidH.sidAuthor));
	xs_rd_str(inFile, psidH.sidCopyright, sizeof(psidH.sidCopyright));

	/* Check if we need to load PSIDv2NG header ... */
	if (psidH.version == 2) {
		/* Yes, we need to */
		psidH2.flags = xs_rd_be16(inFile);
		psidH2.startPage = fgetc(inFile);
		psidH2.pageLength = fgetc(inFile);
		psidH2.reserved = xs_rd_be16(inFile);
	}
#ifdef XS_BUF_DYNAMIC
	/* Allocate buffer */
	songData = (guint8 *) g_malloc(XS_SIDBUF_SIZE * sizeof(guint8));
	if (!songData) {
		fclose(inFile);
		return -3;
	}
#endif

	/* Read data to buffer */
	iRes = fread(songData, sizeof(guint8), XS_SIDBUF_SIZE, inFile);
	fclose(inFile);

	/* Initialize and start MD5-hash calculation */
	xs_md5_init(&inState);
	if (psidH.loadAddress == 0) {
		/* Strip load address (2 first bytes) */
		xs_md5_append(&inState, &songData[2], iRes - 2);
	} else {
		/* Append "as is" */
		xs_md5_append(&inState, songData, iRes);
	}


#ifdef XS_BUF_DYNAMIC
	/* Free buffer */
	g_free(songData);
#endif

	/* Append header data to hash */
#define XSADDHASH(QDATAB) { ib8[0] = (QDATAB & 0xff); ib8[1] = (QDATAB >> 8); xs_md5_append(&inState, (guint8 *) &ib8, sizeof(ib8)); }

	XSADDHASH(psidH.initAddress)
	    XSADDHASH(psidH.playAddress)
	    XSADDHASH(psidH.nSongs)
#undef XSADDHASH
	    /* Append song speed data to hash */
	    i8 = 0;
	for (iIndex = 0; (iIndex < psidH.nSongs) && (iIndex < 32); iIndex++) {
		i8 = (psidH.speed & (1 << iIndex)) ? 60 : 0;
		xs_md5_append(&inState, &i8, sizeof(i8));
	}

	/* Rest of songs (more than 32) */
	for (iIndex = 32; iIndex < psidH.nSongs; iIndex++) {
		xs_md5_append(&inState, &i8, sizeof(i8));
	}


	/* PSIDv2NG specific */
	if (psidH.version == 2) {
		/* SEE SIDPLAY HEADERS FOR INFO */
		i8 = (psidH2.flags >> 2) & 3;
		if (i8 == 2)
			xs_md5_append(&inState, &i8, sizeof(i8));
	}

	/* Calculate the hash */
	xs_md5_finish(&inState, hash);

	return 0;
}


/* Get song lengths
 */
t_xs_sldb_node *xs_sldb_get(t_xs_sldb * db, gchar * pcFilename)
{
	t_xs_sldb_node *pResult;
	t_xs_md5hash dbHash;

	/* Get the hash and then look up from db */
	if (xs_get_sid_hash(pcFilename, dbHash) == 0)
		pResult = xs_sldb_get_node(db, dbHash);
	else
		pResult = NULL;

	return pResult;
}


/*
 * These should be moved out of this module some day ...
 */
static t_xs_sldb *xs_sldb_db = NULL;
extern GStaticMutex xs_cfg_mutex;
GStaticMutex xs_sldb_db_mutex = G_STATIC_MUTEX_INIT;

gint xs_songlen_init(void)
{
	g_static_mutex_lock(&xs_cfg_mutex);

	if (!xs_cfg.songlenDBPath) {
		g_static_mutex_unlock(&xs_cfg_mutex);
		return -1;
	}

	g_static_mutex_lock(&xs_sldb_db_mutex);

	/* Check if already initialized */
	if (xs_sldb_db)
		xs_sldb_free(xs_sldb_db);

	/* Allocate database */
	xs_sldb_db = (t_xs_sldb *) g_malloc0(sizeof(t_xs_sldb));
	if (!xs_sldb_db) {
		g_static_mutex_unlock(&xs_cfg_mutex);
		g_static_mutex_unlock(&xs_sldb_db_mutex);
		return -2;
	}

	/* Read the database */
	if (xs_sldb_read(xs_sldb_db, xs_cfg.songlenDBPath) != 0) {
		xs_sldb_free(xs_sldb_db);
		xs_sldb_db = NULL;
		g_static_mutex_unlock(&xs_cfg_mutex);
		g_static_mutex_unlock(&xs_sldb_db_mutex);
		return -3;
	}

	/* Create index */
	if (xs_sldb_index(xs_sldb_db) != 0) {
		xs_sldb_free(xs_sldb_db);
		xs_sldb_db = NULL;
		g_static_mutex_unlock(&xs_cfg_mutex);
		g_static_mutex_unlock(&xs_sldb_db_mutex);
		return -4;
	}

	g_static_mutex_unlock(&xs_cfg_mutex);
	g_static_mutex_unlock(&xs_sldb_db_mutex);
	return 0;
}


void xs_songlen_close(void)
{
	g_static_mutex_lock(&xs_sldb_db_mutex);
	xs_sldb_free(xs_sldb_db);
	xs_sldb_db = NULL;
	g_static_mutex_unlock(&xs_sldb_db_mutex);
}


t_xs_sldb_node *xs_songlen_get(gchar * pcFilename)
{
	t_xs_sldb_node *pResult;

	g_static_mutex_lock(&xs_sldb_db_mutex);

	if (xs_cfg.songlenDBEnable && xs_sldb_db)
		pResult = xs_sldb_get(xs_sldb_db, pcFilename);
	else
		pResult = NULL;

	g_static_mutex_unlock(&xs_sldb_db_mutex);

	return pResult;
}
