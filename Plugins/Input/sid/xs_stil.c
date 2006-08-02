/*  
   XMMS-SID - SIDPlay input plugin for X MultiMedia System (XMMS)

   STIL-database handling functions
   
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
#include "xs_stil.h"
#include "xs_support.h"
#include "xs_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


/* Database handling functions
 */
static t_xs_stil_node *xs_stildb_node_new(gchar * pcFilename)
{
	t_xs_stil_node *pResult;

	/* Allocate memory for new node */
	pResult = (t_xs_stil_node *) g_malloc0(sizeof(t_xs_stil_node));
	if (!pResult)
		return NULL;

	pResult->pcFilename = g_strdup(pcFilename);
	if (!pResult->pcFilename) {
		g_free(pResult);
		return NULL;
	}

	return pResult;
}


static void xs_stildb_node_free(t_xs_stil_node * pNode)
{
	gint i;

	if (pNode) {
		/* Free subtune information */
		for (i = 0; i <= XS_STIL_MAXENTRY; i++) {
			g_free(pNode->subTunes[i].pName);
			g_free(pNode->subTunes[i].pAuthor);
			g_free(pNode->subTunes[i].pInfo);
		}

		g_free(pNode->pcFilename);
		g_free(pNode);
	}
}


/* Insert given node to db linked list
 */
static void xs_stildb_node_insert(t_xs_stildb * db, t_xs_stil_node * pNode)
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


/* Read database (additively) to given db-structure
 */
#define XS_STILDB_MULTI if (isMulti) { isMulti = FALSE; xs_pstrcat(&(tmpNode->subTunes[subEntry].pInfo), "\n"); }


gint xs_stildb_read(t_xs_stildb * db, gchar * dbFilename)
{
	FILE *inFile;
	gchar inLine[XS_BUF_SIZE + 16];	/* Since we add some chars here and there */
	guint lineNum, linePos, eolPos;
	t_xs_stil_node *tmpNode;
	gboolean isError, isMulti;
	gint subEntry;
	assert(db);

	/* Try to open the file */
	if ((inFile = fopen(dbFilename, "ra")) == NULL) {
		XSERR("Could not open STILDB '%s'\n", dbFilename);
		return -1;
	}

	/* Read and parse the data */
	lineNum = 0;
	isError = FALSE;
	isMulti = FALSE;
	tmpNode = NULL;
	subEntry = 0;

	while (!feof(inFile) && !isError) {
		fgets(inLine, XS_BUF_SIZE, inFile);
		inLine[XS_BUF_SIZE - 1] = 0;
		linePos = eolPos = 0;
		xs_findeol(inLine, &eolPos);
		inLine[eolPos] = 0;
		lineNum++;

		switch (inLine[0]) {
		case '/':
			/* Check if we are already parsing entry */
			isMulti = FALSE;
			if (tmpNode) {
				XSERR("New entry ('%s') before end of current ('%s')! Possibly malformed STIL-file!\n",
				      inLine, tmpNode->pcFilename);

				xs_stildb_node_free(tmpNode);
			}

			/* A new node */
			subEntry = 0;
			tmpNode = xs_stildb_node_new(inLine);
			if (!tmpNode) {
				/* Allocation failed */
				XSERR("Could not allocate new STILdb-node for '%s'!\n", inLine);
				isError = TRUE;
			}
			break;

		case '(':
			/* A new sub-entry */
			isMulti = FALSE;
			linePos++;
			if (inLine[linePos] == '#') {
				linePos++;
				if (inLine[linePos]) {
					xs_findnum(inLine, &linePos);
					inLine[linePos] = 0;
					subEntry = atol(&inLine[2]);

					/* Sanity check */
					if ((subEntry < 1) || (subEntry > XS_STIL_MAXENTRY)) {
						XSERR("Number of subEntry (%i) for '%s' is invalid\n", subEntry,
						      tmpNode->pcFilename);
						subEntry = 0;
					}
				}
			}

			break;

		case 0:
		case '#':
		case '\n':
		case '\r':
			/* End of entry/field */
			isMulti = FALSE;
			if (tmpNode) {
				/* Insert to database */
				xs_stildb_node_insert(db, tmpNode);
				tmpNode = NULL;
			}
			break;

		default:
			/* Check if we are parsing an entry */
			if (!tmpNode) {
				XSERR("Entry data encountered outside of entry!\n");
				break;
			}

			/* Some other type */
			if (strncmp(inLine, "   NAME:", 8) == 0) {
				XS_STILDB_MULTI g_free(tmpNode->subTunes[subEntry].pName);
				tmpNode->subTunes[subEntry].pName = g_strdup(&inLine[9]);
			} else if (strncmp(inLine, " AUTHOR:", 8) == 0) {
				XS_STILDB_MULTI g_free(tmpNode->subTunes[subEntry].pAuthor);
				tmpNode->subTunes[subEntry].pAuthor = g_strdup(&inLine[9]);
			} else if (strncmp(inLine, "  TITLE:", 8) == 0) {
				XS_STILDB_MULTI inLine[eolPos++] = '\n';
				inLine[eolPos++] = 0;
				xs_pstrcat(&(tmpNode->subTunes[subEntry].pInfo), &inLine[2]);
			} else if (strncmp(inLine, " ARTIST:", 8) == 0) {
				XS_STILDB_MULTI inLine[eolPos++] = '\n';
				inLine[eolPos++] = 0;
				xs_pstrcat(&(tmpNode->subTunes[subEntry].pInfo), &inLine[1]);
			} else if (strncmp(inLine, "COMMENT:", 8) == 0) {
				XS_STILDB_MULTI isMulti = TRUE;
				xs_pstrcat(&(tmpNode->subTunes[subEntry].pInfo), inLine);
			} else if (strncmp(inLine, "        ", 8) == 0) {
				xs_pstrcat(&(tmpNode->subTunes[subEntry].pInfo), &inLine[8]);
			}
			break;
		}

	}			/* while */

	/* Check if there is one remaining node */
	if (tmpNode)
		xs_stildb_node_insert(db, tmpNode);

	/* Close the file */
	fclose(inFile);

	return 0;
}


