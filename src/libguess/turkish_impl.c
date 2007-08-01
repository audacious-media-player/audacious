#include "libguess.h"

static const char *_guess_tr(const unsigned char *ptr, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        if (ptr[i] == 0x80 || 
            (ptr[i] >= 0x82 && ptr[i] <= 0x8C) ||
            (ptr[i] >= 0x91 && ptr[i] <= 0x9C) || 
            ptr[ i ] == 0x9F)
            return "CP1254";
    }

    return "ISO-8859-9";
}

const char *guess_tr(const char *ptr, int size)
{
    if (dfa_validate_utf8(ptr, size))
        return "UTF-8";

    return _guess_tr((const unsigned char *)ptr, size);
}
