
// CPU Byte Order Utilities

// Game_Music_Emu 0.2.4. Copyright (C) 2005 Shay Green. BSD license.

#ifndef BLARGG_ENDIAN
#define BLARGG_ENDIAN

inline unsigned get_le16( const void* p ) {
	return *((unsigned char*) p + 1) * 0x100u + *(unsigned char*) p;
}

inline void set_le16( void* p, unsigned n ) {
	*(unsigned char*) p = n;
	*((unsigned char*) p + 1) = n >> 8;
}

#ifndef GET_LE16

	#if 0
		// Read 16-bit little-endian unsigned integer from memory
		unsigned GET_LE16( const void* );

		// Write 16-bit little-endian integer to memory
		void SET_LE16( void*, unsigned );
	#endif

	// Optimized implementation if byte order is known
	#if BLARGG_NONPORTABLE && BLARGG_LITTLE_ENDIAN
		#define GET_LE16( addr )        (*(unsigned short*) (addr))
		#define SET_LE16( addr, data )  (void (*(unsigned short*) (addr) = (data)))

	#elif BLARGG_NONPORTABLE && BLARGG_CPU_POWERPC
		// PowerPC has special byte-reversed instructions
		#define GET_LE16( addr )        ((unsigned) __lhbrx( (addr), 0 ))
		#define SET_LE16( addr, data )  (__sthbrx( (data), (addr), 0 ))

	#else
		#define GET_LE16( addr )        get_le16( addr )
		#define SET_LE16( addr, data )  set_le16( addr, data )

	#endif

#endif

#endif

