/*
 * tinylock.h
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

#ifndef LIBAUDCORE_TINYLOCK_H
#define LIBAUDCORE_TINYLOCK_H

/* TinyLock is an extremely low-overhead lock object (in terms of speed and
 * memory usage).  It makes no guarantees of fair scheduling, however. */

typedef char TinyLock;

void tiny_lock (TinyLock * lock);
void tiny_unlock (TinyLock * lock);

typedef unsigned short TinyRWLock;

void tiny_lock_read (TinyRWLock * lock);
void tiny_unlock_read (TinyRWLock * lock);
void tiny_lock_write (TinyRWLock * lock);
void tiny_unlock_write (TinyRWLock * lock);

#endif /* LIBAUDCORE_TINYLOCK_H */
