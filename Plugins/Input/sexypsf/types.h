#ifndef _SPSF_TYPES_H__
#define _SPSF_TYPES_H__

#include <inttypes.h>

#define INLINE inline

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
        
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;



/* For gcc only? */
#define PACKSTRUCT	__attribute__ ((packed))

#endif
