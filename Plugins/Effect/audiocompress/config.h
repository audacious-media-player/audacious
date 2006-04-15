/* config.h
** Default values for the configuration, and also a few random debug things
*/

#ifndef CONFIG_H
#define CONFIG_H

/*** Version information ***/
#define ACVERSION "1.5.2"

/*** Monitoring stuff ***/
//#define DEBUG			/* Show debugging information */
//#define STATS			/* Show statistics on stderr */

/*** Default configuration stuff ***/
#define ANTICLIP 0		/* Strict clipping protection */
#define TARGET 25000		/* Target level */

#define GAINMAX 32		/* The maximum amount to amplify by */
#define GAINSHIFT 10		/* How fine-grained the gain is */
#define GAINSMOOTH 8		/* How much inertia ramping has*/
#define BUCKETS 400		/* How long of a history to store */

#endif

