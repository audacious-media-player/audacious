/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#include <fstream>
#include <unistd.h>
#include <math.h>

#include "modplugbmp.h"
#include <libmodplug/stdafx.h>
#include <libmodplug/sndfile.h>
#include "stddefs.h"
#include "archive/open.h"

// ModplugXMMS member functions ===============================

// operations ----------------------------------------
ModplugXMMS::ModplugXMMS()
{
	mSoundFile = new CSoundFile;
}
ModplugXMMS::~ModplugXMMS()
{
	delete mSoundFile;
}

ModplugXMMS::Settings::Settings()
{
	mSurround       = true;
	mOversamp       = true;
	mReverb         = false;
	mMegabass       = false;
	mNoiseReduction = true;
	mVolumeRamp     = true;
	mFastinfo       = true;
	mUseFilename    = false;

	mChannels       = 2;
	mFrequency      = 44100;
	mBits           = 16;
	mResamplingMode = SRCMODE_POLYPHASE;

	mReverbDepth    = 30;
	mReverbDelay    = 100;
	mBassAmount     = 40;
	mBassRange      = 30;
	mSurroundDepth  = 20;
	mSurroundDelay  = 20;
	
	mPreamp         = false;
	mPreampLevel    = 0.0f;
	
	mLoopCount      = 0;   //don't loop
}

void ModplugXMMS::Init(void)
{
	fstream lConfigFile;
	string lField, lValue;
	string lConfigFilename;
	bool lValueB;
	char junk;
	
	//I chose to use a separate config file to avoid conflicts
	lConfigFilename = g_get_home_dir();
	lConfigFilename += "/.audacious/modplug-bmp.conf";
	lConfigFile.open(lConfigFilename.c_str(), ios::in);

	if(!lConfigFile.is_open())
		return;

	while(!lConfigFile.eof())
	{
		lConfigFile >> lField;
		if(lField[0] == '#')      //allow comments
		{
			do
			{
				lConfigFile.read(&junk, 1);
			}
			while(junk != '\n');
		}
		else
		{
			if(lField == "reverb_depth")
				lConfigFile >> mModProps.mReverbDepth;
			else if(lField == "reverb_delay")
				lConfigFile >> mModProps.mReverbDelay;
			else if(lField == "megabass_amount")
				lConfigFile >> mModProps.mBassAmount;
			else if(lField == "megabass_range")
				lConfigFile >> mModProps.mBassRange;
			else if(lField == "surround_depth")
				lConfigFile >> mModProps.mSurroundDepth;
			else if(lField == "surround_delay")
				lConfigFile >> mModProps.mSurroundDelay;
			else if(lField == "preamp_volume")
				lConfigFile >> mModProps.mPreampLevel;
			else if(lField == "loop_count")
				lConfigFile >> mModProps.mLoopCount;
      else
			{
				lConfigFile >> lValue;
				if(lValue == "on")
					lValueB = true;
				else
					lValueB = false;

				if(lField == "surround")
					mModProps.mSurround         = lValueB;
				else if(lField == "oversampling")
					mModProps.mOversamp         = lValueB;
				else if(lField == "reverb")
					mModProps.mReverb           = lValueB;
				else if(lField == "megabass")
					mModProps.mMegabass         = lValueB;
				else if(lField == "noisereduction")
					mModProps.mNoiseReduction   = lValueB;
				else if(lField == "volumeramping")
					mModProps.mVolumeRamp       = lValueB;
				else if(lField == "fastinfo")
					mModProps.mFastinfo         = lValueB;
				else if(lField == "use_filename")
					mModProps.mUseFilename      = lValueB;
				else if(lField == "preamp")
					mModProps.mPreamp           = lValueB;

				else if(lField == "channels")
				{
					if(lValue == "mono")
						mModProps.mChannels       = 1;
					else
						mModProps.mChannels       = 2;
				}
				else if(lField == "frequency")
				{
						if(lValue == "22050")
					mModProps.mFrequency        = 22050;
					else if(lValue == "11025")
						mModProps.mFrequency      = 11025;
					else
						mModProps.mFrequency      = 44100;
				}
				else if(lField == "bits")
				{
					if(lValue == "8")
						mModProps.mBits           = 8;
					else
						mModProps.mBits           = 16;
				}
				else if(lField == "resampling")
				{
					if(lValue == "nearest")
						mModProps.mResamplingMode = SRCMODE_NEAREST;
					else if(lValue == "linear")
						mModProps.mResamplingMode = SRCMODE_LINEAR;
					else if(lValue == "spline")
						mModProps.mResamplingMode = SRCMODE_SPLINE;
					else
						mModProps.mResamplingMode = SRCMODE_POLYPHASE;
				}
			} //if(numerical value) else
		}   //if(comment) else
	}     //while(!eof)

	lConfigFile.close();
}