/* Compare two nodes
 */
static gint xs_stildb_cmp(const void *pNode1, const void *pNode2)
{
	/* We assume here that we never ever get NULL-pointers or similar */
	return strcmp((*(t_xs_stil_node **) pNode1)->pcFilename, (*(t_xs_stil_node **) pNode2)->pcFilename);
}


/* (Re)create index
 */
gint xs_stildb_index(t_xs_stildb * db)
{
	t_xs_stil_node *pCurr;
	gint i;

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
		db->ppIndex = (t_xs_stil_node **) g_malloc(sizeof(t_xs_stil_node *) * db->n);
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
		qsort(db->ppIndex, db->n, sizeof(t_xs_stil_node *), xs_stildb_cmp);
	}

	return 0;
}

/* Free a given STIL database
 */
void xs_stildb_free(t_xs_stildb * db)
{
	t_xs_stil_node *pCurr, *pNext;

	if (!db)
		return;

	/* Free the memory allocated for nodes */
	pCurr = db->pNodes;
	while (pCurr) {
		pNext = pCurr->pNext;
		xs_stildb_node_free(pCurr);
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


/* Get STIL information node from database
 */
static t_xs_stil_node *xs_stildb_get_node(t_xs_stildb * db, gchar * pcFilename)
{
	gint iStartNode, iEndNode, iQNode, r, i;
	gboolean iFound;
	t_xs_stil_node *pResult;

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
		r = strcmp(pcFilename, db->ppIndex[iQNode]->pcFilename);
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
			if (strcmp(pcFilename, db->ppIndex[i]->pcFilename) == 0)
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


/*
 * These should be moved out of this module some day ...
 */
static t_xs_stildb *xs_stildb_db = NULL;
GStaticMutex xs_stildb_db_mutex = G_STATIC_MUTEX_INIT;
extern GStaticMutex xs_cfg_mutex;

gint xs_stil_init(void)
{
	g_static_mutex_lock(&xs_cfg_mutex);

	if (!xs_cfg.stilDBPath) {
		g_static_mutex_unlock(&xs_cfg_mutex);
		return -1;
	}

	g_static_mutex_lock(&xs_stildb_db_mutex);

	/* Check if already initialized */
	if (xs_stildb_db)
		xs_stildb_free(xs_stildb_db);

	/* Allocate database */
	xs_stildb_db = (t_xs_stildb *) g_malloc0(sizeof(t_xs_stildb));
	if (!xs_stildb_db) {
		g_static_mutex_unlock(&xs_cfg_mutex);
		g_static_mutex_unlock(&xs_stildb_db_mutex);
		return -2;
	}

	/* Read the database */
	if (xs_stildb_read(xs_stildb_db, xs_cfg.stilDBPath) != 0) {
		xs_stildb_free(xs_stildb_db);
		xs_stildb_db = NULL;
		g_static_mutex_unlock(&xs_cfg_mutex);
		g_static_mutex_unlock(&xs_stildb_db_mutex);
		return -3;
	}

	/* Create index */
	if (xs_stildb_index(xs_stildb_db) != 0) {
		xs_stildb_free(xs_stildb_db);
		xs_stildb_db = NULL;
		g_static_mutex_unlock(&xs_cfg_mutex);
		g_static_mutex_unlock(&xs_stildb_db_mutex);
		return -4;
	}

	g_static_mutex_unlock(&xs_cfg_mutex);
	g_static_mutex_unlock(&xs_stildb_db_mutex);
	return 0;
}


void xs_stil_close(void)
{
	g_static_mutex_lock(&xs_stildb_db_mutex);
	xs_stildb_free(xs_stildb_db);
	xs_stildb_db = NULL;
	g_static_mutex_unlock(&xs_stildb_db_mutex);
}


t_xs_stil_node *xs_stil_get(gchar * pcFilename)
{
	t_xs_stil_node *pResult;
	gchar *tmpFilename;

	g_static_mutex_lock(&xs_stildb_db_mutex);
	g_static_mutex_lock(&xs_cfg_mutex);

	if (xs_cfg.stilDBEnable && xs_stildb_db) {
		if (xs_cfg.hvscPath) {
			/* Remove postfixed directory separator from HVSC-path */
			tmpFilename = xs_strrchr(xs_cfg.hvscPath, '/');
			if (tmpFilename && (tmpFilename[1] == 0))
				tmpFilename[0] = 0;

			/* Remove HVSC location-prefix from filename */
			tmpFilename = strstr(pcFilename, xs_cfg.hvscPath);
			if (tmpFilename)
				tmpFilename += strlen(xs_cfg.hvscPath);
			else
				tmpFilename = pcFilename;
		} else
			tmpFilename = pcFilename;

		pResult = xs_stildb_get_node(xs_stildb_db, tmpFilename);
	} else
		pResult = NULL;

	g_static_mutex_unlock(&xs_stildb_db_mutex);
	g_static_mutex_unlock(&xs_cfg_mutex);

	return pResult;
}
