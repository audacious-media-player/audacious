/* Lirc plugin

   Copyright (C) 2005 Audacious development team

   Copyright (c) 1998-1999 Carl van Schaik (carl@leg.uct.ac.za)

   Copyright (C) 2000 Christoph Bartelmus (xmms@bartelmus.de)

   some code was stolen from:
   IRman plugin for xmms by Charles Sielski (stray@teklabs.net)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "audacious/plugin.h"
#include "libaudacious/beepctrl.h"

#include <lirc/lirc_client.h>

#include "lirc.h"

const char *plugin_name="LIRC Plugin";

GeneralPlugin lirc_plugin = {
	NULL,
	NULL,
	-1,
	NULL,
	init,
	about,
	NULL,
	cleanup,
};

GeneralPlugin *get_gplugin_info(void)
{
        lirc_plugin.description = g_strdup_printf(_("LIRC Plugin")); 
        return(&lirc_plugin);
}

int lirc_fd=-1;
struct lirc_config *config=NULL;
gint tracknr=0;
gint mute=0;        /* mute flag */
gint mute_vol=0;    /* holds volume before mute */

gint input_tag;

void init(void)
{
	int flags;
	
	if((lirc_fd=lirc_init("audacious",1))==-1)
	{
		fprintf(stderr,_("%s: could not init LIRC support\n"),
			plugin_name);
		return;
	}
	if(lirc_readconfig(NULL,&config,NULL)==-1)
	{
		lirc_deinit();
		fprintf(stderr,
			_("%s: could not read LIRC config file\n"
			"%s: please read the documentation of LIRC\n"
			"%s: how to create a proper config file\n"),
			plugin_name,plugin_name,plugin_name);
		return;
	}
	input_tag=gdk_input_add(lirc_fd,GDK_INPUT_READ,
				lirc_input_callback,NULL);
	fcntl(lirc_fd,F_SETOWN,getpid());
	flags=fcntl(lirc_fd,F_GETFL,0);
	if(flags!=-1)
	{
		fcntl(lirc_fd,F_SETFL,flags|O_NONBLOCK);
	}
	fflush(stdout);
}

