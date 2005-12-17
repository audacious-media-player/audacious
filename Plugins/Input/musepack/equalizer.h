#ifndef IIR_H
#define IIR_H

#define EQ_MAX_BANDS 10
#define EQ_CHANNELS 2

int iir(char * d, gint length);
void init_iir(int on , float preamp_ctrl, float*  eq_ctrl);


#endif                          /* #define IIR_H */
