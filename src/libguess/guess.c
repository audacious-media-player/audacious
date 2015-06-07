#include <stdlib.h>
#include <strings.h>

#include "libguess.h"

struct guess_impl {
    const char *lang;
    guess_impl_f impl;
};

/* keep these in alphabetical order! */
static const struct guess_impl guess_impl_list[] = {
    {"arabic", guess_ar},
    {"baltic", guess_bl},
    {"chinese", guess_cn},
    {"greek", guess_gr},
    {"hebrew", guess_hw},
    {"japanese", guess_jp},
    {"korean", guess_kr},
    {"polish", guess_pl},
    {"russian", guess_ru},
    {"taiwanese", guess_tw},
    {"turkish", guess_tr},
};

static int
guess_cmp_impl(const void *key, const void *ptr)
{
    const struct guess_impl *impl = ptr;
    return strcasecmp(key, impl->lang);
}

static guess_impl_f
guess_find_impl(const char *lang)
{
    struct guess_impl *impl =
        bsearch(lang, guess_impl_list,
                sizeof guess_impl_list / sizeof(struct guess_impl),
                sizeof(struct guess_impl), guess_cmp_impl);

    return (impl != NULL) ? impl->impl : NULL;
}

const char *
libguess_determine_encoding(const char *inbuf, int buflen, const char *lang)
{
    guess_impl_f impl = guess_find_impl(lang);

    if (impl != NULL)
        return impl(inbuf, buflen);

    /* TODO: try other languages as fallback? */
    return NULL;
}
