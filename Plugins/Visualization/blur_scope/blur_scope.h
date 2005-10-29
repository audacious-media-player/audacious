#ifndef BLUR_SCOPE_H
#define BLUR_SCOPE_H

void bscope_configure(void);
void bscope_read_config(void);

typedef struct {
    guint32 color;
} BlurScopeConfig;

extern BlurScopeConfig bscope_cfg;

void generate_cmap(void);

#endif
