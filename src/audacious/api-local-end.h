/*
 * api-local-end.h
 * Copyright 2010 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#if ! defined AUD_API_NAME || ! defined AUD_API_SYMBOL || ! defined AUD_API_LOCAL_H
#error Bad usage of api-local-end.h
#endif

#undef AUD_API_LOCAL_H

#undef AUD_FUNC0
#undef AUD_FUNC1
#undef AUD_FUNC2
#undef AUD_FUNC3
#undef AUD_FUNC4
#undef AUD_FUNC5
#undef AUD_FUNC6
#undef AUD_FUNC7
#undef AUD_FUNC8

#undef AUD_VFUNC0
#undef AUD_VFUNC1
#undef AUD_VFUNC2
#undef AUD_VFUNC3
#undef AUD_VFUNC4
#undef AUD_VFUNC5
#undef AUD_VFUNC6
#undef AUD_VFUNC7
#undef AUD_VFUNC8
