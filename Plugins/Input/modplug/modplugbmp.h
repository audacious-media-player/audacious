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

/* Module files have their magic deep inside the file, at offset 1080; source: http://www.onicos.com/staff/iz/formats/mod.html and information by Michael Doering from UADE */
#define MOD_MAGIC_PROTRACKER4	(unsigned char [4]) { 0x4D, 0x2E, 0x4B, 0x2E }  // "M.K." - Protracker 4 channel
#define MOD_MAGIC_PROTRACKER4X	(unsigned char [4]) { 0x4D, 0x21, 0x4B, 0x21 }  // "M!K!" - Protracker 4 channel
#define MOD_MAGIC_NOISETRACKER	(unsigned char [4]) { 0x4D, 0x26, 0x4B, 0x21 }  // "M&K!" - Noisetracker 1.3 by Kaktus & Mahoney
#define MOD_MAGIC_STARTRACKER4	(unsigned char [4]) { 0x46, 0x4C, 0x54, 0x34 }  // "FLT4" - Startracker 4 channel (Startrekker/AudioSculpture)
#define MOD_MAGIC_STARTRACKER8	(unsigned char [4]) { 0x46, 0x4C, 0x54, 0x38 }  // "FLT8" - Startracker 8 channel (Startrekker/AudioSculpture) 
#define MOD_MAGIC_STARTRACKER4X	(unsigned char [4]) { 0x45, 0x58, 0x30, 0x34 }  // "EX04" - Startracker 4 channel (Startrekker/AudioSculpture)
#define MOD_MAGIC_STARTRACKER8X	(unsigned char [4]) { 0x45, 0x58, 0x30, 0x38 }  // "EX08" - Startracker 8 channel (Startrekker/AudioSculpture) 
#define MOD_MAGIC_FASTTRACKER4	(unsigned char [4]) { 0x34, 0x43, 0x48, 0x4E }  // "4CHN" - Fasttracker 4 channel
#define MOD_MAGIC_FASTTRACKER6	(unsigned char [4]) { 0x36, 0x43, 0x48, 0x4E }  // "6CHN" - Fasttracker 6 channel
#define MOD_MAGIC_FASTTRACKER8	(unsigned char [4]) { 0x38, 0x43, 0x48, 0x4E }  // "8CHN" - Fasttracker 8 channel
#define MOD_MAGIC_OKTALYZER8	(unsigned char [4]) { 0x43, 0x44, 0x38, 0x31 }  // "CD81" - Atari oktalyzer 8 channel
#define MOD_MAGIC_OKTALYZER8X	(unsigned char [4]) { 0x4F, 0x4B, 0x54, 0x41 }  // "OKTA" - Atari oktalyzer 8 channel
#define MOD_MAGIC_TAKETRACKER16	(unsigned char [4]) { 0x31, 0x36, 0x43, 0x4E }  // "16CN" - Taketracker 16 channel
#define MOD_MAGIC_TAKETRACKER32 (unsigned char [4]) { 0x33, 0x32, 0x43, 0x4E }  // "32CN" - Taketracker 32 channel

#define S3M_MAGIC	(unsigned char [4]) { 0x53, 0x43, 0x52, 0x4D }			/* This is the SCRM string at offset 44 to 47 in the S3M header */

/* These nicer formats have the magic bytes at the front of the file where they belong */
#define UMX_MAGIC	(unsigned char [4]) { 0xC1, 0x83, 0x2A, 0x9E }
#define XM_MAGIC	(unsigned char [4]) { 0x45, 0x78, 0x74, 0x65 }			/* Exte(nded Module) */
#define M669_MAGIC	(unsigned char [4]) { 0x69, 0x66, 0x20, 0x20 }
#define IT_MAGIC	(unsigned char [4]) { 0x49, 0x4D, 0x50, 0x4D }			/* IMPM */
#define MTM_MAGIC	(unsigned char [4]) { 0x4D, 0x54, 0x4D, 0x10 }
#define PSM_MAGIC	(unsigned char [4]) { 0x50, 0x53, 0x4D, 0x20 }

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
		gboolean   mGrabAmigaMOD;
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
