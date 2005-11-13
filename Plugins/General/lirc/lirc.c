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

#include "audacious/plugin.h"
#include "libaudacious/configfile.h"
#include "libaudacious/beepctrl.h"

#include <lirc/lirc_client.h>

#include "lirc.h"

const char *plugin_name="LIRC Plugin " VERSION;

GeneralPlugin lirc_plugin = {
	NULL,
	NULL,
	-1,
	NULL,
	init,
	about,
	configure,
	cleanup,
};

GeneralPlugin *get_gplugin_info(void)
{
        lirc_plugin.description = g_strdup_printf("LIRC Plugin"); 
        return(&lirc_plugin);
}

int lirc_fd=-1;
struct lirc_config *config=NULL;
gint tracknr=0;
gint mute=0;        /* mute flag */
gint mute_vol=0;    /* holds volume before mute */

gint input_tag;

int select_tag=0;
GtkWidget *select_popup=NULL;
GtkStyle *popup_style=NULL;
GList *select_list=NULL;
GtkWidget *fsd;
gchar *fontname;

struct select_info
{
	char *s;
	size_t common;
};

struct title_info
{
	char *s;
	int pos;
};

void clear_select_list(GList **list)
{
	GList *l;

	l=*list;
	while(l!=NULL)
	{
		free(((struct select_info *) (l->data))->s);
		free(l->data);
		l=g_list_remove_link(l,l);
	}
	*list=NULL;
}

void clear_title_list(GList **list)
{
	GList *l;

	l=*list;
	while(l!=NULL)
	{
		free(((struct title_info *) (l->data))->s);
		free(l->data);
		l=g_list_remove_link(l,l);
	}
	*list=NULL;
}

gchar *get_title_postfix(gchar *title)
{
	gchar *match;
	GList *scan_list;
	struct select_info *si;
	size_t skip;
	
	match=title;
	scan_list=select_list;
	si=(struct select_info *) (scan_list->data);
	while(*match)
	{
		if(!isalnum(*match))
		{
			match++;
		}
		else
		{
			if(strchr(si->s,toupper(*match))!=NULL)
			{
				skip=si->common;
				while(skip>0)
				{
					if(isalnum(*match))
					{
						skip--;
					}
					match++;
					if(!*match)
					{
						break;
					}
				}
				scan_list=g_list_next(scan_list);
				if(scan_list==NULL)
				{
					while(!isalnum(*match) && *match)
					{
						match++;
					}
					return(match);
				}
				si=(struct select_info *) (scan_list->data);
			}
			else
			{
				break;
			}
		}
	}
	return(NULL);
}

#define SELECT_BORDER 8

gint remove_select_popup(GtkWidget **select_popup)
{
	gtk_widget_destroy(*select_popup);
	*select_popup=NULL;
	select_tag=0;
	return(FALSE);
}