bool ModplugXMMS::CanPlayFile(const string& aFilename)
{
	string lExt;
	uint32 lPos;

	lPos = aFilename.find_last_of('.');
	if((int)lPos == -1)
		return false;
	lExt = aFilename.substr(lPos);
	for(uint32 i = 0; i < lExt.length(); i++)
		lExt[i] = tolower(lExt[i]);

	if (lExt == ".669")
		return true;
	if (lExt == ".amf")
		return true;
	if (lExt == ".ams")
		return true;
	if (lExt == ".dbm")
		return true;
	if (lExt == ".dbf")
		return true;
	if (lExt == ".dsm")
		return true;
	if (lExt == ".far")
		return true;
	if (lExt == ".it")
		return true;
	if (lExt == ".mdl")
		return true;
	if (lExt == ".med")
		return true;
	if (lExt == ".mod")
		return true;
	if (lExt == ".mtm")
		return true;
	if (lExt == ".okt")
		return true;
	if (lExt == ".ptm")
		return true;
	if (lExt == ".s3m")
		return true;
	if (lExt == ".stm")
		return true;
	if (lExt == ".ult")
		return true;
	if (lExt == ".umx")      //Unreal rocks!
		return true;
	if (lExt == ".xm")
		return true;
	if (lExt == ".j2b")
		return true;
	if (lExt == ".mt2")
		return true;
	if (lExt == ".psm")
		return true;

	if (lExt == ".mdz")
		return true;
	if (lExt == ".mdr")
		return true;
	if (lExt == ".mdgz")
		return true;
	if (lExt == ".mdbz")
		return true;
	if (lExt == ".s3z")
		return true;
	if (lExt == ".s3r")
		return true;
	if (lExt == ".s3gz")
		return true;
	if (lExt == ".xmz")
		return true;
	if (lExt == ".xmr")
		return true;
	if (lExt == ".xmgz")
		return true;
	if (lExt == ".itz")
		return true;
	if (lExt == ".itr")
		return true;
	if (lExt == ".itgz")
		return true;
	if (lExt == ".dmf")
		return true;
	
	if (lExt == ".zip")
		return ContainsMod(aFilename);
	if (lExt == ".gz")
		return ContainsMod(aFilename);
	if (lExt == ".bz2")
		return ContainsMod(aFilename);

	return false;
}

void* ModplugXMMS::PlayThread(void* arg)
{
	((ModplugXMMS*)arg)->PlayLoop();
	return NULL;
}

