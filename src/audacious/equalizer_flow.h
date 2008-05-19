#ifndef AUDACIOUS_EQUALIZER_FLOW_H
#define AUDACIOUS_EQUALIZER_FLOW_H

#include "flow.h"

#define EQUALIZER_MAX_GAIN 12.0

void equalizer_flow(FlowContext *context);
void equalizer_flow_set_bands(gfloat pre, gfloat *bands);
void equalizer_flow_free();

#endif /* AUDACIOUS_EQUALIZER_FLOW_H */
