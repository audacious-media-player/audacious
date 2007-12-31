/*
 * Copyright (c) 2008 William Pitcock <nenolod@sacredspiral.co.uk>
 *
 * [insert GPL license here later]
 */

#ifndef __AUDACIOUSXX__PLUGIN_H_GUARD
#define __AUDACIOUSXX__PLUGIN_H_GUARD

#include <string>

namespace Audacious {

class Plugin {
private:
	std::string name;
	std::string description;

public:
	Plugin(std::string name_, std::string description_);
	~Plugin();
};

class VisPlugin : Plugin {
private:
	int pcm_channels;
	int freq_channels;

public:
	VisPlugin(std::string name_, std::string description, int pc, int fc);
	~VisPlugin();
};

};

#endif
