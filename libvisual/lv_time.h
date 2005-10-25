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

#ifndef _LV_TIME_H
#define _LV_TIME_H

#include <sys/time.h>
#include <time.h>

#include <libvisual/lv_common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
	
#define VISUAL_USEC_PER_SEC	1000000

typedef struct _VisTime VisTime;
typedef struct _VisTimer VisTimer;

/**
 * The VisTime structure can contain seconds and microseconds for timing purpose.
 */
struct _VisTime {
	VisObject	object;		/**< The VisObject data. */
	long		tv_sec;		/**< seconds. */
	long		tv_usec;	/**< microseconds. */
};

/**
 * The VisTimer structure is used for timing using the visual_timer_* methods.
 */
struct _VisTimer {
	VisObject	object;		/**< The VisObject data. */
	VisTime		start;		/**< Private entry to indicate the starting point,
					 * Shouldn't be read for reliable starting point because
					 * the visual_timer_continue method changes it's value. */
	VisTime		stop;		/**< Private entry to indicate the stopping point. */

	int		active;		/**< Private entry to indicate if the timer is currently active or inactive. */
};

VisTime *visual_time_new (void);
int visual_time_get (VisTime *time_);
int visual_time_set (VisTime *time_, long sec, long usec);
int visual_time_difference (VisTime *dest, VisTime *time1, VisTime *time2);
int visual_time_copy (VisTime *dest, VisTime *src);
int visual_time_usleep (unsigned long microseconds);

VisTimer *visual_timer_new (void);
int visual_timer_is_active (VisTimer *timer);
int visual_timer_start (VisTimer *timer);
int visual_timer_stop (VisTimer *timer);
int visual_timer_continue (VisTimer *timer);
int visual_timer_elapsed (VisTimer *timer, VisTime *time_);
int visual_timer_elapsed_msecs (VisTimer *timer);
int visual_timer_has_passed (VisTimer *timer, VisTime *time_);
int visual_timer_has_passed_by_values (VisTimer *timer, long sec, long usec);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LV_TIME_H */
