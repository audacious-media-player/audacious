#ifndef __DFA_H__
#define __DFA_H__

typedef int boolean;
#define TRUE  1
#define FALSE 0

/* workaround for that glib's g_convert can't convert
   properly from UCS-2BE/LE trailing after BOM. */
#define WITH_G_CONVERT 1
/* #undef WITH_G_CONVERT */

#ifdef WITH_G_CONVERT
#define UCS_2BE "UTF-16"
#define UCS_2LE "UTF-16"
#else
#define UCS_2BE "UCS_2BE"
#define UCS_2LE "UCS_2LE"
#endif

/* data types */
typedef struct guess_arc_rec
{
    unsigned int next;          /* next state */
    double score;               /* score */
} guess_arc;

typedef struct guess_dfa_rec
{
    signed char (*states)[256];
    guess_arc *arcs;
    int state;
    double score;
    char *name;
} guess_dfa;

/* macros */
#define DFA_INIT(st, ar, name)                       \
    { st, ar, 0, 1.0 ,name}

#define DFA_NEXT(dfa, ch)                               \
    do {                                                \
        int arc__;                                      \
        if (dfa.state >= 0) {                           \
            arc__ = dfa.states[dfa.state][ch];          \
            if (arc__ < 0) {                            \
                dfa.state = -1;                         \
            } else {                                    \
                dfa.state = dfa.arcs[arc__].next;       \
                dfa.score *= dfa.arcs[arc__].score;     \
            }                                           \
        }                                               \
    } while (0)

#define DFA_ALIVE(dfa)  (dfa.state >= 0)

#define DFA_NEXT_P(dfa, ch)                               \
    do {                                                \
        int arc__;                                      \
        if (dfa->state >= 0) {                         \
            arc__ = dfa->states[dfa->state][ch];       \
            if (arc__ < 0) {                            \
                dfa->state = -1;                       \
            } else {                                    \
                dfa->state = dfa->arcs[arc__].next;     \
                dfa->score *= dfa->arcs[arc__].score;     \
            }                                           \
        }                                               \
    } while (0)

#define DFA_ALIVE_P(dfa)  (dfa->state >= 0)

/* prototypes */
boolean dfa_alone(guess_dfa *dfa, guess_dfa *order[]);
boolean dfa_none(guess_dfa *order[]);
guess_dfa *dfa_top(guess_dfa *order[]);
const char *dfa_process(guess_dfa *order[], int c);

#endif
