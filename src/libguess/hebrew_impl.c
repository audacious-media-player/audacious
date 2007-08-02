const char *_guess_hw(const unsigned char *ptr, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        if (ptr[i] == 0x80 || (ptr[i] >= 0x82 && ptr[i] <= 0x89) || ptr[i] == 0x8B ||
            (ptr[i] >= 0x91 && ptr[i] <= 0x99) || ptr[i] == 0x9B || ptr[i] == 0xA1 ||
            (ptr[i] >= 0xBF && ptr[i] <= 0xC9) ||
            (ptr[i] >= 0xCB && ptr[i] <= 0xD8))
            return "CP1255";

        if (ptr[i] == 0xDF)
            return "ISO-8859-8-I";
    }

    return "ISO-8859-8-I";
}

const char *guess_hw(const char *ptr, int size)
{
    return _guess_hw((const unsigned char *) ptr, size);
}