/*void select_title(char *s,int num)
{
	GdkFont *font;
	GtkStyle *style;
	int gap,baseline_skip;
	int width,height,x,y;
	GList *new_list,*scan_list;
	GList *title_list=NULL;
	int length,i;
	gchar *title;
	struct select_info *si;
	struct title_info *ti;
	
	if(select_tag!=0)
	{
		gtk_timeout_remove(select_tag);
		remove_select_popup(&select_popup);
	}
	if(s!=NULL)
	{
		si=malloc(sizeof(*si));
		if(si==NULL)
		{
			return;
		}
		si->s=strdup(s);
		si->common=1;
		if(si->s==NULL)
		{
			free(si);
			return;
		}
		new_list=g_list_append(select_list,si);
		if(new_list==NULL)
		{
			free(si->s);
			free(si);
			return;
		}
		
		select_list=new_list;
	}
	
	scan_list=select_list;
	while(scan_list)
	{
		scan_list=g_list_next(scan_list);
	}
	
	length=xmms_remote_get_playlist_length
		(lirc_plugin.xmms_session);
	for(i=0;i<length;i++)
	{
		title=xmms_remote_get_playlist_title
			(lirc_plugin.xmms_session,i);
		if(get_title_postfix(title)!=NULL)
		{
			ti=malloc(sizeof(*ti));
			if(ti==NULL) break;
			ti->s=strdup(title);
			if(ti->s==NULL)
			{
				free(ti);
				break;
			}
			ti->pos=i;
			new_list=g_list_append(title_list,ti);
			if(new_list==NULL)
			{
				free(ti->s);
				free(ti);
				break;
			}
			title_list=new_list;
		}
	}
	select_popup=gtk_window_new(GTK_WINDOW_POPUP);
	if(popup_style)
	{
		 if(fontname!=NULL)
		{
			font=gdk_font_load(fontname);
		}
		else
		{
			font=gdk_font_load
				("-xxl-*-medium-r-semicondensed-*-*-*-75-75-*-*-*-*");
		}
		if(font) popup_style->font=font;
		gtk_widget_set_style(select_popup,popup_style);
	}	
	gtk_widget_set_app_paintable(select_popup,TRUE);
	gtk_window_set_policy(GTK_WINDOW(select_popup),FALSE,FALSE,TRUE);
	gtk_widget_set_name(select_popup,"LIRC select list");
	style=select_popup->style;
	font=style->font;
	gap=(font->ascent+font->descent)/4;
	if(gap<2) gap=2;
	baseline_skip=font->ascent+font->descent+gap;
	
	if(title_list==NULL)
	{
		gchar *text="No match.";
		
		x=gdk_string_width(font,text);
		width=SELECT_BORDER*2+x;
		height=SELECT_BORDER*2+baseline_skip-gap;
		gtk_widget_set_usize(select_popup,width,height);
		x=(gdk_screen_width()-width)/2;
		if(x<0) x=0;
		y=(gdk_screen_height()-height)/2;
		if(y<0) y=0;
		gtk_widget_popup(select_popup,x,y);
		gtk_paint_box(select_popup->style,select_popup->window,
			      GTK_STATE_NORMAL,GTK_SHADOW_OUT,
			      NULL,GTK_WIDGET(select_popup),
			      "LIRC select list",0,0,-1,-1);

		gtk_paint_string(style,select_popup->window,
				 GTK_STATE_NORMAL,NULL,
				 GTK_WIDGET(select_popup),
				 "LIRC select item",
				 SELECT_BORDER,
				 SELECT_BORDER+font->ascent,
				 "No match.");
		
		clear_select_list(&select_list);
	}
	else
	{
		int count;
		int common;
		int pass;
		int x1,x2,x3;
		
		if(s==NULL && num>0)
		{
			scan_list=title_list;
			while(scan_list!=NULL)
			{
				num--;
				if(num==0)
				{
					clear_select_list(&select_list);
					xmms_remote_set_playlist_pos
						(lirc_plugin.xmms_session,
						 ((struct title_info *) (scan_list->data))->pos);
					break;
				}
				scan_list=g_list_next(scan_list);
			}
			clear_title_list(&title_list);
			return;
		}

		 check how many chars are common 
		scan_list=title_list;
		title=get_title_postfix
			(((struct title_info *) scan_list->data)->s);
		common=0;
		count=strlen(title);
		for(i=0;i<count;i++)
		{
			if(isalnum(title[i])) common++;
		}
		scan_list=g_list_next(scan_list);
		while(scan_list!=NULL)
		{
			gchar *title1,*title2;
			
			count=0;
			title1=title;
			title2=get_title_postfix
				(((struct title_info *) scan_list->data)->s);
			while(*title1 && *title2)
			{
				if(!isalnum(*title1)) {title1++;continue;}
				if(!isalnum(*title2)) {title2++;continue;}
				if(toupper(*title1)==toupper(*title2))
				{
					count++;
					title1++;
					title2++;
				}
				else
				{
					break;
				}
			}
			if(count<common) common=count;
			scan_list=g_list_next(scan_list);
		}
		scan_list=select_list;
		while(g_list_next(scan_list)!=NULL)
		{
			scan_list=g_list_next(scan_list);
		}
		((struct select_info *) (scan_list->data))->common=common+1;
		
		 print out selected titles 
		x1=x2=x3=0;
		count=0;
		pass=1;
		scan_list=title_list;
		while(scan_list!=NULL || pass==1)
		{
			gchar *title,*postfix,help;
			char buffer[20];
			
			count++;
			title=((struct title_info *) scan_list->data)->s;
			postfix=get_title_postfix(title);
			help=postfix[0];
			postfix[0]=0;
			
			snprintf(buffer,20,"%2d:",count);
			if(pass==1)
			{
				x=gdk_string_width
					(select_popup->style->font,buffer);
				if(x>x1) x1=x;
				x2=gdk_string_width
					(select_popup->style->font,title);
				postfix[0]=help;
				x=gdk_string_width(select_popup->style->font,
						   postfix);
				if(x>x3) x3=x;
			}
			else
			{
				gtk_paint_string(style,select_popup->window,
						 GTK_STATE_NORMAL,NULL,
						 GTK_WIDGET(select_popup),
						 "LIRC select item",
						 SELECT_BORDER,
						 SELECT_BORDER+font->ascent+
						 (count-1)*baseline_skip,
						 buffer);
				gtk_paint_string(style,select_popup->window,
						 GTK_STATE_NORMAL,NULL,
						 GTK_WIDGET(select_popup),
						 "LIRC select item",
						 SELECT_BORDER+x1,
						 SELECT_BORDER+font->ascent+
						 (count-1)*baseline_skip,
						 title);
				postfix[0]=help;
				gtk_paint_string(style,select_popup->window,
						 GTK_STATE_NORMAL,NULL,
						 GTK_WIDGET(select_popup),
						 "LIRC select item",
						 SELECT_BORDER+x1+x2+10,
						 SELECT_BORDER+font->ascent+
						 (count-1)*baseline_skip,
						 postfix);
				
			}
			scan_list=g_list_next(scan_list);
			if(scan_list==NULL)
			{
				if(pass==1)
				{
					int x,y;
					
					width=SELECT_BORDER*2+x1+x2+10+x3;
					height=SELECT_BORDER*2+count*baseline_skip-gap;
					
					gtk_widget_set_usize(select_popup,
							     width,
							     height);
					x=(gdk_screen_width()-width)/2;
					if(x<0) x=0;
					y=(gdk_screen_height()-height)/2;
					if(y<0) y=0;
					gtk_widget_popup(select_popup,x,y);
					gtk_paint_box(style,select_popup->window,
						      GTK_STATE_NORMAL,
						      GTK_SHADOW_OUT,
						      NULL,GTK_WIDGET(select_popup),
						      "LIRC select list",
						      0,0,-1,-1);
					scan_list=title_list;
					count=0;
				}
				pass++;
			}
		}
		if(count==1)
		{
			clear_select_list(&select_list);
			xmms_remote_set_playlist_pos
				(lirc_plugin.xmms_session,
				 ((struct title_info *) title_list->data)
				 ->pos);
		}
		clear_title_list(&title_list);
		
	}
	select_tag=gtk_timeout_add(3000,(GtkFunction) remove_select_popup,
				   (gpointer) &select_popup);
}
*/
void init(void)
{
	int flags;
	ConfigFile *cfg;
	
	if((lirc_fd=lirc_init("audacious",1))==-1)
	{
		fprintf(stderr,"%s: could not init LIRC support\n",
			plugin_name);
		return;
	}
	if(lirc_readconfig(NULL,&config,NULL)==-1)
	{
		lirc_deinit();
		fprintf(stderr,
			"%s: could not read LIRC config file\n"
			"%s: please read the documentation of LIRC \n"
			"%s: how to create a proper config file\n",
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
	cfg=xmms_cfg_open_default_file();
	if(cfg)
	{
		xmms_cfg_read_string(cfg,"LIRC","font",&fontname);
		xmms_cfg_free(cfg);
	}
	popup_style=gtk_style_new();
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
#ifdef HAVE_XMMS_REMOTE_QUIT
				xmms_remote_quit(lirc_plugin.xmms_session);
#else
				raise(SIGTERM);
#endif
			}
			else if(strcasecmp("SETPOS",c)==0)
			{
				xmms_remote_set_playlist_pos(lirc_plugin.xmms_session,tracknr-1);
				xmms_remote_play(lirc_plugin.xmms_session);
				tracknr=0; 
			}
			else if(strcasecmp("ONE",c)==0)
			{
				if(select_list!=NULL)
				{
					/* select_title(NULL,1); */
				}
				else
				{
					tracknr=tracknr*10+1; 
				}
                        }
			else if(strcasecmp("TWO",c)==0)
			{
				if(select_list!=NULL)
				{
					/* select_title(NULL,2); */
				}
				else
				{
					tracknr=tracknr*10+2; 
				}
                        }
			else if(strcasecmp("THREE",c)==0)
			{
				if(select_list!=NULL)
				{
					/* select_title(NULL,3);*/
				}
				else
				{
					tracknr=tracknr*10+3; 
				}
                        }
			else if(strcasecmp("FOUR",c)==0)
			{
				if(select_list!=NULL)
				{
					/* select_title(NULL,4);*/
				}
				else
				{
					tracknr=tracknr*10+4; 
				}
                        }
			else if(strcasecmp("FIVE",c)==0)
			{
				if(select_list!=NULL)
				{
					/*select_title(NULL,5);*/
				}
				else
				{
					tracknr=tracknr*10+5; 
				}
                        }
			else if(strcasecmp("SIX",c)==0)
			{
				if(select_list!=NULL)
				{
					/*select_title(NULL,6);*/
				}
				else
				{
					tracknr=tracknr*10+6; 
				}
                        }
			else if(strcasecmp("SEVEN",c)==0)
			{
				if(select_list!=NULL)
				{
					/*select_title(NULL,7);*/
				}
				else
				{
					tracknr=tracknr*10+7; 
				}
                        }
			else if(strcasecmp("EIGHT",c)==0)
			{
				if(select_list!=NULL)
				{
					/*select_title(NULL,8);*/
				}
				else
				{
					tracknr=tracknr*10+8; 
				}
                        }
			else if(strcasecmp("NINE",c)==0)
			{
				if(select_list!=NULL)
				{
					/*select_title(NULL,9);*/
				}
				else
				{
					tracknr=tracknr*10+9; 
				}
                        }
			else if(strcasecmp("ZERO",c)==0)
			{
				if(select_list!=NULL)
				{
					/*select_title(NULL,10);*/
				}
				else
				{
					tracknr=tracknr*10; 
				}
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
			else if(strncasecmp("SELECT",c,6)==0)
			{
				char *sel;
				int i;
				
                                ptr=c+6;
                                while(isspace(*ptr)) ptr++;
				
				sel=ptr;
				for(i=0;*ptr;ptr++)
				{
					if(isalnum(*ptr))
					{
						sel[i]=toupper(*ptr);
						i++;
					}
				}
				sel[i]=0;
				
				if(strlen(sel)>0)
				{
					/*select_title(sel,0);*/
				}
			}
			else
			{
				fprintf(stderr,"%s: unknown command \"%s\"\n",
					plugin_name,c);
			}
		}
		free(code);
		if(ret==-1) break;
	}
	if(ret==-1)
	{
		/* something went badly wrong */
		fprintf(stderr,"%s: disconnected from LIRC\n",plugin_name);
		cleanup();
		return;
	}
}