void ModplugXMMS::PlayLoop()
{
	uint32 lLength;
	//the user might change the number of channels while playing.
	// we don't want this to take effect until we are done!
	uint8 lChannels = mModProps.mChannels;

	while(!mStopped)
	{
		if(!(lLength = mSoundFile->Read(
				mBuffer,
				mBufSize)))
		{
			//no more to play.  Wait for output to finish and then stop.
			while((mOutPlug->buffer_playing())
			   && (!mStopped))
				usleep(10000);
			break;
		}
		
		if(mModProps.mPreamp)
		{
			//apply preamp
			if(mModProps.mBits == 16)
			{
				uint n = mBufSize >> 1;
				for(uint i = 0; i < n; i++) {
					short old = ((short*)mBuffer)[i];
					((short*)mBuffer)[i] *= mPreampFactor;
					// detect overflow and clip!
					if ((old & 0x8000) != 
					 (((short*)mBuffer)[i] & 0x8000))
					  ((short*)mBuffer)[i] = old | 0x7FFF;
						
				}
			}
			else
			{
				for(uint i = 0; i < mBufSize; i++) {
					uchar old = ((uchar*)mBuffer)[i];
					((uchar*)mBuffer)[i] *= mPreampFactor;
					// detect overflow and clip!
					if ((old & 0x80) != 
					 (((uchar*)mBuffer)[i] & 0x80))
					  ((uchar*)mBuffer)[i] = old | 0x7F;
				}
			}
		}
		
		if(mStopped)
			break;
	
		//wait for buffer space to free up.
		while(((mOutPlug->buffer_free()
		    < (int)mBufSize))
		   && (!mStopped))
			usleep(10000);
			
		if(mStopped)
			break;
		
		mOutPlug->write_audio
		(
			mBuffer,
			mBufSize
		);
		mInPlug->add_vis_pcm
		(
			mPlayed,
			mFormat,
			lChannels,
			mBufSize,
			mBuffer
		);

		mPlayed += mBufTime;
	}

//	mOutPlug->flush(0);
	mOutPlug->close_audio();

	//Unload the file
	mSoundFile->Destroy();
	delete mArchive;

	if (mBuffer)
	{
		delete [] mBuffer;
		mBuffer = NULL;
	}

	mPaused = false;
	mStopped = true;

	g_thread_exit(NULL);
}

void ModplugXMMS::PlayFile(const string& aFilename)
{
	mStopped = true;
	mPaused = false;
	
	//open and mmap the file
	mArchive = OpenArchive(aFilename);
	if(mArchive->Size() == 0)
	{
		delete mArchive;
		return;
	}
	
	if (mBuffer)
		delete [] mBuffer;
	
	//find buftime to get approx. 512 samples/block
	mBufTime = 512000 / mModProps.mFrequency + 1;

	mBufSize = mBufTime;
	mBufSize *= mModProps.mFrequency;
	mBufSize /= 1000;    //milliseconds
	mBufSize *= mModProps.mChannels;
	mBufSize *= mModProps.mBits / 8;

	mBuffer = new uchar[mBufSize];
	if(!mBuffer)
		return;             //out of memory!

	CSoundFile::SetWaveConfig
	(
		mModProps.mFrequency,
		mModProps.mBits,
		mModProps.mChannels
	);
	CSoundFile::SetWaveConfigEx
	(
		mModProps.mSurround,
		!mModProps.mOversamp,
		mModProps.mReverb,
		true,
		mModProps.mMegabass,
		mModProps.mNoiseReduction,
		false
	);
	
	// [Reverb level 0(quiet)-100(loud)], [delay in ms, usually 40-200ms]
	if(mModProps.mReverb)
	{
		CSoundFile::SetReverbParameters
		(
			mModProps.mReverbDepth,
			mModProps.mReverbDelay
		);
	}
	// [XBass level 0(quiet)-100(loud)], [cutoff in Hz 10-100]
	if(mModProps.mMegabass)
	{
		CSoundFile::SetXBassParameters
		(
			mModProps.mBassAmount,
			mModProps.mBassRange
		);
	}
	// [Surround level 0(quiet)-100(heavy)] [delay in ms, usually 5-40ms]
	if(mModProps.mSurround)
	{
		CSoundFile::SetSurroundParameters
		(
			mModProps.mSurroundDepth,
			mModProps.mSurroundDelay
		);
	}
	CSoundFile::SetResamplingMode(mModProps.mResamplingMode);
	mSoundFile->SetRepeatCount(mModProps.mLoopCount);
	mPreampFactor = exp(mModProps.mPreampLevel);
	
	mPaused = false;
	mStopped = false;

	mSoundFile->Create
	(
		(uchar*)mArchive->Map(),
		mArchive->Size()
	);
	mPlayed = 0;
	
	bool useFilename = mModProps.mUseFilename;
	
	if(!useFilename)
	{
		strncpy(mModName, mSoundFile->GetTitle(), 100);
		
		for(int i = 0; mModName[i] == ' ' || mModName[i] == 0; i++)
		{
			if(mModName[i] == 0)
			{
				useFilename = true;  //mod name is blank -- use filename
				break;
			}
		}
	}
	
	if(useFilename)
	{
		strncpy(mModName, strrchr(aFilename.c_str(), '/') + 1, 100);
		char* ext = strrchr(mModName, '.');
		if(ext) *ext = '\0';
	}
	
	mInPlug->set_info
	(
		mModName,
		mSoundFile->GetSongTime() * 1000,
		mSoundFile->GetNumChannels(),
		mModProps.mFrequency / 1000,
		mModProps.mChannels
	);

	mStopped = mPaused = false;

	if(mModProps.mBits == 16)
		mFormat = FMT_S16_NE;
	else
		mFormat = FMT_U8;

	mOutPlug->open_audio
	(
		mFormat,
		mModProps.mFrequency,
		mModProps.mChannels
	);

	mDecodeThread = g_thread_create(
		(GThreadFunc)PlayThread,
		this,
		TRUE,
		NULL
	);
}

