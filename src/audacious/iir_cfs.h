/*
 *   PCM time-domain equalizer
 *
 *   Copyright (C) 2002-2005  Felipe Rivera <liebremx at users.sourceforge.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *   $Id: iir_cfs.h,v 1.1 2005/10/17 01:57:59 liebremx Exp $
 */
#ifndef IIR_CFS_H
#define IIR_CFS_H

#include <glib.h>

/* Coefficients entry */
typedef struct 
{
    float beta;
    float alpha; 
    float gamma;
    float dummy; /* Word alignment */
}sIIRCoefficients;

sIIRCoefficients* get_coeffs(gint *bands, gint sfreq, 
                              gboolean use_xmms_original_freqs);
void calc_coeffs();

#endif