void lirc_input_callback(gpointer data,gint source,
			 GdkInputCondition condition)
{
	char *code;
	char *c;
	gint playlist_time,playlist_pos,output_time,v;
	int ret;
	char *ptr;
	gint balance;
	gboolean show_pl;
        int n;
	
	while((ret=lirc_nextcode(&code))==0 && code!=NULL)
	{
		while((ret=lirc_code2char(config,code,&c))==0 && c!=NULL)
		{
			if(strcasecmp("PLAY",c)==0)
			{
				xmms_remote_play(lirc_plugin.xmms_session);
			}
			else if(strcasecmp("STOP",c)==0)
			{
				xmms_remote_stop(lirc_plugin.xmms_session);
			}
			else if(strcasecmp("PAUSE",c)==0)
			{
				xmms_remote_pause(lirc_plugin.xmms_session);
			}
			else if(strcasecmp("PLAYPAUSE",c) == 0)
			{
				if(xmms_remote_is_playing(lirc_plugin.
							  xmms_session))
					xmms_remote_pause
						(lirc_plugin.xmms_session);
				else
					xmms_remote_play
						(lirc_plugin.xmms_session);
			}
			else if(strncasecmp("NEXT",c,4)==0)
			{
                                ptr=c+4;
                                while(isspace(*ptr)) ptr++;
				n=atoi(ptr);
				
				if(n<=0) n=1;
				for(;n>0;n--)
				{
					xmms_remote_playlist_next
						(lirc_plugin.xmms_session);
				}
			}
			else if(strncasecmp("PREV",c,4)==0)
			{
                                ptr=c+4;
                                while(isspace(*ptr)) ptr++;
				n=atoi(ptr);
				
				if(n<=0) n=1;
				for(;n>0;n--)
				{
					xmms_remote_playlist_prev
						(lirc_plugin.xmms_session);
				}
			}
			else if(strcasecmp("SHUFFLE",c)==0)
			{
				xmms_remote_toggle_shuffle(lirc_plugin.xmms_session);
			}
			else if(strcasecmp("REPEAT",c)==0)
			{
				xmms_remote_toggle_repeat(lirc_plugin.xmms_session);
			}
			else if(strncasecmp("FWD",c,3)==0)
			{
                                ptr=c+3;
                                while(isspace(*ptr)) ptr++;
				n=atoi(ptr)*1000;
				
				if(n<=0) n=5000;
				output_time=xmms_remote_get_output_time
					(lirc_plugin.xmms_session);
				playlist_pos=xmms_remote_get_playlist_pos
					(lirc_plugin.xmms_session);
				playlist_time=xmms_remote_get_playlist_time
					(lirc_plugin.xmms_session,
					 playlist_pos);
				if(playlist_time-output_time<n)
					output_time=playlist_time-n;
				xmms_remote_jump_to_time
					(lirc_plugin.xmms_session,
					 output_time+n);
			}
			else if(strncasecmp("BWD",c,3)==0)
			{
                                ptr=c+3;
                                while(isspace(*ptr)) ptr++;
				n=atoi(ptr)*1000;

				if(n<=0) n=5000;
				output_time=xmms_remote_get_output_time
					(lirc_plugin.xmms_session);
				if(output_time<n)
					output_time=n;
				xmms_remote_jump_to_time
					(lirc_plugin.xmms_session,
					 output_time-n);
			}
			else if(strncasecmp("VOL_UP",c,6)==0)
			{
                                ptr=c+6;
                                while(isspace(*ptr)) ptr++;
				n=atoi(ptr);
                                if(n<=0) n=5;

				v = xmms_remote_get_main_volume
					(lirc_plugin.xmms_session);
				if(v > (100-n)) v=100-n;
				xmms_remote_set_main_volume
					(lirc_plugin.xmms_session,v+n);
			}
			else if(strncasecmp("VOL_DOWN",c,8)==0)
			{                                
                                ptr=c+8;
                                while (isspace(*ptr)) ptr++;
				n=atoi(ptr);
                                if(n<=0) n=5;

				v = xmms_remote_get_main_volume
					(lirc_plugin.xmms_session);
				if(v<n) v=n;
				xmms_remote_set_main_volume
					(lirc_plugin.xmms_session,v-n);
			}
			else if(strcasecmp("QUIT",c)==0)
			{
				xmms_remote_quit(lirc_plugin.xmms_session);
			}
			else if(strcasecmp("MUTE",c)==0)
 			{
				if(mute==0)
 				{
 					mute=1;
 					/* store the master volume so
                                           we can restore it on unmute. */
 					mute_vol = xmms_remote_get_main_volume
						(lirc_plugin.xmms_session);
 					xmms_remote_set_main_volume
						(lirc_plugin.xmms_session, 0);
 				}
 				else
 				{
 					mute=0;
 					xmms_remote_set_main_volume
						(lirc_plugin.xmms_session,
						 mute_vol);
 				}
			}
			else if(strncasecmp("BAL_LEFT",c,8)==0)
			{
                                ptr=c+8;
                                while(isspace(*ptr)) ptr++;
				n=atoi(ptr);
				if(n<=0) n=5;
				
				balance=xmms_remote_get_balance
					(lirc_plugin.xmms_session);
				balance-=n;
				if(balance<-100) balance=-100;
				xmms_remote_set_balance
					(lirc_plugin.xmms_session,balance);
			}
			else if(strncasecmp("BAL_RIGHT",c,9)==0)
			{
                                ptr=c+9;
                                while(isspace(*ptr)) ptr++;
				n=atoi(ptr);
				if(n<=0) n=5;

				balance=xmms_remote_get_balance
					(lirc_plugin.xmms_session);
				balance+=n;
				if(balance>100) balance=100;
				xmms_remote_set_balance
					(lirc_plugin.xmms_session,balance);
			}
			else if(strcasecmp("BAL_CENTER",c)==0)
			{
				balance=0;
				xmms_remote_set_balance
					(lirc_plugin.xmms_session,balance);
			}
			else if(strcasecmp("LIST",c)==0)
			{
				show_pl=xmms_remote_is_pl_win
					(lirc_plugin.xmms_session);
				show_pl=(show_pl) ? 0:1;
				xmms_remote_pl_win_toggle
					(lirc_plugin.xmms_session,show_pl);
 			}
			else if(strcasecmp("PLAYLIST_CLEAR",c)==0)
			{
				gboolean pl_visible;

				pl_visible=xmms_remote_is_pl_win
					(lirc_plugin.xmms_session);
				xmms_remote_stop(lirc_plugin.xmms_session);
				xmms_remote_playlist_clear
					(lirc_plugin.xmms_session);
				/* This is to refresh window content */
				xmms_remote_pl_win_toggle
					(lirc_plugin.xmms_session,pl_visible);
			}
			else if(strncasecmp("PLAYLIST_ADD ",c,13)==0)
			{
				gboolean pl_visible;
				GList list;

				pl_visible=xmms_remote_is_pl_win
					(lirc_plugin.xmms_session);
				list.prev=list.next=NULL;
				list.data=c+13;
				xmms_remote_playlist_add
					(lirc_plugin.xmms_session,&list);
				/* This is to refresh window content */
				xmms_remote_pl_win_toggle
					(lirc_plugin.xmms_session,pl_visible);
                        }
			else
			{
				fprintf(stderr,_("%s: unknown command \"%s\"\n"),
					plugin_name,c);
			}
		}
		free(code);
		if(ret==-1) break;
	}
	if(ret==-1)
	{
		/* something went badly wrong */
		fprintf(stderr,_("%s: disconnected from LIRC\n"),plugin_name);
		cleanup();
		return;
	}
}

void cleanup()
{
	if(config)
	{
		if(input_tag)
			gtk_input_remove(input_tag);

		config=NULL;
	}
	if(lirc_fd!=-1)
	{
		lirc_deinit();
		lirc_fd=-1;
	}
}