void ModplugXMMS::Stop(void)
{
	if(mStopped)
		return;

	mStopped = true;
	mPaused = false;
	
	g_thread_join(mDecodeThread);
}

void ModplugXMMS::Pause(bool aPaused)
{
	if(aPaused)
		mPaused = true;
	else
		mPaused = false;
	
	mOutPlug->pause(aPaused);
}

void ModplugXMMS::Seek(float32 aTime)
{
	uint32  lMax;
	uint32  lMaxtime;
	float32 lPostime;
	
	if(aTime > (lMaxtime = mSoundFile->GetSongTime()))
		aTime = lMaxtime;
	lMax = mSoundFile->GetMaxPosition();
	lPostime = float(lMax) / lMaxtime;

	mSoundFile->SetCurrentPos(int(aTime * lPostime));

	mOutPlug->flush(int(aTime * 1000));
	mPlayed = uint32(aTime * 1000);
}

float32 ModplugXMMS::GetTime(void)
{
	if(mStopped)
		return -1;
	return (float32)mOutPlug->output_time() / 1000;
}

void ModplugXMMS::GetSongInfo(const string& aFilename, char*& aTitle, int32& aLength)
{
	aLength = -1;
	fstream lTestFile;
	string lError;
	bool lDone;
	
	lTestFile.open(aFilename.c_str(), ios::in);
	if(!lTestFile)
	{
		lError = "**no such file: ";
		lError += strrchr(aFilename.c_str(), '/') + 1;
		aTitle = new char[lError.length() + 1];
		strcpy(aTitle, lError.c_str());
		return;
	}
	
	lTestFile.close();

	if(mModProps.mFastinfo)
	{
		if(mModProps.mUseFilename)
		{
			//Use filename as name
			aTitle = new char[aFilename.length() + 1];
			strcpy(aTitle, strrchr(aFilename.c_str(), '/') + 1);
			*strrchr(aTitle, '.') = '\0';
			return;
		}
		
		fstream lModFile;
		string lExt;
		uint32 lPos;
		
		lDone = true;

		// previously ios::nocreate was used (X Standard C++ Library)
		lModFile.open(aFilename.c_str(), ios::in);

		lPos = aFilename.find_last_of('.');
		if((int)lPos == 0)
			return;
		lExt = aFilename.substr(lPos);
		for(uint32 i = 0; i < lExt.length(); i++)
			lExt[i] = tolower(lExt[i]);

		if (lExt == ".mod")
		{
			lModFile.read(mModName, 20);
			mModName[20] = 0;
		}
		else if (lExt == ".s3m")
		{
			lModFile.read(mModName, 28);
			mModName[28] = 0;
		}
		else if (lExt == ".xm")
		{
			lModFile.seekg(17);
			lModFile.read(mModName, 20);
			mModName[20] = 0;
		}
		else if (lExt == ".it")
		{
			lModFile.seekg(4);
			lModFile.read(mModName, 28);
			mModName[28] = 0;
		}
		else
			lDone = false;     //fall back to slow info

		lModFile.close();

		if(lDone)
		{
			for(int i = 0; mModName[i] != 0; i++)
			{
				if(mModName[i] != ' ')
				{
					aTitle = new char[strlen(mModName) + 1];
					strcpy(aTitle, mModName);
					
					return;
				}
			}
			
			//mod name is blank.  Use filename instead.
			aTitle = new char[aFilename.length() + 1];
			strcpy(aTitle, strrchr(aFilename.c_str(), '/') + 1);
			*strrchr(aTitle, '.') = '\0';
			return;
		}
	}
		
	Archive* lArchive;
	CSoundFile* lSoundFile;
	const char* lTitle;

	//open and mmap the file
	lArchive = OpenArchive(aFilename);
	if(lArchive->Size() == 0)
	{
		lError = "**bad mod file: ";
		lError += strrchr(aFilename.c_str(), '/') + 1;
		aTitle = new char[lError.length() + 1];
		strcpy(aTitle, lError.c_str());
		delete lArchive;
		return;
	}

	lSoundFile = new CSoundFile;
	lSoundFile->Create((uchar*)lArchive->Map(), lArchive->Size());

	if(!mModProps.mUseFilename)
	{
		lTitle = lSoundFile->GetTitle();
		
		for(int i = 0; lTitle[i] != 0; i++)
		{
			if(lTitle[i] != ' ')
			{
				aTitle = new char[strlen(lTitle) + 1];
				strcpy(aTitle, lTitle);
				goto therest;     //sorry
			}
		}
	}
	
	//mod name is blank, or user wants the filename to be used as the title.
	aTitle = new char[aFilename.length() + 1];
	strcpy(aTitle, strrchr(aFilename.c_str(), '/') + 1);
	*strrchr(aTitle, '.') = '\0';

therest:
	aLength = lSoundFile->GetSongTime() * 1000;                   //It wants milliseconds!?!

	//unload the file
	lSoundFile->Destroy();
	delete lSoundFile;
	delete lArchive;
}

