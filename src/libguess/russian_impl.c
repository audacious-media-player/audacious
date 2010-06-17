#include "libguess.h"
#include "dfa.h"
#include "guess_tab.c"

/* precedence order */
#define ORDER &utf8, &cp1251, &koi8_u, &koi8_r, &cp866, &iso8859_2, &iso8859_5

/* common */
const char *guess_ru(const char *buf, int buflen)
{
    int i;
    const char *rv = NULL;

    /* encodings */
    guess_dfa utf8 = DFA_INIT(guess_utf8_st, guess_utf8_ar, "UTF-8");
    guess_dfa cp1251 = DFA_INIT(guess_cp1251_st, guess_cp1251_ar, "CP1251");
    guess_dfa cp866 = DFA_INIT(guess_cp866_st, guess_cp866_ar, "CP866");
    guess_dfa koi8_u = DFA_INIT(guess_koi8_u_st, guess_koi8_u_ar, "KOI8-U");
    guess_dfa koi8_r = DFA_INIT(guess_koi8_r_st, guess_koi8_r_ar, "KOI8-R");
    guess_dfa iso8859_2 = DFA_INIT(guess_iso8859_2_st, guess_iso8859_2_ar, "ISO-8859-2");
    guess_dfa iso8859_5 = DFA_INIT(guess_iso8859_5_st, guess_iso8859_5_ar, "ISO-8859-5");

    guess_dfa *top = NULL;
    guess_dfa *order[] = { ORDER, NULL };

    for (i = 0; i < buflen; i++) {
        int c = (unsigned char) buf[i];

        /* special treatment of BOM */
        if (i == 0 && c == 0xff) {
            if (i < buflen - 1) {
                c = (unsigned char) buf[i + 1];
                if (c == 0xfe)
                    return UCS_2LE;
            }
        }
        if (i == 0 && c == 0xfe) {
            if (i < buflen - 1) {
                c = (unsigned char) buf[i + 1];
                if (c == 0xff)
                    return UCS_2BE;
            }
        }

        rv = dfa_process(order, c);
        if(rv)
            return rv;

        if (dfa_none(order)) {
            /* we ran out the possibilities */
            return NULL;
        }
    }

    top = dfa_top(order);
    if (top)
        return top->name;
    else
        return NULL;
}
