#include "libguess.h"

typedef struct _guess_impl {
    struct _guess_impl *next;
    const char *name;
    const char *(*impl)(const char *buf, int len);
} guess_impl;

static guess_impl *guess_impl_list = NULL;

void guess_impl_register(const char *lang,
    const char *(*impl)(const char *buf, int len))
{
    guess_impl *iptr = calloc(sizeof(guess_impl), 1);

    iptr->name = lang;
    iptr->impl = impl;
    iptr->next = guess_impl_list;

    guess_impl_list = iptr;
}

void guess_init(void)
{
    /* check if already initialized */
    if (guess_impl_list != NULL)
        return;

    guess_impl_register(GUESS_REGION_JP, guess_jp);
    guess_impl_register(GUESS_REGION_TW, guess_tw);
    guess_impl_register(GUESS_REGION_CN, guess_cn);
    guess_impl_register(GUESS_REGION_KR, guess_kr);
    guess_impl_register(GUESS_REGION_RU, guess_ru);
    guess_impl_register(GUESS_REGION_AR, guess_ar);
    guess_impl_register(GUESS_REGION_TR, guess_tr);
    guess_impl_register(GUESS_REGION_GR, guess_gr);
    guess_impl_register(GUESS_REGION_HW, guess_hw);
}

const char *guess_encoding(const char *inbuf, int buflen, const char *lang)
{
    guess_impl *iter;

    guess_init();

    for (iter = guess_impl_list; iter != NULL; iter = iter->next)
    {
        if (!strcasecmp(lang, iter->name))
           return iter->impl(inbuf, buflen);
    }

    /* TODO: try other languages as fallback? */

    return NULL;
}
