/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 *
 * Contributor(s):
 *      M. Bakker     mbakker at nero.com
 *
 * Apple iTunes META data
 */

#include "mp4common.h"

MP4MeanAtom::MP4MeanAtom()
    : MP4Atom("mean")
{
	AddVersionAndFlags(); /* 0, 1 */

    AddProperty(
        new MP4BytesProperty("metadata")); /* 2 */
}

void MP4MeanAtom::Read() 
{
	// calculate size of the metadata from the atom size
	((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 4);

	MP4Atom::Read();
}

MP4NameAtom::MP4NameAtom()
    : MP4Atom("name")
{
	AddVersionAndFlags(); /* 0, 1 */

    AddProperty(
        new MP4BytesProperty("metadata")); /* 2 */
}

void MP4NameAtom::Read() 
{
	// calculate size of the metadata from the atom size
	((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 4);

	MP4Atom::Read();
}

MP4DataAtom::MP4DataAtom()
    : MP4Atom("data")
{
	AddVersionAndFlags(); /* 0, 1 */
    AddReserved("reserved2", 4); /* 2 */

    AddProperty(
        new MP4BytesProperty("metadata")); /* 3 */
}

void MP4DataAtom::Read() 
{
	// calculate size of the metadata from the atom size
	((MP4BytesProperty*)m_pProperties[3])->SetValueSize(m_size - 8);

	MP4Atom::Read();
}

