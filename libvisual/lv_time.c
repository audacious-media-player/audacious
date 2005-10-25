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
#include <errno.h>
#include <string.h>

#include "config.h"
#include "lv_common.h"
#include "lv_thread.h"
#include "lv_time.h"

/**
 * @defgroup VisTime VisTime
 * @{
 */

/**
 * Creates a new VisTime structure.
 *
 * @return A newly allocated VisTime.
 */
VisTime *visual_time_new ()
{
	VisTime *time_;

	time_ = visual_mem_new0 (VisTime, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (time_), TRUE, NULL);

	return time_;
}

/**
 * Loads the current time into the VisTime structure.
 *
 * @param time_ Pointer to the VisTime in which the current time needs to be set.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TIME_NULL on failure.
 */
int visual_time_get (VisTime *time_)
{
	struct timeval tv;
	
	visual_log_return_val_if_fail (time_ != NULL, -VISUAL_ERROR_TIME_NULL);

	gettimeofday (&tv, NULL);

	visual_time_set (time_, tv.tv_sec, tv.tv_usec);

	return VISUAL_OK;
}

/**
 * Sets the time by sec, usec in a VisTime structure.
 *
 * @param time_ Pointer to the VisTime in which the time is set.
 * @param sec The seconds.
 * @param usec The microseconds.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TIME_NULL on failure.
 */
int visual_time_set (VisTime *time_, long sec, long usec)
{
	visual_log_return_val_if_fail (time_ != NULL, -VISUAL_ERROR_TIME_NULL);
	
	time_->tv_sec = sec;
	time_->tv_usec = usec;

	return VISUAL_OK;
}

/**
 * Copies ine VisTime into another.
 *
 * @param dest Pointer to the destination VisTime in which the source VisTime is copied.
 * @param src Pointer to the source VisTime that is copied to the destination VisTime.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TIME_NULL on failure.
 */
int visual_time_copy (VisTime *dest, VisTime *src)
{
	visual_log_return_val_if_fail (dest != NULL, -VISUAL_ERROR_TIME_NULL);
	visual_log_return_val_if_fail (src != NULL, -VISUAL_ERROR_TIME_NULL);

	dest->tv_sec = src->tv_sec;
	dest->tv_usec = src->tv_usec;

	return VISUAL_OK;
}

/**
 * Calculates the difference between two VisTime structures.
 *
 * @param dest Pointer to the VisTime that contains the difference between time1 and time2.
 * @param time1 Pointer to the first VisTime.
 * @param time2 Pointer to the second VisTime from which the first is substracted to generate a result.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TIME_NULL on failure.
 */
int visual_time_difference (VisTime *dest, VisTime *time1, VisTime *time2)
{
	visual_log_return_val_if_fail (dest != NULL, -VISUAL_ERROR_TIME_NULL);
	visual_log_return_val_if_fail (time1 != NULL, -VISUAL_ERROR_TIME_NULL);
	visual_log_return_val_if_fail (time2 != NULL, -VISUAL_ERROR_TIME_NULL);

	dest->tv_usec = time2->tv_usec - time1->tv_usec;
	dest->tv_sec = time2->tv_sec - time1->tv_sec;

	if (dest->tv_usec < 0) {
		dest->tv_usec = VISUAL_USEC_PER_SEC + dest->tv_usec;
		dest->tv_sec--;
	}

	return VISUAL_OK;
}

/**
 * Sleeps an certain amount of microseconds.
 *
 * @param microseconds The amount of microseconds we're going to sleep. To sleep a certain amount of
 *	seconds you can call this function with visual_time_usleep (VISUAL_USEC_PER_SEC * seconds).
 * 
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TIME_NO_USLEEP or -1 on failure.
 */
int visual_time_usleep (unsigned long microseconds)
{
#ifdef HAVE_NANOSLEEP
	struct timespec request, remaining;
	request.tv_sec = microseconds / VISUAL_USEC_PER_SEC;
	request.tv_nsec = 1000 * (microseconds % VISUAL_USEC_PER_SEC);
	while (nanosleep (&request, &remaining) == EINTR)
		request = remaining;
#elif HAVE_SELECT
	struct timeval tv;
	tv.tv_sec = microseconds / VISUAL_USEC_PER_SEC;
	tv.tv_usec = microseconds % VISUAL_USEC_PER_SEC;
	select (0, NULL, NULL, NULL, &tv);
#elif HAVE_USLEEP
	return usleep (microseconds);
#else
#warning visual_time_usleep() will does not work!
	return -VISUAL_ERROR_TIME_NO_USLEEP;
#endif
	return VISUAL_OK;
}

/**
 * Creates a new VisTimer structure.
 *
 * @return A newly allocated VisTimer.
 */
VisTimer *visual_timer_new ()
{
	VisTimer *timer;

	timer = visual_mem_new0 (VisTimer, 1);

	/* Do the VisObject initialization */
	visual_object_initialize (VISUAL_OBJECT (timer), TRUE, NULL);

	return timer;
}

/**
 * Checks if the timer is active.
 *
 * @param timer Pointer to the VisTimer of which we want to know if it's active.
 *
 * @return TRUE or FALSE, -VISUAL_ERROR_TIMER_NULL on failure.
 */
