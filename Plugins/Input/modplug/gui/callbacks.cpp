#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <iostream>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "../modplugbmp.h"


gboolean
hide_window                            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gtk_widget_hide(widget);
	return TRUE;
}

void
on_about_close_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget* lAboutWindow;
	
	lAboutWindow = lookup_widget((GtkWidget*)button, "About");
	if(!lAboutWindow)
		cerr << "ModPlug: on_about_close_clicked: Could not find about window!" << endl;
	else
		gtk_widget_hide(lAboutWindow);
}


void
on_config_apply_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	ModplugXMMS::Settings lProps;
	
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "bit8")))
		lProps.mBits = 8;
	else
		lProps.mBits = 16;
	
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "samp11")))
		lProps.mFrequency = 11025;
	else if (gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "samp22")))
		lProps.mFrequency = 22050;
	else
		lProps.mFrequency = 44100;
	
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "resampNearest")))
		lProps.mResamplingMode = 0;
	else if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "resampLinear")))
		lProps.mResamplingMode = 1;
	else if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "resampSpline")))
		lProps.mResamplingMode = 2;
	else
		lProps.mResamplingMode = 3;
	
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "mono")))
		lProps.mChannels = 1;
	else
		lProps.mChannels = 2;
	
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "fxNR")))
		lProps.mNoiseReduction = true;
	else
		lProps.mNoiseReduction = false;
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "fxFastInfo")))
		lProps.mFastinfo       = true;
	else
		lProps.mFastinfo       = false;
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "fxUseFilename")))
		lProps.mUseFilename    = true;
	else
		lProps.mUseFilename    = false;
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "fxReverb")))
		lProps.mReverb         = true;
	else
		lProps.mReverb         = false;
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "fxBassBoost")))
		lProps.mMegabass       = true;
	else
		lProps.mMegabass       = false;
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "fxSurround")))
		lProps.mSurround       = true;
	else
		lProps.mSurround       = false;
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "fxPreamp")))
		lProps.mPreamp         = true;
	else
		lProps.mPreamp         = false;
	
	if(gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "fxLoopForever")))
		lProps.mLoopCount = -1;
	else if (gtk_toggle_button_get_active((GtkToggleButton*)lookup_widget((GtkWidget*)button, "fxLoopFinite")))
	{
		lProps.mLoopCount =
			(uint32)gtk_spin_button_get_adjustment(
			(GtkSpinButton*)lookup_widget((GtkWidget*)button, "fxLoopCount"))->value;
	}
	else
		lProps.mLoopCount = 0;
	
	//hmm...  GTK objects have un-protected data members?  That's not good...
	lProps.mReverbDepth =
		(uint32)gtk_range_get_adjustment((GtkRange*)lookup_widget((GtkWidget*)button, "fxReverbDepth"))->value;
	lProps.mReverbDelay =
		(uint32)gtk_range_get_adjustment((GtkRange*)lookup_widget((GtkWidget*)button, "fxReverbDelay"))->value;
	lProps.mBassAmount =
		(uint32)gtk_range_get_adjustment((GtkRange*)lookup_widget((GtkWidget*)button, "fxBassAmount"))->value;
	lProps.mBassRange =
		(uint32)gtk_range_get_adjustment((GtkRange*)lookup_widget((GtkWidget*)button, "fxBassRange"))->value;
	lProps.mSurroundDepth =
		(uint32)gtk_range_get_adjustment((GtkRange*)lookup_widget((GtkWidget*)button, "fxSurroundDepth"))->value;
	lProps.mSurroundDelay =
		(uint32)gtk_range_get_adjustment((GtkRange*)lookup_widget((GtkWidget*)button, "fxSurroundDelay"))->value;
	lProps.mPreampLevel =
		(float)gtk_range_get_adjustment((GtkRange*)lookup_widget((GtkWidget*)button, "fxPreampLevel"))->value;
	
	gModplugXMMS.SetModProps(lProps);
}


void
on_config_ok_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget* lConfigWindow;
	
	on_config_apply_clicked(button, user_data);
	
	lConfigWindow = lookup_widget((GtkWidget*)button, "Config");
	if(!lConfigWindow)
		cerr << "ModPlug: on_config_ok_clicked: Could not find config window!" << endl;
	else
		gtk_widget_hide(lConfigWindow);
}


void
on_config_cancel_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget* lConfigWindow;
	
	lConfigWindow = lookup_widget((GtkWidget*)button, "Config");
	if(!lConfigWindow)
		cerr << "ModPlug: on_config_ok_clicked: Could not find config window!" << endl;
	else
		gtk_widget_hide(lConfigWindow);
}


void
on_info_close_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	GtkWidget* lInfoWindow;
	
	lInfoWindow = lookup_widget((GtkWidget*)button, "Info");
	if(!lInfoWindow)
		cerr << "ModPlug: on_info_close_clicked: Could not find info window!" << endl;
	else
		gtk_widget_hide(lInfoWindow);
}
