#pragma once
#ifndef AUDIOSOURCE_H
#define AUDIOSOURCE_H
#include "BaseHeader.h"

/*
* An audio source is a wrapper class around an actual audio source as specified by the OpenAL docs. If you want a thorough description of what
* exactly a source is, go read up there, but for our purposes an audio source represents a single sound that is currently playing. Emphasis on SINGLE-
* multiple sounds cannot play concurrently through one source. It includes several helpful functions to adjust values about the sound like
* pitch, max distance, and whether or not it's looping.
*/
class AudioSource
{
	public:
		AudioSource();
		~AudioSource();

		void play(const ALuint bufToPlay);
		void stop();

		void setPos(const vector3df pos);
		void setVel(const btVector3 vel);
		void setPitch(const f32 pitch);
		void setGain(const f32 gain);
		void setLoop(const bool loop);
		const bool isLooping();

		void setMaxDist(const f32 dist);
		void setRefDist(const f32 dist);

		bool isFinished();
	private:
		f32 m_pitch = 1.f;
		f32 m_gain = .7f;
		f32 m_maxDist = 100.f;
		f32 m_refDist = 10.f;
		f32 m_position[3] = { 0,0,0 };
		f32 m_velocity[3] = { 0,0,0 };
		f32 m_direction[3] = { 0,0,0 };
		bool m_loop = false;
		ALuint source; //the identifier of the source, do not touch this
		//a source has exactly ONE attached buffer - this means that a source plays ONE sound.
		ALuint buf = 0;
};

#endif 