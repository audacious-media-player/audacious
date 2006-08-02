
// Game_Music_Emu 0.3.0. http://www.slack.net/~ant/

#include "Fir_Resampler.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Copyright (C) 2004-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include BLARGG_SOURCE_BEGIN

// to do: fix problems with rolloff < 0.99 or so, and rolloff == 1.0, and related problems

// Sinc impulse genertor

const bool show_impulse = 0;

static const double pi = 3.1415926535897932384626433832795029L;

class Dsf {
	double rolloff;
	double factor;
public:
	Dsf( double r ) : rolloff( r )
	{
		factor = 1.0;
		//if ( rolloff < 1.0 )
		//  factor = 1.0 / (*this)( 0 );
	}
	
	double operator () ( double angle ) const
	{
		double const n_harm = 256;
		angle /= n_harm;
		double pow_a_n = pow( rolloff, n_harm );
		//double rescale = 1.0 / n_harm;
		
		double num = 1.0 - rolloff * cos( angle ) -
				pow_a_n * cos( n_harm * angle ) +
				pow_a_n * rolloff * cos( (n_harm - 1) * angle );
		double den = 1 + rolloff * (rolloff - 2 * cos( angle ));
		
		return (num / den - 1) / n_harm * factor;
	}
};

template<class Sinc>
void gen_sinc( int width, double offset, double spacing, int count, double scale, short* p,
		const Sinc& sinc )
{
	double range = pi * (width / 2);
	double step = pi * spacing;
	double a = -step * (count / 2 - 1);
	a -= offset * step;
	
	while ( count-- )
	{
		double w = a / range;
		double y = 0.0;
		if ( fabs( w ) < 1.0 )
		{
			double window = cos( pi * w ) * 0.5 + 0.5;
			y = sinc( a ) * window;
		}
		
		*p++ = (short) (y * scale);
		a += step;
	}
}

static double plain_sinc( double a )
{
	return fabs( a ) < 0.00001 ? 1.0 : sin( a ) / a;
}

// Fir_Resampler

Fir_Resampler_::Fir_Resampler_( int width, sample_t* impulses_ ) :
	width_( width ),
	write_offset( width * stereo - stereo ),
	impulses( impulses_ )
{
	write_pos = NULL;
	res = 1;
	imp = 0;
	skip_bits = 0;
	step = stereo;
	ratio_ = 1.0;
}

Fir_Resampler_::~Fir_Resampler_()
{
}

void Fir_Resampler_::clear()
{
	imp = 0;
	if ( buf.size() )
	{
		write_pos = &buf [write_offset];
		memset( buf.begin(), 0, write_offset * sizeof buf [0] );
	}
}

blargg_err_t Fir_Resampler_::buffer_size( int new_size )
{
	BLARGG_RETURN_ERR( buf.resize( new_size + write_offset ) );
	clear();
	return blargg_success;
}
	
double Fir_Resampler_::time_ratio( double new_factor, double rolloff, double gain )
{
	ratio_ = new_factor;
	
	double fstep = 0.0;
	{
		double least_error = 2;
		double pos = 0;
		res = -1;
		for ( int r = 1; r <= max_res; r++ )
		{
			pos += ratio_;
			double nearest = floor( pos + 0.5 );
			double error = fabs( pos - nearest );
			if ( error < least_error )
			{
				res = r;
				fstep = nearest / res;
				least_error = error;
			}
		}
	}
	
	skip_bits = 0;
	
	step = stereo * (int) floor( fstep );
	
	ratio_ = fstep;
	fstep = fmod( fstep, 1.0 );
	
	double filter = (ratio_ < 1.0) ? 1.0 : 1.0 / ratio_;
	double pos = 0.0;
	input_per_cycle = 0;
	Dsf dsf( rolloff );
	for ( int i = 0; i < res; i++ )
	{
		if ( show_impulse )
			printf( "pos = %f\n", pos );
		
		gen_sinc( int (width_ * filter + 1) & ~1, pos, filter, (int) width_,
				double (0x7fff * gain * filter), impulses + i * width_, dsf );
		
		if ( show_impulse )
		{
			for ( int j = 0; j < width_; j++ )
				printf( "%d ", (int) impulses [i * width_ + j] );
			printf( "\n" );
		}
		
		pos += fstep;
		input_per_cycle += step;
		if ( pos >= 0.9999999 )
		{
			pos -= 1.0;
			skip_bits |= 1 << i;
			input_per_cycle++;
		}
	}
	
	if ( show_impulse )
	{
		printf( "skip = %8lX\n", (long) skip_bits );
		printf( "step = %d\n", step );
	}
	
	clear();
	
	return ratio_;
}

int Fir_Resampler_::input_needed( long output_count ) const
{
	long input_count = 0;
	
	unsigned long skip = skip_bits >> imp;
	int remain = res - imp;
	while ( (output_count -= 2) > 0 )
	{
		input_count += step + (skip & 1) * stereo;
		skip >>= 1;
		if ( !--remain )
		{
			skip = skip_bits;
			remain = res;
		}
		output_count -= 2;
	}
	
	long input_extra = input_count - (write_pos - &buf [(width_ - 1) * stereo]);
	if ( input_extra < 0 )
		input_extra = 0;
	return input_extra;
}

int Fir_Resampler_::avail_( long input_count ) const
{
	int cycle_count = input_count / input_per_cycle;
	int output_count = cycle_count * res * stereo;
	input_count -= cycle_count * input_per_cycle;
	
	unsigned long skip = skip_bits >> imp;
	int remain = res - imp;
	while ( input_count >= 0 )
	{
		input_count -= step + (skip & 1) * stereo;
		skip >>= 1;
		if ( !--remain )
		{
			skip = skip_bits;
			remain = res;
		}
		output_count += 2;
	}
	return output_count;
}

int Fir_Resampler_::skip_input( long count )
{
	int remain = write_pos - buf.begin();
	int avail = remain - width_ * stereo;
	if ( avail < 0 ) avail = 0; // inserted
	if ( count > avail )
		count = avail;
	
	remain -= count;
	write_pos = &buf [remain];
	memmove( buf.begin(), &buf [count], remain * sizeof buf [0] );
	
	return count;
}

