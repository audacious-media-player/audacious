/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

//open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "arch_zip.h"
#include <iostream>
#include <vector>
	
arch_Zip::arch_Zip(const string& aFileName)
{
	//check if file exists
	int lFileDesc = open(aFileName.c_str(), O_RDONLY);
	string lGoodName;
	bool bFound = false;

	if(lFileDesc == -1)
	{
		mSize = 0;
		return;
	}
	
	close(lFileDesc);
	
	// file exists.
	string lCommand = "unzip -l -qq \"" + aFileName + '\"';   //get info
	FILE *f = popen(lCommand.c_str(), "r");

	if(f <= 0)
	{
		mSize = 0;
		return;
	}
	

	bool eof = false;
	while(!eof)
	{
		char line[301];
		char lUncompName[300];
		if (fgets(line, 300, f) <= 0) 
		{
			eof = true;
			break;
		}
		
		if (processLine(line, &mSize, lUncompName))
		{
			lGoodName = lUncompName;
			bFound = true;
			break;
		}
		
	}
	if(!bFound)
	{
		mSize = 0;
		return;
	}

	pclose(f);
	
	mMap = new char[mSize];
	
	lCommand = "unzip -p \"" + aFileName + "\" \"" + lGoodName + '\"';
	//decompress to stdout
	f = popen(lCommand.c_str(), "r");
	
	if (f <= 0)
	{
		mSize = 0;
		return;
	}

	fread((char *)mMap, sizeof(char), mSize, f);
	
	pclose(f);
	
}

bool arch_Zip::processLine(char *buffer, uint32 *mSize, char *filename)
{
	uint32 mlSize;
	mlSize = 0;

	if (sscanf(buffer, "%u %*s %*s %s\n", &mlSize, &filename[0]) <= 0)
		return false;
		
	*mSize = mlSize;
	string lName = filename;

	// size date time name
	return IsOurFile(lName);	
}

arch_Zip::~arch_Zip()
{
	if(mSize != 0)
		delete [] (char*)mMap;
}
	
bool arch_Zip::ContainsMod(const string& aFileName)
{
	int lFileDesc = open(aFileName.c_str(), O_RDONLY);

	if(lFileDesc == -1)
		return false;
	
	close(lFileDesc);
	
	// file exists.
	string lCommand = "unzip -l -qq \"" + aFileName + '\"';   //get info
	FILE *f = popen(lCommand.c_str(), "r");

	if(f <= 0)
		return false;
	
	bool eof = false;
	while(!eof)
	{
		char line[301], lName[300];
		if (fgets(line, 300, f) <= 0) 
			return false;
		
		uint32 tempSize;
		// Simplification of previous if statement
		pclose(f);
		return (processLine(line, &tempSize, lName));
		
	}

	pclose(f);	
	return false;
}
