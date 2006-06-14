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

#include "libjma/jma.h"
#include "Jma_File.h"

#include <vector>

// Jma_File_Reader

Jma_File_Reader::Jma_File_Reader()
{
}

Jma_File_Reader::~Jma_File_Reader()
{
	close();
}

Jma_File_Reader::error_t Jma_File_Reader::open( const char *jmaspec, const char* file )
{
	jma_ = new JMA::jma_open(jmaspec);
	std::vector<JMA::jma_public_file_info> file_info = jma_->get_files_info();
	bool found = false;
	int size_;
	std::string m = file;

	for (std::vector<JMA::jma_public_file_info>::iterator i = file_info.begin();
		i != file_info.end(); i++)
	{
		if (m == i->name)
		{
			found = true;
			size_ = i->size;
			break;
		}
	}

	if (found == false)
		return "JMA container entry not found";

	buf_ = new unsigned char[size_];

	jma_->extract_file(m, buf_);

	bufptr_ = buf_;

	return NULL;
}

long Jma_File_Reader::size() const
{
	return size_;
}

long Jma_File_Reader::read_avail( unsigned char* p, long s )
{
	p = new unsigned char[s];

	for (long i = 0; i < s; i++)
		p[i] = *bufptr_++;

	return s;
}

long Jma_File_Reader::tell() const
{
	long i = (char *) bufptr_ - (char *) buf_;

	return i;
}

Jma_File_Reader::error_t Jma_File_Reader::seek( long n )
{
	if (n > size_)
		return "Error seeking in file";

	/* move the iter to this position */
	bufptr_ = buf_ + n;

	return NULL;
}

void Jma_File_Reader::close()
{
	delete buf_;
	delete jma_;

	size_ = 0;
	bufptr_ = NULL;
	buf_ = NULL;
}
