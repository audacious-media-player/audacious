/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#ifndef __MODPLUGXMMS_CMODPLUGXMMS_H_INCLUDED__
#define __MODPLUGXMMS_CMODPLUGXMMS_H_INCLUDED__

#include <string>
#include <glib.h>

#ifndef __MODPLUGXMMS_STDDEFS_H__INCLUDED__
#include "stddefs.h"
#endif

#include "audacious/plugin.h"

using namespace std;

class CSoundFile;
class Archive;

class ModplugXMMS
{
public:
	struct Settings
	{
		gboolean   mSurround;
		gboolean   mOversamp;
		gboolean   mMegabass;
		gboolean   mNoiseReduction;
		gboolean   mVolumeRamp;
		gboolean   mReverb;
		gboolean   mFastinfo;
		gboolean   mUseFilename;
		gboolean   mPreamp;
	
		gint       mChannels;
		gint       mBits;
		gint       mFrequency;
		gint       mResamplingMode;
	
		gint       mReverbDepth;
		gint       mReverbDelay;
		gint       mBassAmount;
		gint       mBassRange;
		gint       mSurroundDepth;
		gint       mSurroundDelay;
		gfloat     mPreampLevel;
		gint       mLoopCount;
		
		Settings();
	};

	ModplugXMMS();
	~ModplugXMMS();
	
	void Init();                      // Called when the plugin is loaded
	bool CanPlayFile(const string& aFilename);// Return true if the plugin can handle the file

	void CloseConfigureBox();

	void PlayFile(const string& aFilename);// Play the file.
	void Stop();                       // Stop playing.
	void Pause(bool aPaused);              // Pause or unpause.

	void Seek(float32 aTime);                // Seek to the specified time.
	float32 GetTime();                   // Get the current play time.

	void GetSongInfo(const string& aFilename, char*& aTitle, int32& aLength); // Function to grab the title string

	void SetInputPlugin(InputPlugin& aInPlugin);
	void SetOutputPlugin(OutputPlugin& aOutPlugin);

	const Settings& GetModProps();
	void SetModProps(const Settings& aModProps);

private:
	InputPlugin*  mInPlug;
	OutputPlugin* mOutPlug;

	uchar*  mBuffer;
	uint32  mBufSize;

	bool          mPaused;
	volatile bool mStopped;

	Settings mModProps;

	AFormat mFormat;

	uint32  mBufTime;		//milliseconds

	CSoundFile* mSoundFile;
	Archive*    mArchive;

	uint32      mPlayed;

	GThread*    mDecodeThread;

	char        mModName[100];
	
	float mPreampFactor;

	void PlayLoop();
	static void* PlayThread(void* arg);
	const char* Bool2OnOff(bool aValue);
};

extern ModplugXMMS gModplugXMMS;

#endif //included
