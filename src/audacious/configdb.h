/*
 * configdb.h
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

#ifndef AUDACIOUS_CONFIGDB_H
#define AUDACIOUS_CONFIGDB_H

#include <glib.h>
#include <libmcs/mcs.h>
#include <audacious/api.h>

#define AUD_API_NAME ConfigDBAPI
#define AUD_API_SYMBOL configdb_api

#ifdef _AUDACIOUS_CORE

#include "api-local-begin.h"
#include "configdb-api.h"
#include "api-local-end.h"

void cfg_db_flush (void);

#else

#include <audacious/api-define-begin.h>
#include <audacious/configdb-api.h>
#include <audacious/api-define-end.h>

#include <audacious/api-alias-begin.h>
#include <audacious/configdb-api.h>
#include <audacious/api-alias-end.h>

#endif

#undef AUD_API_NAME
#undef AUD_API_SYMBOL

#endif

#ifdef AUD_API_DECLARE

#define AUD_API_NAME ConfigDBAPI
#define AUD_API_SYMBOL configdb_api

#include "api-define-begin.h"
#include "configdb-api.h"
#include "api-define-end.h"

#include "api-declare-begin.h"
#include "configdb-api.h"
#include "api-declare-end.h"

#undef AUD_API_NAME
#undef AUD_API_SYMBOL

#endif
