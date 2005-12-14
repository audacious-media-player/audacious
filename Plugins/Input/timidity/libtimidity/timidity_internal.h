#ifndef TIMIDITY_INTERNAL_H
#define TIMIDITY_INTERNAL_H

#include "timidity.h"

#if  defined(__i386__) || defined(__ia64__) || defined(WIN32) || \
    (defined(__alpha__) || defined(__alpha)) || \
     defined(__arm__) || \
    (defined(__mips__) && defined(__MIPSEL__)) || \
     defined(__SYMBIAN32__) || \
     defined(__x86_64__) || \
     defined(__LITTLE_ENDIAN__)
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif
#undef BIG_ENDIAN
#else
#ifndef BIG_ENDIAN
#define BIG_ENDIAN
#endif
#undef LITTLE_ENDIAN
#endif

/* Instrument files are little-endian, MIDI files big-endian, so we
   need to do some conversions. */

#define XCHG_SHORT(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#ifdef __i486__
# define XCHG_LONG(x) \
     ({ sint32 __value; \
        asm ("bswap %1; movl %1,%0" : "=g" (__value) : "r" (x)); \
       __value; })
#else
# define XCHG_LONG(x) ((((x)&0xFF)<<24) | \
		      (((x)&0xFF00)<<8) | \
		      (((x)&0xFF0000)>>8) | \
		      (((x)>>24)&0xFF))
#endif

#ifdef LITTLE_ENDIAN
#define SWAPLE16(x) x
#define SWAPLE32(x) x
#define SWAPBE16(x) XCHG_SHORT(x)
#define SWAPBE32(x) XCHG_LONG(x)
#else
#define SWAPBE16(x) x
#define SWAPBE32(x) x
#define SWAPLE16(x) XCHG_SHORT(x)
#define SWAPLE32(x) XCHG_LONG(x)
#endif

#ifdef DEBUG
#define DEBUG_MSG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif


#define MID_VIBRATO_SAMPLE_INCREMENTS 32

/* Maximum polyphony. */
#define MID_MAX_VOICES	48

typedef sint16 sample_t;
typedef sint32 final_volume_t;

typedef struct _MidSample MidSample;
struct _MidSample
{
  sint32
    loop_start, loop_end, data_length,
    sample_rate, low_vel, high_vel, low_freq, high_freq, root_freq;
  sint32 envelope_rate[6], envelope_offset[6];
  float volume;
  sample_t *data;
    sint32
    tremolo_sweep_increment, tremolo_phase_increment,
    vibrato_sweep_increment, vibrato_control_ratio;
    uint8 tremolo_depth, vibrato_depth, modes;
    sint8 panning, note_to_use;
};

typedef struct _MidChannel MidChannel;
struct _MidChannel
{
  int bank, program, volume, sustain, panning, pitchbend, expression;
  int mono;	/* one note only on this channel -- not implemented yet */
  int pitchsens;
  /* chorus, reverb... Coming soon to a 300-MHz, eight-way superscalar
     processor near you */
  float pitchfactor; /* precomputed pitch bend factor to save some fdiv's */
};

typedef struct _MidVoice MidVoice;
struct _MidVoice
{
  uint8 status, channel, note, velocity;
  MidSample *sample;
  sint32
    orig_frequency, frequency,
    sample_offset, sample_increment,
    envelope_volume, envelope_target, envelope_increment,
    tremolo_sweep, tremolo_sweep_position,
    tremolo_phase, tremolo_phase_increment,
    vibrato_sweep, vibrato_sweep_position;

  final_volume_t left_mix, right_mix;

  float left_amp, right_amp, tremolo_volume;
    sint32 vibrato_sample_increment[MID_VIBRATO_SAMPLE_INCREMENTS];
  int
    vibrato_phase, vibrato_control_ratio, vibrato_control_counter,
    envelope_stage, control_counter, panning, panned;

};

typedef struct _MidInstrument MidInstrument;
struct _MidInstrument
{
  int samples;
  MidSample *sample;
};

typedef struct _MidToneBankElement MidToneBankElement;
struct _MidToneBankElement
{
  char *name;
  int note, amp, pan, strip_loop, strip_envelope, strip_tail;
};

typedef struct _MidToneBank MidToneBank;
struct _MidToneBank
{
  MidToneBankElement *tone;
  MidInstrument *instrument[128];
};

typedef struct _MidEvent MidEvent;
struct _MidEvent
{
  sint32 time;
  uint8 channel, type, a, b;
};

typedef struct _MidEventList MidEventList;
struct _MidEventList
{
  MidEvent event;
  void *next;
};

struct _MidSong
{
  int playing;
  sint32 rate;
  sint32 encoding;
  int bytes_per_sample;
  float master_volume;
  sint32 amplification;
  MidDLSPatches *patches;
  MidToneBank *tonebank[128];
  MidToneBank *drumset[128];
  MidInstrument *default_instrument;
  int default_program;
  void (*write) (void *dp, sint32 * lp, sint32 c);
  int buffer_size;
  sample_t *resample_buffer;
  sint32 *common_buffer;
  /* These would both fit into 32 bits, but they are often added in
     large multiples, so it's simpler to have two roomy ints */
  /* samples per MIDI delta-t */
  sint32 sample_increment;
  sint32 sample_correction;
  MidChannel channel[16];
  MidVoice voice[MID_MAX_VOICES];
  int voices;
  sint32 drumchannels;
  sint32 control_ratio;
  sint32 lost_notes;
  sint32 cut_notes;
  sint32 samples;
  MidEvent *events;
  MidEvent *current_event;
  MidEventList *evlist;
  sint32 current_sample;
  sint32 event_count;
  sint32 at;
  sint32 groomed_event_count;
  char *meta_data[8];
};

#endif /* TIMIDITY_INTERNAL_H */
