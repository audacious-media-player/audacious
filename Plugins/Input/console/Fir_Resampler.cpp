
// Game_Music_Emu 0.2.4. http://www.slack.net/~ant/libs/

#include "Fir_Resampler.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Copyright (C) 2003-2005 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details. You should have received a copy of the GNU Lesser General
Public License along with this module; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#include BLARGG_SOURCE_BEGIN

static const double pi = 3.1415926535897932384626433832795029L;

const bool show_impulse = 0;

class Dsf {
	double rolloff;
	double factor;
public:
	Dsf( double r ) : rolloff( r ) {
		factor = 1.0;
		//if ( rolloff < 1.0 )
		//  factor = 1.0 / (*this)( 0 );
	}
	
	double operator () ( double angle ) const
	{
		double const n_harm = 256;
		angle /= n_harm;
		double pow_a_n = pow( rolloff, n_harm );
		double rescale = 1.0 / n_harm;
		
		double num = 1.0 - rolloff * cos( angle ) -
				pow_a_n * cos( n_harm * angle ) +
				pow_a_n * rolloff * cos( (n_harm - 1) * angle );
		double den = 1 + rolloff * (rolloff - 2 * cos( angle ));
		
		return (num / den - 1) / n_harm * factor;
	}
};

template<class T,class Sinc>
void gen_sinc( int width, double offset, double spacing, int count, double scale, T* p,
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
			y = cos( pi * w ) * 0.5 + 0.5;
			y *= sinc( a );
		}
		
		*p++ = (T) (y * scale);
		a += step;
	}
}

static double plain_sinc( double a ) {
	return fabs( a ) < 0.00001 ? 1.0 : sin( a ) / a;
}

Fir_Resampler::Fir_Resampler()
{
	res = 1;
	skip_bits = 0;
	step = 2;
	buf = NULL;
	write_pos = NULL;
	buf_size = 0;
}

Fir_Resampler::~Fir_Resampler() {
	free( buf );
}

void Fir_Resampler::clear()
{
	imp = 0;
	if ( buf ) {
		write_pos = buf + latency;
		memset( buf, 0, (write_pos - buf) * sizeof *buf );
	}
}

blargg_err_t Fir_Resampler::buffer_size( int new_size )
{
	new_size += latency;
	void* new_buf = realloc( buf, new_size * sizeof *buf );
	if ( !new_buf )
		return "Out of memory";
	buf = (sample_t*) new_buf;
	buf_size = new_size;
	clear();
	return blargg_success;
}
	
double Fir_Resampler::time_ratio( double ratio, double rolloff, double volume )
{
	this->ratio = ratio;
	
	double fstep = 0.0;
	{
		double least_error = 2;
		double pos = 0;
		res = -1;
		for ( int r = 1; r <= max_res; r++ ) {
			pos += ratio;
			double nearest = floor( pos + 0.5 );
			double error = fabs( pos - nearest );
			if ( error < least_error ) {
				res = r;
				fstep = nearest / res;
				least_error = error;
			}
		}
	}
	
	skip_bits = 0;
	
	step = 2 * (int) floor( fstep );
	
	ratio = fstep;
	fstep = fmod( fstep, 1.0 );
	
	double filter = (ratio < 1.0) ? 1.0 : 1.0 / ratio;
	double pos = 0.0;
	Dsf dsf( rolloff );
	for ( int i = 0; i < res; i++ )
	{
		if ( show_impulse )
			printf( "pos = %f\n", pos );
		
		gen_sinc( int (width * filter + 1) & ~1, pos, filter, (int) width,
				double (0x7fff * volume * filter), impulses [i], dsf );
		
		if ( show_impulse ) {
			for ( int j = 0; j < width; j++ )
				printf( "%d ", (int) impulses [i] [j] );
			printf( "\n" );
		}
		
		pos += fstep;
		if ( pos >= 0.9999999 ) {
			pos -= 1.0;
			skip_bits |= 1 << i;
		}
	}
	
	if ( show_impulse ) {
		printf( "skip = %X\n", skip_bits );
		printf( "step = %d\n", step );
	}
	
	clear();
	
	return ratio;
}

#include BLARGG_ENABLE_OPTIMIZER

int Fir_Resampler::read( sample_t* out_begin, int count )
{
	sample_t* out = out_begin;
	const sample_t* in = buf;
	sample_t* end_pos = write_pos;
	unsigned long skip = skip_bits >> imp;
	sample_t const* imp = impulses [this->imp];
	int remain = res - this->imp;
	int const step = this->step;
	
	count = (count >> 1) + 1;
	
	// to do: optimize loop to use a single counter rather than 'in' and 'count'
	
	if ( end_pos - in >= width * 2 )
	{
		end_pos -= width * 2;
		do
		{
			count--;
			
			// accumulate in extended precision
			long l = 0;
			long r = 0;
			
			const sample_t* i = in;
			if ( !count )
				break;
			
			for ( int n = width / 2; n--; )
			{
				int pt0 = imp [0];
				int pt1 = imp [1];
				imp += 2;
				l += (pt0 * i [0]) + (pt1 * i [2]);
				r += (pt0 * i [1]) + (pt1 * i [3]);
				i += 4;
			}
			
			remain--;
			
			l >>= 15;
			r >>= 15;
			
			in += step + ((skip * 2) & 2);
			skip >>= 1;
			
			if ( !remain ) {
				imp = impulses [0];
				skip = skip_bits;
				remain = res;
			}
			
			out [0] = l;
			out [1] = r;
			out += 2;
		}
		while ( in <= end_pos );
	}
	
	this->imp = res - remain;
	
	int left = write_pos - in;
	write_pos = buf + left;
	assert( unsigned (write_pos - buf) <= buf_size );
	memmove( buf, in, left * sizeof *in );
	
	return out - out_begin;
}

int Fir_Resampler::skip_input( int count )
{
	int remain = write_pos - buf;
	int avail = remain - width * 2;
	if ( count > avail )
		count = avail;
	
	remain -= count;
	write_pos = buf + remain;
	assert( unsigned (write_pos - buf) <= buf_size );
	memmove( buf, buf + count, remain * sizeof *buf );
	
	return count;
}

/*
int Fir_Resampler::skip( int count )
{
	count = int (count * ratio) & ~1;
	count = skip_input( count );
	return int (count / ratio) & ~1;
}
*/
