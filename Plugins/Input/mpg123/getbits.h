
/*
 * This does the same as getbits.c but with defines to
 * force inlining
 */

#define mpg123_backbits(nob)			\
do {						\
	bsi.bitindex    -= nob;			\
	bsi.wordpointer += (bsi.bitindex >> 3);	\
	bsi.bitindex    &= 0x7;			\
} while (0)

#define mpg123_getbitoffset() ((-bsi.bitindex) & 0x7)
#define mpg123_getbyte()      (*bsi.wordpointer++)

#define mpg123_getbits(nob)			\
	(rval = bsi.wordpointer[0],		\
	rval <<= 8,				\
	rval |= bsi.wordpointer[1],		\
	rval <<= 8,				\
	rval |= bsi.wordpointer[2],		\
	rval <<= bsi.bitindex,			\
	rval &= 0xffffff,			\
	bsi.bitindex += (nob),			\
	rval >>= (24-(nob)),			\
	bsi.wordpointer += (bsi.bitindex>>3),	\
	bsi.bitindex &= 7,			\
	rval)

#define mpg123_getbits_fast(nob)						\
	(rval = (unsigned char) (bsi.wordpointer[0] << bsi.bitindex),		\
	rval |= ((unsigned long) bsi.wordpointer[1] << bsi.bitindex) >> 8,	\
	rval <<= (nob),								\
	rval >>= 8,								\
	bsi.bitindex += (nob),							\
	bsi.wordpointer += (bsi.bitindex >> 3),					\
	bsi.bitindex &= 7,							\
	rval)

#define mpg123_get1bit()				\
	(rval_uc = *bsi.wordpointer << bsi.bitindex,	\
	bsi.bitindex++,					\
	bsi.wordpointer += (bsi.bitindex>>3),		\
	bsi.bitindex &= 7,				\
	rval_uc >> 7)
