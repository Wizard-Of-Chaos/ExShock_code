#pragma once
#ifndef AUDIODRIVER_H
#define AUDIODRIVER_H
#include "BaseHeader.h"
#include "AudioBuffer.h"
#include "AudioSource.h"

/*
* The audio driver class does what you think it does and handles the audio for the game itself, including the loading of files, playing of audio,
* and management of various sound sources within a scene. It keeps track of anything that is currently making noise in the game, be that a menu sound
* effect, the music, or in-game effects.
*/
class AudioDriver
{
	public:
		AudioDriver(u32 maxTracks=1);

		//void audioUpdate();
		static const u32 MAX_TRACKS = 8;
		//This plays a sound from the given source in the game and registers the source. Returns the source if you need to track it.
		AudioSource* playGameSound(flecs::entity ent, std::string fname, f32 gain=1.f, f32 refDist=20.f, f32 maxDist=1200.f, bool loop=false, bool disablePitchFuzz=false);
		AudioSource* playGameSound(vector3df position, std::string fname, f32 gain = 1.f, f32 refDist = 20.f, f32 maxDist = 1200.f, bool loop = false, bool disablePitchFuzz = false);
		//Plays a menu sound effect.
		void playMenuSound(std::string fname);
		//Plays music. Will halt any present music.
		void playMusic(std::string fname, u32 track = 0);
		void stopMusic(u32 track = 0);
		//Sets the gain on the music track.
		void setMusicGain(u32 track, f32 gain);
		
		//Fades out the first track, and then fades in the second track.
		void fadeTracks(u32 trackToFade, u32 trackToGain, f32 timeToFade=1.f, f32 fadingTrackGainTarget=0.f, f32 gainingTrackGainTarget=1.f);
		void clearFades();
		//Sets the listener position, including up values and forward velocity.
		void setListenerPosition(vector3df pos, vector3df up = vector3df(0,1,0), vector3df forward = vector3df(0,0,-1), btVector3 vel = btVector3(0,0,0));
		//Wipes the data buffer for in-game sound effects.
		void cleanupGameSounds();
		struct _SoundInstance {
			flecs::entity id;
			AudioSource* src;
		};
		std::list<_SoundInstance> curGameSounds;
		std::list<AudioSource*> curMenuSounds;
		void menuSoundUpdate(bool inGame=false);
		void ingameMusicUpdate(f32 dt);
		void setGlobalGains(f32 master, f32 music, f32 game, f32 menu);
	private:
		void m_updateGains();
		std::unordered_map<std::string, ALuint> loadedGameSounds;
		std::unordered_map<std::string, ALuint> loadedMenuSounds;

		AudioBuffer gameSounds;
		AudioBuffer menuSounds;

		ALuint currentMusic[MAX_TRACKS];
		AudioSource* musicSources[MAX_TRACKS]; //should always be on top of the listener
		f32 musicGains[MAX_TRACKS];

		ALCcontext* context;
		ALCdevice* device;

		u32 simultaneousTracks = 1;

		f32 masterGain = 1.f;
		f32 globalMusicGain = 1.f;
		f32 gameGain = 1.f;
		f32 menuGain = 1.f;

		struct _trackfade {
			u32 fadingTrack = 0;
			f32 fadingTrackGainTarget = 0.f;
			u32 gainingTrack = 0;
			f32 gainingTrackGainTarget = 0.f;
			f32 timeToFade = 0.f;
			f32 elapsedTime = 0.f;
			bool halftime = false;
			const f32 getFade() const  {
				return std::max(0.f, (1.f - (elapsedTime / timeToFade)));
			}
			const f32 getGain() const {
				return std::min(1.f, (elapsedTime / timeToFade) * gainingTrackGainTarget);
			}
		};

		std::list<_trackfade> fades;
};

#endif