int visual_timer_is_active (VisTimer *timer)
{
	visual_log_return_val_if_fail (timer != NULL, -VISUAL_ERROR_TIMER_NULL);

	return timer->active;
}

/**
 * Starts a timer.
 *
 * @param timer Pointer to the VisTimer in which we start the timer.
 *
 * return VISUAL_OK on succes, -VISUAL_ERROR_TIMER_NULL on failure.
 */ 
int visual_timer_start (VisTimer *timer)
{
	visual_log_return_val_if_fail (timer != NULL, -VISUAL_ERROR_TIMER_NULL);

	visual_time_get (&timer->start);

	timer->active = TRUE;

	return VISUAL_OK;
}

/**
 * Stops a timer.
 *
 * @param timer Pointer to the VisTimer that is stopped.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TIMER_NULL on failure.
 */
int visual_timer_stop (VisTimer *timer)
{
	visual_log_return_val_if_fail (timer != NULL, -VISUAL_ERROR_TIMER_NULL);

	visual_time_get (&timer->stop);

	timer->active = FALSE;

	return VISUAL_OK;
}

/**
 * Continues a stopped timer. The timer needs to be stopped before continueing.
 *
 * @param timer Pointer to the VisTimer that is continued.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TIMER_NULL on failure.
 */
int visual_timer_continue (VisTimer *timer)
{
	VisTime elapsed;

	visual_log_return_val_if_fail (timer != NULL, -VISUAL_ERROR_TIMER_NULL);
	visual_log_return_val_if_fail (timer->active != FALSE, -VISUAL_ERROR_TIMER_NULL);

	visual_time_difference (&elapsed, &timer->start, &timer->stop);

	visual_time_get (&timer->start);

	if (timer->start.tv_usec < elapsed.tv_usec) {
		timer->start.tv_usec += VISUAL_USEC_PER_SEC;
		timer->start.tv_sec--;
	}

	timer->start.tv_sec -= elapsed.tv_sec;
	timer->start.tv_usec -= elapsed.tv_usec;

	timer->active = TRUE;

	return VISUAL_OK;
}

/**
 * Retrieve the amount of time passed since the timer has started.
 *
 * @param timer Pointer to the VisTimer from which we want to know the amount of time passed.
 * @param time_ Pointer to the VisTime in which we put the amount of time passed.
 *
 * @return VISUAL_OK on succes, -VISUAL_ERROR_TIMER_NULL or -VISUAL_ERROR_TIME_NULL on failure.
 */
int visual_timer_elapsed (VisTimer *timer, VisTime *time_)
{
	VisTime cur;

	visual_log_return_val_if_fail (timer != NULL, -VISUAL_ERROR_TIMER_NULL);
	visual_log_return_val_if_fail (time_ != NULL, -VISUAL_ERROR_TIME_NULL);

	visual_time_get (&cur);

	if (timer->active == TRUE)
		visual_time_difference (time_, &timer->start, &cur);
	else
		visual_time_difference (time_, &timer->start, &timer->stop);


	return VISUAL_OK;
}

/**
 * Returns the amount of milliseconds passed since the timer has started.
 * Be aware to not confuse milliseconds with microseconds.
 *
 * @param timer Pointer to the VisTimer from which we want to know the amount of milliseconds passed since activation.
 *
 * @return The amount of milliseconds passed, or -1 on error, this function is not clockscrew safe.
 */
int visual_timer_elapsed_msecs (VisTimer *timer)
{
	VisTime cur;

	visual_log_return_val_if_fail (timer != NULL, -1);

	visual_timer_elapsed (timer, &cur);

	return (cur.tv_sec * 1000) + (cur.tv_usec / 1000);
}

/**
 * Checks if the timer has passed a certain age.
 *
 * @param timer Pointer to the VisTimer which we check for age.
 * @param time_ Pointer to the VisTime containing the age we check against.
 *
 * @return TRUE on passed, FALSE if not passed, -VISUAL_ERROR_TIMER_NULL or -VISUAL_ERROR_TIME_NULL on failure.
 */
int visual_timer_has_passed (VisTimer *timer, VisTime *time_)
{
	VisTime elapsed;

	visual_log_return_val_if_fail (timer != NULL, -VISUAL_ERROR_TIMER_NULL);
	visual_log_return_val_if_fail (time_ != NULL, -VISUAL_ERROR_TIME_NULL);

	visual_timer_elapsed (timer, &elapsed);

	if (time_->tv_sec == elapsed.tv_sec && time_->tv_usec <= elapsed.tv_usec)
		return TRUE;
	else if (time_->tv_sec < elapsed.tv_sec)
		return TRUE;

	return FALSE;
}

/**
 * Checks if the timer has passed a certain age by values.
 *
 * @param timer Pointer to the VisTimer which we check for age.
 * @param sec The number of seconds we check against.
 * @param usec The number of microseconds we check against.
 *
 * @return TRUE on passed, FALSE if not passed, -VISUAL_ERROR_TIMER_NULL on failure.
 */
int visual_timer_has_passed_by_values (VisTimer *timer, long sec, long usec)
{
	VisTime passed;

	visual_log_return_val_if_fail (timer != NULL, -VISUAL_ERROR_TIMER_NULL);

	visual_time_set (&passed, sec, usec);

	return visual_timer_has_passed (timer, &passed);
}

/**
 * @}
 */

