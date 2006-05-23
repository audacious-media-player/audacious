
/* that's the same file as getits.c but with defines to
  force inlining */

unsigned long rval;
unsigned char rval_uc;

#define mpgdec_backbits(bitbuf,nob) ((void)( \
  (*(bitbuf)).bitindex    -= (nob), \
  (*(bitbuf)).wordpointer += ((*(bitbuf)).bitindex>>3), \
  (*(bitbuf)).bitindex    &= 0x7 ))

#define mpgdec_getbitoffset(bitbuf) ((-(*(bitbuf)).bitindex)&0x7)
#define mpgdec_getbyte(bitbuf)      (*(*(bitbuf)).wordpointer++)

#define mpgdec_getbits(bitbuf,nob) ( \
  rval = (*(bitbuf)).wordpointer[0], rval <<= 8, rval |= (*(bitbuf)).wordpointer[1], \
  rval <<= 8, rval |= (*(bitbuf)).wordpointer[2], rval <<= (*(bitbuf)).bitindex, \
  rval &= 0xffffff, (*(bitbuf)).bitindex += (nob), \
  rval >>= (24-(nob)), (*(bitbuf)).wordpointer += ((*(bitbuf)).bitindex>>3), \
  (*(bitbuf)).bitindex &= 7,rval)

#define mpgdec_getbits_fast(bitbuf,nob) ( \
  rval = (unsigned char) ((*(bitbuf)).wordpointer[0] << (*(bitbuf)).bitindex), \
  rval |= ((unsigned long) (*(bitbuf)).wordpointer[1]<<(*(bitbuf)).bitindex)>>8, \
  rval <<= (nob), rval >>= 8, \
  (*(bitbuf)).bitindex += (nob), (*(bitbuf)).wordpointer += ((*(bitbuf)).bitindex>>3), \
  (*(bitbuf)).bitindex &= 7, rval )

#define mpgdec_get1bit(bitbuf) ( \
  rval_uc = *(*(bitbuf)).wordpointer << (*(bitbuf)).bitindex, (*(bitbuf)).bitindex++, \
  (*(bitbuf)).wordpointer += ((*(bitbuf)).bitindex>>3), (*(bitbuf)).bitindex &= 7, rval_uc>>7 )

