
// Stubs to disable tagged data reflection functions

// Game_Music_Emu 0.2.4. Copyright (C) 2005 Shay Green. GNU LGPL license.

#ifndef TAGGED_DATA_H
#define TAGGED_DATA_H

typedef long data_tag_t;

class Tagged_Data {
public:
	Tagged_Data( Tagged_Data& parent, data_tag_t ) { }
	int reading() const { return false; }
	int not_found() const { return true; }
	int reflect_int8( data_tag_t, int n ) { return n; }
	int reflect_int16( data_tag_t, int n ) { return n; }
	long reflect_int32( data_tag_t, long n ) { return n; }
};

template<class T>
inline void reflect_int8( Tagged_Data&, data_tag_t, T* ) { }

template<class T>
inline void reflect_int16( Tagged_Data&, data_tag_t, T* ) { }

template<class T>
inline void reflect_int32( Tagged_Data&, data_tag_t, T* ) { }

#endif

