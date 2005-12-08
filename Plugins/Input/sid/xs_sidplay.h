/*
 * Here comes the really ugly code...
 * Get all SID-tune information (for all sub-tunes)
 * including name, length, etc.
 */
t_xs_tuneinfo *TFUNCTION(gchar * pcFilename)
{
	t_xs_sldb_node *tuneLength = NULL;
	t_xs_tuneinfo *pResult;
	TTUNEINFO tuneInfo;
	TTUNE *testTune;
	gboolean haveInfo = TRUE;
	gint i;

	/* Check if the tune exists and is readable */
	if ((testTune = new TTUNE(pcFilename)) == NULL)
		return NULL;

	if (!testTune->getStatus()) {
		delete testTune;
		return NULL;
	}

	/* Get general tune information */
#ifdef _XS_SIDPLAY1_H
	haveInfo = testTune->getInfo(tuneInfo);
#endif
#ifdef _XS_SIDPLAY2_H
	testTune->getInfo(tuneInfo);
	haveInfo = TRUE;
#endif

	/* Get length information (NOTE: Do not free this!) */
	tuneLength = xs_songlen_get(pcFilename);

	/* Allocate tuneinfo structure */
	pResult = xs_tuneinfo_new(pcFilename,
				  tuneInfo.songs, tuneInfo.startSong,
				  tuneInfo.infoString[0], tuneInfo.infoString[1], tuneInfo.infoString[2],
				  tuneInfo.loadAddr, tuneInfo.initAddr, tuneInfo.playAddr, tuneInfo.dataFileLen);

	if (!pResult) {
		delete testTune;
		return NULL;
	}

	/* Get information for subtunes */
	for (i = 0; i < pResult->nsubTunes; i++) {
		/* Make the title */
		if (haveInfo) {
			pResult->subTunes[i].tuneTitle =
			    xs_make_titlestring(pcFilename, i + 1, pResult->nsubTunes, tuneInfo.sidModel,
						tuneInfo.formatString, tuneInfo.infoString[0],
						tuneInfo.infoString[1], tuneInfo.infoString[2]);
		} else
			pResult->subTunes[i].tuneTitle = g_strdup(pcFilename);

		/* Get song length */
		if (tuneLength && (i < tuneLength->nLengths))
			pResult->subTunes[i].tuneLength = tuneLength->sLengths[i];
		else
			pResult->subTunes[i].tuneLength = -1;
	}

	delete testTune;

	return pResult;
}

/* Undefine these */
#undef TFUNCTION
#undef TTUNEINFO
#undef TTUNE
