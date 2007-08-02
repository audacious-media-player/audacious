static const char *_guess_gr(const unsigned char *ptr, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        if (ptr[i] == 0x80 ||
            (ptr[i] >= 0x82 && ptr[i] <= 0x87) ||
            ptr[i] == 0x89 || ptr[i] == 0x8B ||
            (ptr[i] >= 0x91 && ptr[i] <= 0x97) ||
            ptr[i] == 0x99 || ptr[i] == 0x9B || ptr[i] == 0xA4 ||
            ptr[i] == 0xA5 || ptr[i] == 0xAE)
            return "CP1253";
    }

    return "ISO-8859-7";
}

const char *guess_gr(const char *ptr, int size)
{
    return _guess_gr((const unsigned char *) ptr, size);
}