void configure(void)
{
	if(!fsd)
	{
		GtkWidget *window;
		
		window=gtk_font_selection_dialog_new
			("Please choose font for SELECT popup.");
		g_return_if_fail(GTK_IS_FONT_SELECTION_DIALOG(window));
		
		fsd=window;
		gtk_window_position(GTK_WINDOW(fsd),GTK_WIN_POS_MOUSE);
		gtk_signal_connect(GTK_OBJECT(fsd), "destroy",
				   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
				   &fsd);
		gtk_signal_connect(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG
					      (fsd)->ok_button),
				   "clicked",
				   GTK_SIGNAL_FUNC(font_selection_ok),
				   NULL);
		gtk_signal_connect_object(GTK_OBJECT(GTK_FONT_SELECTION_DIALOG(fsd)->cancel_button),
					  "clicked",
					  GTK_SIGNAL_FUNC(gtk_widget_destroy),
					  GTK_OBJECT(fsd));
	}
	if(!GTK_WIDGET_VISIBLE(fsd))
		gtk_widget_show(fsd);
	else
		gtk_widget_destroy(fsd);
}

void font_selection_ok(GtkWidget *button,gpointer *data)
{
	gchar *new_fontname;
	
	new_fontname=gtk_font_selection_dialog_get_font_name
		(GTK_FONT_SELECTION_DIALOG(fsd));
	if(new_fontname)
	{
		if(fontname) free(fontname);
		fontname=new_fontname;
	}
	gtk_widget_destroy(fsd);
	fsd=NULL;
}

void cleanup()
{
	ConfigFile *cfg;
	
	if(config)
	{
		gtk_input_remove(input_tag);
		lirc_freeconfig(config);
		config=NULL;
	}
	if(lirc_fd!=-1)
	{
		lirc_deinit();
		lirc_fd=-1;
	}
	clear_select_list(&select_list);
	if(fontname)
	{
		cfg=xmms_cfg_open_default_file();
		if(cfg)
		{
			xmms_cfg_write_string(cfg,"LIRC","font",fontname);
			xmms_cfg_write_default_file(cfg);
			xmms_cfg_free(cfg);
		}
		free(fontname);
	}
}
