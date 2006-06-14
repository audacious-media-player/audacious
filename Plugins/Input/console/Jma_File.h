/*
 * Copyright (C) 2006 William Pitcock, et al.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software module and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 *     The above copyright notice and this permission notice shall be
 *     included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// JMA file access

#ifndef JMA_FILE_H
#define JMA_FILE_H

#include "abstract_file.h"

class Jma_File_Reader : public File_Reader {
private:
	unsigned char *buf_;
	unsigned char *bufptr_;
	long size_;
	JMA::jma_open *jma_;
public:
	Jma_File_Reader();
	~Jma_File_Reader();
	
	// JMA::jma_open is the container class, oh how fucked up
	error_t open( const char *, const char* );
	
	long size() const;
	long read_avail( unsigned char*, long );
	
	long tell() const;
	error_t seek( long );
	
	void close();
};

#endif

