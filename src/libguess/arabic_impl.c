#include "libguess.h"
#include "dfa.h"
#include "guess_tab.c"

/* precedence order */
#define ORDER &utf8, &iso8859_6, &cp1256

const char *guess_ar(const char *buf, int buflen)
{
    int i;
    const char *rv = NULL;

    /* encodings */
    guess_dfa cp1256 = DFA_INIT(guess_cp1256_st, guess_cp1256_ar, "CP1256");
    guess_dfa iso8859_6 = DFA_INIT(guess_iso8859_6_st, guess_iso8859_6_ar, "ISO-8859-6");
    guess_dfa utf8 = DFA_INIT(guess_utf8_st, guess_utf8_ar, "UTF-8");

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
