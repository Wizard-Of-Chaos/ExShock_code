#pragma once
#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H
#include "BaseHeader.h"

/*
* Audio buffers should absolutely never be seen outside of the base AudioDriver class. Their primary purpose is to both load and store
* .ogg files for use as game sounds or for music. They keep this data stored in an internal map so it can be re-called on demand.
* 
* There are a few audio buffers in use, one for game sounds and one for menu sounds. Sounds are pretty hefty, so the game sounds buffer gets wiped
* at the end of every scenario. The menu sound buffer should not be wiped.
*/
class AudioBuffer
{
public:
	ALuint loadAudio(std::string fname);
	bool removeAudio(const ALuint& buf);
	void removeAllAudio();
private:
	std::unordered_map<std::string, ALuint> buffers;
};

#endif 