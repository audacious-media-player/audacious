/*
 * threads.cc
 * Copyright 2020 John Lindgren
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

#include "threads.h"
#include "tinylock.h"

namespace aud
{

EXPORT void spinlock::lock() { tiny_lock(&m_lock); }
EXPORT void spinlock::unlock() { tiny_unlock(&m_lock); }

EXPORT void spinlock_rw::lock_r() { tiny_lock_read(&m_lock); }
EXPORT void spinlock_rw::unlock_r() { tiny_unlock_read(&m_lock); }
EXPORT void spinlock_rw::lock_w() { tiny_lock_write(&m_lock); }
EXPORT void spinlock_rw::unlock_w() { tiny_unlock_write(&m_lock); }

} // namespace aud
