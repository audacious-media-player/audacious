/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#include "plugin.h"
#include <libmodplug/modplug.h>
#include "gui/main.h"

extern InputPlugin gModPlug;

static void Init(void)
{
	gModplugXMMS.SetInputPlugin(gModPlug);
	gModplugXMMS.Init();
}

static int CanPlayFile(char* aFilename)
{
	if(gModplugXMMS.CanPlayFile(aFilename))
		return 1;
	return 0;
}

static void PlayFile(char* aFilename)
{
	gModplugXMMS.SetOutputPlugin(*gModPlug.output);
	gModplugXMMS.PlayFile(aFilename);
}

static void Stop(void)
{
	gModplugXMMS.Stop();
}

static void Pause(short aPaused)
{
	gModplugXMMS.Pause((bool)aPaused);
}

static void Seek(int aTime)
{
	gModplugXMMS.Seek(float32(aTime));
}
static int GetTime(void)
{
	float32 lTime;
	
	lTime = gModplugXMMS.GetTime();
	if(lTime == -1)
		return -1;
	else
		return (int)(lTime * 1000);
}

static void GetSongInfo(char* aFilename, char** aTitle, int* aLength)
{
	gModplugXMMS.GetSongInfo(aFilename, *aTitle, *aLength);
}

void ShowAboutBox(void)
{
	ShowAboutWindow();
}

void ShowConfigureBox(void)
{
	ShowConfigureWindow(gModplugXMMS.GetModProps());
}

void ShowFileInfoBox(char* aFilename)
{
	ShowInfoWindow(aFilename);
}

InputPlugin gModPlug =
{
	NULL,
	NULL,
	"ModPlug Player",
	Init,
	ShowAboutBox,
	ShowConfigureBox,
	CanPlayFile,
	NULL,
	PlayFile,
	Stop,
	Pause,
	Seek,
	NULL,
	GetTime,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	GetSongInfo,
	ShowFileInfoBox,
	NULL
};

extern "C"
{
	InputPlugin* get_iplugin_info (void)
	{
		return &gModPlug;
	}
}
