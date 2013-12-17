/*
 * chardet.c
 * Copyright 2013 John Lindgren
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

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "main.h"
#include "misc.h"

static void chardet_update (void)
{
    char * region = get_str (NULL, "chardet_detector");
    char * fallbacks = get_str (NULL, "chardet_fallback");

    Index * list = str_list_to_index (fallbacks, ", ");
    str_set_charsets (region[0] ? region : NULL, list);

    str_unref (region);
    str_unref (fallbacks);
}

void chardet_init (void)
{
    chardet_update ();

    hook_associate ("set chardet_detector", (HookFunction) chardet_update, NULL);
    hook_associate ("set chardet_fallback", (HookFunction) chardet_update, NULL);
}

void chardet_cleanup (void)
{
    hook_dissociate ("set chardet_detector", (HookFunction) chardet_update);
    hook_dissociate ("set chardet_fallback", (HookFunction) chardet_update);

    str_set_charsets (NULL, NULL);
}
