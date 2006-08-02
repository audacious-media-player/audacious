/***************************************************************************
 *            thinklight.c
 *
 *  Sun Dec 26 17:02:38 2004
 *  Copyright  2004  Benedikt 'Hunz' Heinz
 *  rocklight@hunz.org
 *  $Id: thinklight.c,v 1.2 2005/03/26 21:29:17 hunz Exp $
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int thinklight_open(void) {
	return open("/proc/acpi/ibm/light",O_RDWR);
}

void thinklight_close(int fd) {
	close(fd);	
}

int thinklight_set(int fd, int state) {
	return write(fd,state?"on\n":"off\n",state?3:4);
}

int thinklight_get(int fd) {
	char buf[256];
	int ret=read(fd,&buf,sizeof(buf));
	
	if(ret<0)
		return ret;
	else if(ret<11)
		return -23;
	else if(buf[10]=='f')
		return 0;
	else if(buf[10]=='n')
		return 1;
	else
		return -42;
}
