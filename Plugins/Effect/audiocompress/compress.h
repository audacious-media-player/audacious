/* compress.h
** interface to audio compression
*/

#ifndef COMPRESS_H
#define COMPRESS_H

void CompressCfg(int anticlip,
		 int target,
		 int maxgain,
		 int smooth,
		 int buckets);

void CompressDo(void *data, unsigned int numSamples);

void CompressFree(void);

#endif
