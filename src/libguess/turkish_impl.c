#include "libguess.h"
#include "dfa.h"
#include "guess_tab.c"

/* precedence order */
#define ORDER &utf8, &iso8859_9, &cp1254

/* common */
const char *guess_tr(const char *buf, int buflen)
{
    int i;
    const char *rv = NULL;

    /* encodings */
    guess_dfa cp1254 = DFA_INIT(guess_cp1253_st, guess_cp1253_ar, "CP1254");
    guess_dfa iso8859_9 = DFA_INIT(guess_iso8859_9_st, guess_iso8859_9_ar, "ISO-8859-9");
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
