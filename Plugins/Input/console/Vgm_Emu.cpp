
// Game_Music_Emu 0.3.0. http://www.slack.net/~ant/

#include "Vgm_Emu.h"

#include <math.h>
#include <string.h>
#include "blargg_endian.h"

/* Copyright (C) 2003-2006 Shay Green. This module is free software; you
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

double const gain = 3.0; // FM emulators are internally quieter to avoid 16-bit overflow
double const rolloff = 0.990;
double const oversample_factor = 1.5;

Vgm_Emu::Vgm_Emu( bool os, double tempo )
{
	oversample = os;
	pos = NULL;
	data = NULL;
	uses_fm = false;
	vgm_rate = (long) (header_t::time_rate * tempo + 0.5);
	
	static equalizer_t const eq = { -14.0, 80 };
	set_equalizer( eq );
	psg.volume( 1.0 );
}

Vgm_Emu::~Vgm_Emu()
{
	unload();
}

void Vgm_Emu::unload()
{
	data = NULL;
	pos = NULL;
	set_track_ended( false );
	mem.clear();
}

blargg_err_t Vgm_Emu::set_sample_rate( long sample_rate )
{
	BLARGG_RETURN_ERR( blip_buf.set_sample_rate( sample_rate, 1000 / 30 ) );
	return Classic_Emu::set_sample_rate( sample_rate );
}

BOOST::uint8_t const* Vgm_Emu::gd3_data( int* size ) const
{
	if ( size )
		*size = 0;
	
	long gd3_offset = get_le32( header_.gd3_offset );
	if ( !gd3_offset )
		return NULL;
	
	gd3_offset -= 0x40 - offsetof (header_t,gd3_offset);
	if ( gd3_offset < 0 )
		return NULL;
	
	byte const* gd3 = data + gd3_offset;
	if ( data_end - gd3 < 16 || 0 != memcmp( gd3, "Gd3 ", 4 ) || get_le32( gd3 + 4 ) >= 0x200 )
		return NULL;
	
	long gd3_size = get_le32( gd3 + 8 );
	if ( data_end - gd3 < gd3_size - 12 )
		return NULL;
	
	if ( size )
		*size = data_end - gd3;
	return gd3;
}

void Vgm_Emu::update_eq( blip_eq_t const& eq )
{
	psg.treble_eq( eq );
	dac_synth.treble_eq( eq );
}

void Vgm_Emu::set_voice( int i, Blip_Buffer* c, Blip_Buffer* l, Blip_Buffer* r )
{
	if ( i < psg.osc_count )
		psg.osc_output( i, c, l, r );
}

const char** Vgm_Emu::voice_names() const
{
	static const char* fm_names [] = {
		"FM 1", "FM 2", "FM 3", "FM 4", "FM 5", "FM 6", "PCM", "PSG"
	};
	if ( uses_fm )
		return fm_names;
	
	static const char* psg_names [] = { "Square 1", "Square 2", "Square 3", "Noise" };
	return psg_names;
}

void Vgm_Emu::mute_voices( int mask )
{
	Classic_Emu::mute_voices( mask );
	dac_synth.output( &blip_buf );
	if ( uses_fm )
	{
		psg.output( (mask & 0x80) ? 0 : &blip_buf );
		if ( ym2612.enabled() )
		{
			dac_synth.volume( (mask & 0x40) ? 0.0 : 0.1115 / 256 * gain );
			ym2612.mute_voices( mask );
		}
		
		if ( ym2413.enabled() )
		{
			int m = mask & 0x3f;
			if ( mask & 0x20 )
				m |= 0x01e0; // channels 5-8
			if ( mask & 0x40 )
				m |= 0x3e00;
			ym2413.mute_voices( m );
		}
	}
}

blargg_err_t Vgm_Emu::load_( const header_t& h, void const* new_data, long new_size )
{
	header_ = h;
	
	// compatibility
	if ( 0 != memcmp( header_.tag, "Vgm ", 4 ) )
		return "Not a VGM file";
	check( get_le32( header_.version ) <= 0x150 );
	
	// psg rate
	long psg_rate = get_le32( header_.psg_rate );
	if ( !psg_rate )
		psg_rate = 3579545;
	blip_time_factor = (long) floor( (double) (1L << blip_time_bits) / vgm_rate * psg_rate + 0.5 );
	blip_buf.clock_rate( psg_rate );
	
	data = (byte*) new_data;
	data_end = data + new_size;
	
	// get loop
	loop_begin = data_end;
	if ( get_le32( header_.loop_offset ) )
		loop_begin = &data [get_le32( header_.loop_offset ) + offsetof (header_t,loop_offset) - 0x40];
	
	set_voice_count( psg.osc_count );
	set_track_count( 1 );
	
	BLARGG_RETURN_ERR( setup_fm() );
	
	// do after FM in case output buffer is changed
	BLARGG_RETURN_ERR( Classic_Emu::setup_buffer( psg_rate ) );
	
	return blargg_success;
}

blargg_err_t Vgm_Emu::setup_fm()
{
	long ym2612_rate = get_le32( header_.ym2612_rate );
	long ym2413_rate = get_le32( header_.ym2413_rate );
	if ( ym2413_rate && get_le32( header_.version ) < 0x110 )
		update_fm_rates( &ym2413_rate, &ym2612_rate );
	
	uses_fm = false;
	
	double fm_rate = blip_buf.sample_rate() * oversample_factor;
	
	if ( ym2612_rate )
	{
		uses_fm = true;
		if ( !oversample )
			fm_rate = ym2612_rate / 144.0;
		Dual_Resampler::setup( fm_rate / blip_buf.sample_rate(), rolloff, gain );
		BLARGG_RETURN_ERR( ym2612.set_rate( fm_rate, ym2612_rate ) );
		ym2612.enable( true );
		set_voice_count( 8 );
	}
	
	if ( !uses_fm && ym2413_rate )
	{
		uses_fm = true;
		if ( !oversample )
			fm_rate = ym2413_rate / 72.0;
		Dual_Resampler::setup( fm_rate / blip_buf.sample_rate(), rolloff, gain );
		int result = ym2413.set_rate( fm_rate, ym2413_rate );
		if ( result == 2 )
			return "YM2413 FM sound isn't supported";
		BLARGG_CHECK_ALLOC( !result );
		ym2413.enable( true );
		set_voice_count( 8 );
	}
	
	if ( uses_fm )
	{
		//dprintf( "fm_rate: %f\n", fm_rate );
		fm_time_factor = 2 + (long) floor( fm_rate * (1L << fm_time_bits) / vgm_rate + 0.5 );
		BLARGG_RETURN_ERR( Dual_Resampler::resize( blip_buf.length() * blip_buf.sample_rate() / 1000 ) );
		psg.volume( 0.135 * gain );
	}
	else
	{
		ym2612.enable( false );
		ym2413.enable( false );
		psg.volume( 1.0 );
	}
	
	return blargg_success;
}

blargg_err_t Vgm_Emu::load( Data_Reader& reader )
{
	header_t h;
	BLARGG_RETURN_ERR( reader.read( &h, sizeof h ) );
	return load( h, reader );
}

blargg_err_t Vgm_Emu::load( const header_t& h, Data_Reader& reader )
{
	unload();
	
	// allocate and read data
	long data_size = reader.remain();
	int const padding = 8;
	BLARGG_RETURN_ERR( mem.resize( data_size + padding ) );
	blargg_err_t err = reader.read( mem.begin(), data_size );
	if ( err ) {
		unload();
		return err;
	}
	memset( &mem [data_size], 0x66, padding ); // pad with end command
	
	return load_( h, mem.begin(), data_size );
}

void Vgm_Emu::start_track( int track )
{
	require( data ); // file must have been loaded
	
	Classic_Emu::start_track( track );
	psg.reset();
	
	dac_disabled = -1;
	pcm_data = data;
	pcm_pos = data;
	dac_amp = -1;
	vgm_time = 0;
	pos = data;
	if ( get_le32( header_.version ) >= 0x150 )
	{
		long data_offset = get_le32( header_.data_offset );
		check( data_offset );
		if ( data_offset )
			pos += data_offset + offsetof (header_t,data_offset) - 0x40;
	}
	
	if ( uses_fm )
	{
		if ( ym2413.enabled() )
			ym2413.reset();
		
		if ( ym2612.enabled() )
			ym2612.reset();
		
		fm_time_offset = 0;
		blip_buf.clear();
		Dual_Resampler::clear();
	}
}

long Vgm_Emu::run( int msec, bool* added_stereo )
{
	blip_time_t psg_end = run_commands( msec * vgm_rate / 1000 );
	*added_stereo = psg.end_frame( psg_end );
	return psg_end;
}

void Vgm_Emu::play( long count, sample_t* out )
{
	require( pos ); // track must have been started
	
	if ( uses_fm )
		Dual_Resampler::play( count, out, blip_buf );
	else
		Classic_Emu::play( count, out );
}

