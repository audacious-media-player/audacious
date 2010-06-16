#include "libguess.h"
#include "dfa.h"

boolean
dfa_alone(guess_dfa *dfa, guess_dfa *order[])
{
    int i;

    if (dfa->state < 0)
        return FALSE;

    for (i = 0; order[i] != NULL; i++) {
        if (order[i] != dfa && order[i]->state >= 0) { //DFA_ALIVE()
            return FALSE;
        }
    }

    return TRUE;
}

boolean
dfa_none(guess_dfa *order[])
{
    int i;

    for (i = 0; order[i] != NULL; i++) {
        if (order[i]->state >= 0) { //DFA_ALIVE()
            return FALSE;
        }
    }

    return TRUE;
}

guess_dfa *
dfa_top(guess_dfa *order[])
{
    int i;
    guess_dfa *top = NULL;
    for (i = 0; order[i] != NULL; i++) {
        if (order[i]->state >= 0) { //DFA_ALIVE()
            if (top == NULL || order[i]->score > top->score)
                top = order[i];
        }
    }
    return top;
}

const char *
dfa_process(guess_dfa *order[], int c)
{
    int i;
    for (i = 0; order[i] != NULL; i++) {
        if (DFA_ALIVE_P(order[i])) {
            if (dfa_alone(order[i], order))
                return order[i]->name;
            DFA_NEXT_P(order[i], c);
        }
    }

    return NULL;
}
