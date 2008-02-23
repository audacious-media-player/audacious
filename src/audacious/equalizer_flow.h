#ifndef EQUALIZER_FLOW_H
#define EQUALIZER_FLOW_H

#include "flow.h"

void equalizer_flow(FlowContext *context);
void equalizer_flow_set_bands(gfloat pre, gfloat *bands);
void equalizer_flow_free();

#endif
