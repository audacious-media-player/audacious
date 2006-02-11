#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* static int intro_ms, outro_ms; */

typedef struct
{
	int RMS;
	float mix_in;
	float mix_out;
	float length;
} quantaudio_t;

/* id3 stuff is taken from the id3 GPL program */
typedef struct 
{
	char tag[3];
	char title[30];
	char artist[30];
	char album[30];
	char year[4];
	/* With ID3 v1.0, the comment is 30 chars long */
	/* With ID3 v1.1, if comment[28] == 0 then comment[29] == tracknum */
	char comment[30];
	unsigned char genre;
} id3_t;

int get_timing_comment (char *filename, quantaudio_t *qa);
int get_id3 (char *filename, id3_t *id3);
