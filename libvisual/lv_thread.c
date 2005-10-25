/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "lv_log.h"
#include "lv_thread.h"

static int __lv_thread_enabled = TRUE;

/* FIXME add more threading backends here:
 * glib			(could proof to be a nice fallback when needed)
 * native windows	(says nuff)
 * what is used on Mac os X ?
 */


/**
 * @defgroup VisThread VisThread
 * @{
 */

/**
 * Enable or disable threading support. This can be used to disallow threads, which might be needed in some environments.
 * 
 * @see visual_thread_is_enabled
 *
 * @param enabled TRUE to enable threads, FALSE to disable threads.
 */
void visual_thread_enable (int enabled)
{
	__lv_thread_enabled = enabled > 0 ? TRUE : FALSE;
}

/**
 * Request if threads are enabled or not. This function should not be confused with visual_thread_is_supported().
 *
 * @return TRUE if enabled, FALSE if disabled.
 */
int visual_thread_is_enabled (void)
{
	return __lv_thread_enabled;
}

/**
 * Is used to check if threading is supported. When threading is used this should always
 * be checked, it's possible to disable threads from within the code, so no #ifdefs should
 * be used.
 *
 * @return TRUE if threading is supported or FALSE if not.
 */
int visual_thread_is_supported ()
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	return TRUE;
#else /* !VISUAL_THREAD_MODEL_POSIX */
	return FALSE;
#endif
#else
	return FALSE;
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * Creates a new VisThread that is used in threading.
 *
 * @param func The threading function.
 * @param data The private data that is send to the threading function.
 * @param joinable Flag that contains whatever the thread can be joined or not.
 *
 * @return A newly allocated VisThread, or NULL on failure.
 */
VisThread *visual_thread_create (VisThreadFunc func, void *data, int joinable)
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	VisThread *thread;
	pthread_attr_t attr;
	int res;

	if (visual_thread_is_enabled () == FALSE)
		return NULL;

	thread = visual_mem_new0 (VisThread, 1);

	pthread_attr_init(&attr);

	if (joinable == TRUE)
		pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
	else
		pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

	res = pthread_create (&thread->thread, &attr, func, data);

	pthread_attr_destroy (&attr);

	if (res != 0) {
		visual_log (VISUAL_LOG_CRITICAL, "Error while creating thread");

		visual_mem_free (thread);

		return NULL;
	}

	return thread;
#else /* !VISUAL_THREAD_MODEL_POSIX */
	return NULL;
#endif 
#else
	return NULL;
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * After a VisThread is not needed anylonger it needs to be freed using this function.
 *
 * @param thread The VisThread that needs to be freed.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_THREAD_NULL, -VISUAL_ERROR_THREAD_NO_THREADING or
 *	error values returned by visual_mem_free on failure.
 */
int visual_thread_free (VisThread *thread)
{
#ifdef VISUAL_HAVE_THREADS
	visual_log_return_val_if_fail (thread != NULL, -VISUAL_ERROR_THREAD_NULL);

	return visual_mem_free (thread);
#else
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * Joins a VisThread with another.
 *
 * @param thread The VisThread that is about to be joined.
 *
 * @return Possible result that was passed to visual_thread_exit as retval or
 *	NULL.
 */
void *visual_thread_join (VisThread *thread)
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	void *result;

	if (thread == NULL)
		return NULL;

	if (pthread_join (thread->thread, &result) < 0) {
		visual_log (VISUAL_LOG_CRITICAL, "Error while joining thread");

		return NULL;
	}

	return result;
#else /* !VISUAL_THREAD_MODEL_POSIX */
	return NULL;
#endif
#else
	return NULL;
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * Exits a VisThread, this will terminate the thread.
 *
 * @param retval The return value that is catched by the visual_thread_join function.
 */
void visual_thread_exit (void *retval)
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	pthread_exit (retval);
#else /* !VISUAL_THREAD_MODEL_POSIX */

#endif
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * Yield the current VisThread so another gets time.
 */
void visual_thread_yield ()
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	sched_yield ();
#else /* !VISUAL_THREAD_MODEL_POSIX */
	
#endif
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * Creates a new VisMutex that is used to do thread locking so data can be synchronized.
 *
 * @return A newly allocated VisMutex that can be used with the visual_mutex_lock and
 *	visual_mutex_unlock functions or NULL on failure.
 */
VisMutex *visual_mutex_new ()
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	VisMutex *mutex;

	if (visual_thread_is_enabled () == FALSE)
		return NULL;

	mutex = visual_mem_new0 (VisMutex, 1);

	pthread_mutex_init (&mutex->mutex, NULL);

	return mutex;
#else /* !VISUAL_THREAD_MODEL_POSIX */
	return NULL;
#endif
#else
	return NULL;
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * A VisMutex is allocated to have more flexibility with the actual thread backend. Thus they need
 * to be freed as well.
 *
 * @param mutex Pointer to the VisMutex that needs to be freed.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_MUTEX_NULL or -VISUAL_ERROR_THREAD_NO_THREADING on failure.
 */
int visual_mutex_free (VisMutex *mutex)
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	visual_log_return_val_if_fail (mutex != NULL, -VISUAL_ERROR_MUTEX_NULL);

	return visual_mem_free (mutex);
#else /* !VISUAL_THREAD_MODEL_POSIX */
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif
#else
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * A VisMutex that has not been allocated using visual_mutex_new () can be initialized using this function. You can
 * use non allocated VisMutex variables in this context by giving a reference to them.
 *
 * @param mutex Pointer to the VisMutex which needs to be initialized.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_MUTEX_NULL or -VISUAL_ERROR_THREAD_NO_THREADING on failure.
 */
int visual_mutex_init (VisMutex *mutex)
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	visual_log_return_val_if_fail (mutex != NULL, -VISUAL_ERROR_MUTEX_NULL);

	memset (mutex, 0, sizeof (VisMutex));

	pthread_mutex_init (&mutex->mutex, NULL);

	return VISUAL_OK;
#else /* !VISUAL_THREAD_MODEL_POSIX */
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif
#else	
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif /* VISUAL_HAVE_THREADS */

}

