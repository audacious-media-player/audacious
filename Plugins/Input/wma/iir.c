/*
 *   PCM time-domain equalizer
 *
 *   Copyright (C) 2002  Felipe Rivera <liebremx at users sourceforge net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   $Id: iir.c,v 1.5 2003/09/10 21:53:08 liebremx Exp $
 */

/* IIR filter coefficients */
#include "iir.h"
#include <math.h>
#include <strings.h>

/* History for two filters */
static sXYData data_history[EQ_MAX_BANDS][EQ_CHANNELS] __attribute__((aligned));
static sXYData data_history2[EQ_MAX_BANDS][EQ_CHANNELS] __attribute__((aligned));

/* Coefficients */
static sIIRCoefficients *iir_cf;

/* Config EQ */
static int band_num = 10;       /* Set num band: 10 - default in XMMS */ 
static int extra_filtering = 1; /* Set extra filtering: 0 - OFF, 1 - ON */

/* BETA, ALPHA, GAMMA */
static sIIRCoefficients iir_cforiginal10[] __attribute__((aligned)) = {
    { (9.9421504945e-01), (2.8924752745e-03), (1.9941421835e+00) },    /*    60.0 Hz */
    { (9.8335039428e-01), (8.3248028618e-03), (1.9827686547e+00) },    /*   170.0 Hz */
    { (9.6958094144e-01), (1.5209529281e-02), (1.9676601546e+00) },    /*   310.0 Hz */
    { (9.4163923306e-01), (2.9180383468e-02), (1.9345490229e+00) },    /*   600.0 Hz */
    { (9.0450844499e-01), (4.7745777504e-02), (1.8852109613e+00) },    /*  1000.0 Hz */
    { (7.3940088234e-01), (1.3029955883e-01), (1.5829158753e+00) },    /*  3000.0 Hz */
    { (5.4697667908e-01), (2.2651166046e-01), (1.0153238114e+00) },    /*  6000.0 Hz */
    { (3.1023210589e-01), (3.4488394706e-01), (-1.8142472036e-01) },    /* 12000.0 Hz */
    { (2.6718639778e-01), (3.6640680111e-01), (-5.2117742267e-01) },    /* 14000.0 Hz */
    { (2.4201241845e-01), (3.7899379077e-01), (-8.0847117831e-01) },    /* 16000.0 Hz */
};

/* Init the filter */
void init_iir()
{
	iir_cf = iir_cforiginal10;

	/* Zero the history arrays */
	bzero(data_history, sizeof(sXYData) * EQ_MAX_BANDS * EQ_CHANNELS);
	bzero(data_history2, sizeof(sXYData) * EQ_MAX_BANDS * EQ_CHANNELS);
}

__inline__ int iir(gpointer * d, gint length)
{
	gint16 *data = (gint16 *) * d;
	/* Indexes for the history arrays
	 * These have to be kept between calls to this function
	 * hence they are static */
	static gint i = 0, j = 2, k = 1;	

	gint index, band, channel;
	gint tempgint, halflength;
	float out[EQ_CHANNELS], pcm[EQ_CHANNELS];

	/**
	 * IIR filter equation is
	 * y[n] = 2 * (alpha*(x[n]-x[n-2]) + gamma*y[n-1] - beta*y[n-2])
	 *
	 * NOTE: The 2 factor was introduced in the coefficients to save
	 * 			a multiplication
	 *
	 * This algorithm cascades two filters to get nice filtering
	 * at the expense of extra CPU cycles
	 */
	/* 16bit, 2 bytes per sample, so divide by two the length of
	 * the buffer (length is in bytes)
	 */
	halflength = (length >> 1);
	for (index = 0; index < halflength; index+=2)
	{
		/* For each channel */
		for (channel = 0; channel < EQ_CHANNELS; channel++)
		{
			/* No need to scale when processing the PCM with the filter */
			pcm[channel] = data[index+channel];
			/* Preamp gain */
			pcm[channel] *= preamp[channel];

	
			out[channel] = 0;
			/* For each band */
			for (band = 0; band < band_num; band++)
			{
				/* Store Xi(n) */
				data_history[band][channel].x[i] = pcm[channel];
				/* Calculate and store Yi(n) */
				data_history[band][channel].y[i] = 
               		   (
               	/* 		= alpha * [x(n)-x(n-2)] */
						iir_cf[band].alpha * ( data_history[band][channel].x[i]
               			-  data_history[band][channel].x[k])
               	/* 		+ gamma * y(n-1) */
               			+ iir_cf[band].gamma * data_history[band][channel].y[j]
               	/* 		- beta * y(n-2) */
               			- iir_cf[band].beta * data_history[band][channel].y[k]
						);
				/* 
				 * The multiplication by 2.0 was 'moved' into the coefficients to save
				 * CPU cycles here */
				 /* Apply the gain  */
				out[channel] +=  data_history[band][channel].y[i]*gain[band][channel]; // * 2.0;
			} /* For each band */

                        if (extra_filtering)
                        {
				/* Filter the sample again */
				for (band = 0; band < band_num; band++)
				{
					/* Store Xi(n) */
					data_history2[band][channel].x[i] = out[channel];
					/* Calculate and store Yi(n) */
					data_history2[band][channel].y[i] = 
            	   		   (
	               	/* y(n) = alpha * [x(n)-x(n-2)] */
							iir_cf[band].alpha * (data_history2[band][channel].x[i]
            	   			-  data_history2[band][channel].x[k])
               		/* 	    + gamma * y(n-1) */
	               			+ iir_cf[band].gamma * data_history2[band][channel].y[j]
    	           	/* 		- beta * y(n-2) */
        	       			- iir_cf[band].beta * data_history2[band][channel].y[k]
							);
					/* Apply the gain */
					out[channel] +=  data_history2[band][channel].y[i]*gain[band][channel];
				} /* For each band */
                        }

			/* Volume stuff
			   Scale down original PCM sample and add it to the filters 
			   output. This substitutes the multiplication by 0.25
			 */

			out[channel] += (data[index+channel]>>2);

			/* Round and convert to integer */
			tempgint = (int)lroundf(out[channel]);

			/* Limit the output */
			if (tempgint < -32768)
				data[index+channel] = -32768;
			else if (tempgint > 32767)
				data[index+channel] = 32767;
			else 
				data[index+channel] = tempgint;
		} /* For each channel */

		i++; j++; k++;
		
		/* Wrap around the indexes */
		if (i == 3) i = 0;
		else if (j == 3) j = 0;
		else k = 0;

		
	}/* For each pair of samples */

	return length;
}