void ModplugXMMS::SetInputPlugin(InputPlugin& aInPlugin)
{
	mInPlug = &aInPlugin;
}
void ModplugXMMS::SetOutputPlugin(OutputPlugin& aOutPlugin)
{
	mOutPlug = &aOutPlugin;
}

const ModplugXMMS::Settings& ModplugXMMS::GetModProps()
{
	return mModProps;
}

const char* ModplugXMMS::Bool2OnOff(bool aValue)
{
	if(aValue)
		return "on";
	else
		return "off";
}

void ModplugXMMS::SetModProps(const Settings& aModProps)
{
	fstream lConfigFile;
	string lConfigFilename;
	
	mModProps = aModProps;

	// [Reverb level 0(quiet)-100(loud)], [delay in ms, usually 40-200ms]
	if(mModProps.mReverb)
	{
		CSoundFile::SetReverbParameters
		(
			mModProps.mReverbDepth,
			mModProps.mReverbDelay
		);
	}
	// [XBass level 0(quiet)-100(loud)], [cutoff in Hz 10-100]
	if(mModProps.mMegabass)
	{
		CSoundFile::SetXBassParameters
		(
			mModProps.mBassAmount,
			mModProps.mBassRange
		);
	}
	else //modplug seems to ignore the SetWaveConfigEx() setting for bass boost
	{
		CSoundFile::SetXBassParameters
		(
			0,
			0
		);
	}
	// [Surround level 0(quiet)-100(heavy)] [delay in ms, usually 5-40ms]
	if(mModProps.mSurround)
	{
		CSoundFile::SetSurroundParameters
		(
			mModProps.mSurroundDepth,
			mModProps.mSurroundDelay
		);
	}
	CSoundFile::SetWaveConfigEx
	(
		mModProps.mSurround,
		!mModProps.mOversamp,
		mModProps.mReverb,
		true,
		mModProps.mMegabass,
		mModProps.mNoiseReduction,
		false
	);
	CSoundFile::SetResamplingMode(mModProps.mResamplingMode);
	mPreampFactor = exp(mModProps.mPreampLevel);

	lConfigFilename = g_get_home_dir();
	lConfigFilename += "/.audacious/modplug-bmp.conf";
	lConfigFile.open(lConfigFilename.c_str(), ios::out);

	lConfigFile << "# Modplug BMP plugin config file\n"
	            << "# Modplug (C) 1999 Olivier Lapicque\n"
	            << "# XMMS port (C) 1999 Kenton Varda\n" 
		    << "# BMP port (C) 2004 Theofilos Intzoglou" << endl;

	lConfigFile << "# ---Effects---"  << endl;
	lConfigFile << "reverb          " << Bool2OnOff(mModProps.mReverb)         << endl;
	lConfigFile << "reverb_depth    " << mModProps.mReverbDepth                << endl;
	lConfigFile << "reverb_delay    " << mModProps.mReverbDelay                << endl;
	lConfigFile << endl;
	lConfigFile << "surround        " << Bool2OnOff(mModProps.mSurround)       << endl;
	lConfigFile << "surround_depth  " << mModProps.mSurroundDepth              << endl;
	lConfigFile << "surround_delay  " << mModProps.mSurroundDelay              << endl;
	lConfigFile << endl;
	lConfigFile << "megabass        " << Bool2OnOff(mModProps.mMegabass)       << endl;
	lConfigFile << "megabass_amount " << mModProps.mBassAmount                 << endl;
	lConfigFile << "megabass_range  " << mModProps.mBassRange                  << endl;
	lConfigFile << endl;
	lConfigFile << "oversampling    " << Bool2OnOff(mModProps.mOversamp)       << endl;
	lConfigFile << "noisereduction  " << Bool2OnOff(mModProps.mNoiseReduction) << endl;
	lConfigFile << "volumeramping   " << Bool2OnOff(mModProps.mVolumeRamp)     << endl;
	lConfigFile << "fastinfo        " << Bool2OnOff(mModProps.mFastinfo)       << endl;
	lConfigFile << "use_filename    " << Bool2OnOff(mModProps.mUseFilename)    << endl;
	lConfigFile << "loop_count      " << mModProps.mLoopCount                  << endl;
	lConfigFile << endl;
	lConfigFile << "preamp          " << Bool2OnOff(mModProps.mPreamp)         << endl;
	lConfigFile << "preamp_volume   " << mModProps.mPreampLevel                << endl;
	lConfigFile << endl;

	lConfigFile << "# ---Quality---" << endl;
	lConfigFile << "channels        ";
	if(mModProps.mChannels == 1)
		lConfigFile << "mono" << endl;
	else
		lConfigFile << "stereo" << endl;
	lConfigFile << "bits            " << (int)mModProps.mBits << endl;
	lConfigFile << "frequency       " << mModProps.mFrequency << endl;
	lConfigFile << "resampling      ";
	switch(mModProps.mResamplingMode)
	{
	case SRCMODE_NEAREST:
		lConfigFile << "nearest" << endl;
		break;
	case SRCMODE_LINEAR:
		lConfigFile << "linear" << endl;
		break;
	case SRCMODE_SPLINE:
		lConfigFile << "spline" << endl;
		break;
	default:
	case SRCMODE_POLYPHASE:
		lConfigFile << "fir" << endl;
		break;
	};

	lConfigFile.close();
}

ModplugXMMS gModplugXMMS;
