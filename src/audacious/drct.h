/*
 * drct.h
 * Copyright 2010 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#ifndef AUDACIOUS_DRCT_H
#define AUDACIOUS_DRCT_H

#include <audacious/api.h>
#include <libaudcore/core.h>
#include <libaudcore/index.h>

#define AUD_API_NAME DRCTAPI
#define AUD_API_SYMBOL drct_api

#ifdef _AUDACIOUS_CORE

#include "api-local-begin.h"
#include "drct-api.h"
#include "api-local-end.h"

#else

#include <audacious/api-define-begin.h>
#include <audacious/drct-api.h>
#include <audacious/api-define-end.h>

#include <audacious/api-alias-begin.h>
#include <audacious/drct-api.h>
#include <audacious/api-alias-end.h>

#endif

#undef AUD_API_NAME
#undef AUD_API_SYMBOL

#endif

#ifdef AUD_API_DECLARE

#define AUD_API_NAME DRCTAPI
#define AUD_API_SYMBOL drct_api

#include "api-define-begin.h"
#include "drct-api.h"
#include "api-define-end.h"

#include "api-declare-begin.h"
#include "drct-api.h"
#include "api-declare-end.h"

#undef AUD_API_NAME
#undef AUD_API_SYMBOL

#endif
