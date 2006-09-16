/* Modplug XMMS Plugin
 * Authors: Kenton Varda <temporal@gauge3d.org>
 *
 * This source code is public domain.
 */

#include "archive.h"



///* Open a read pipe */ File f;
//f=popen("gzip -d compressed.file.name","r");
///* Read some data in the usual manner, for example */
//fscanf(f,"%d %f %f",name,&age,$id);
///* Close the pipe */
//pclose(f); 

Archive::~Archive()
{
}

bool Archive::IsOurFile(const string& aFileName)
{
	string lExt;
	uint32 lPos;

	lPos = aFileName.find_last_of('.');
	if((int)lPos == -1)
		return false;
	lExt = aFileName.substr(lPos);
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
	
	return false;
}
