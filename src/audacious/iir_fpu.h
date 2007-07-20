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
 *   $Id: iir_fpu.h,v 1.2 2005/11/01 15:59:20 lisanet Exp $$
 */
#ifndef IIR_FPU_H
#define IIR_FPU_H

#define sample_t double

/*
 * Normal FPU implementation data structures
 */
/* Coefficient history for the IIR filter */
typedef struct
{
    sample_t x[3]; /* x[n], x[n-1], x[n-2] */
    sample_t y[3]; /* y[n], y[n-1], y[n-2] */
    sample_t dummy1; // Word alignment
    sample_t dummy2;
}sXYData;

#endif