/**
 * Locks a VisMutex, with the VisMutex locks checked right, only one thread can access the area at once.
 *	This will block if the thread is already locked.
 *
 * @param mutex Pointer to the VisMutex to register the lock.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_MUTEX_NULL, -VISUAL_ERROR_MUTEX_LOCK_FAILURE or
 *	-VISUAL_ERROR_THREAD_NO_THREADING on failure.
 */
int visual_mutex_lock (VisMutex *mutex)
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	visual_log_return_val_if_fail (mutex != NULL, -VISUAL_ERROR_MUTEX_NULL);

	if (pthread_mutex_lock (&mutex->mutex) < 0)
		return -VISUAL_ERROR_MUTEX_LOCK_FAILURE;

	return VISUAL_OK;
#else /* !VISUAL_THREAD_MODEL_POSIX */
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif
#else
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * Tries to lock a VisMutex, however instead of visual_mutex_lock it does not block on failure. but returns
 * instead with -VISUAL_ERROR_MUTEX_TRYLOCK_FAILURE as error value.
 *
 * @param mutex Pointer to the VisMutex that needs to be locked.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_MUTEX_NULL, -VISUAL_ERROR_MUTEX_TRYLOCK_FAILURE or
 *	-VISUAL_ERROR_THREAD_NO_THREADING on failure.
 */
int visual_mutex_trylock (VisMutex *mutex)
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	visual_log_return_val_if_fail (mutex != NULL, -VISUAL_ERROR_MUTEX_NULL);

	if (pthread_mutex_trylock (&mutex->mutex) < 0)
		return -VISUAL_ERROR_MUTEX_TRYLOCK_FAILURE;

	return VISUAL_OK;
#else /* !VISUAL_THREAD_MODEL_POSIX */
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif
#else
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * Unlocks a VisMutex so other threads that use the same lock can now enter the critical area.
 *
 * @param mutex Pointer to the VisMutex that is unlocked.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_MUTEX_NULL, -VISUAL_ERROR_MUTEX_UNLOCK_FAILURE or
 *	-VISUAL_ERROR_THREAD_NO_THREADING on failure.
 */
int visual_mutex_unlock (VisMutex *mutex)
{
#ifdef VISUAL_HAVE_THREADS
#ifdef VISUAL_THREAD_MODEL_POSIX
	visual_log_return_val_if_fail (mutex != NULL, -VISUAL_ERROR_MUTEX_NULL);

	if (pthread_mutex_unlock (&mutex->mutex) < 0)
		return -VISUAL_ERROR_MUTEX_UNLOCK_FAILURE;

	return VISUAL_OK;
#else /* !VISUAL_THREAD_MODEL_POSIX */
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif
#else
	return -VISUAL_ERROR_THREAD_NO_THREADING;
#endif /* VISUAL_HAVE_THREADS */
}

/**
 * @}
 */

