#include "AudioDriver.h"
#include <iostream>
#include "IrrlichtComponent.h"
#include "BulletRigidBodyComponent.h"
#include "CrashLogger.h"

const std::string musicPath = "assets/audio/music/";
const std::string menuSoundPath = "assets/audio/menu/";
const std::string gameSoundPath = "assets/audio/game/";

AudioDriver::AudioDriver(u32 maxTracks)
{
	simultaneousTracks = maxTracks;
	device = alcOpenDevice(nullptr);
	if (device) {
		context = alcCreateContext(device, nullptr);
		if (context) {
			alcMakeContextCurrent(context);
		}
	}
	const ALCchar* name = nullptr;
	if (alcIsExtensionPresent(device, "ALC_ENUMERATE_ALL_EXT"))
		name = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
	if (!name || alcGetError(device) != AL_NO_ERROR)
		name = alcGetString(device, ALC_DEVICE_SPECIFIER);

	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
	alSpeedOfSound(600.f); //This is space. I do what I want.
	alDopplerFactor(.5f);

	baedsLogger::log("Opened audio device: " + std::string(name) + "\n");

	for (u32 i = 0; i < maxTracks; ++i) {
		musicSources[i] = new AudioSource;
		musicSources[i]->setGain(globalMusicGain);
		musicSources[i]->setLoop(true);
		musicGains[i] = 1.f;
	}

	for (u32 i = maxTracks; i < MAX_TRACKS; ++i) {
		musicSources[i] = nullptr;
		musicGains[i] = 0.f;
	}
}

AudioSource* AudioDriver::playGameSound(flecs::entity ent, std::string fname, f32 gain, f32 refDist, f32 maxDist, bool loop, bool disablePitchFuzz)
{
	auto irr = ent.get<IrrlichtComponent>();
	if (!irr) {
		return nullptr;
	}
	vector3df srcPos = irr->node->getAbsolutePosition();

	ALfloat lPos[3] = { 0.f, 0.f, 0.f };
	alGetListener3f(AL_POSITION, &lPos[0], &lPos[1], &lPos[2]);
	vector3df listener(lPos[0], lPos[1], -lPos[2]);

	if (std::abs((srcPos - listener).getLength()) >= 1200.f) {
		return nullptr; //returns a null if the sound is more than a kilometer away
	}
	ALuint buf = 0;
	if (loadedGameSounds.find(fname) != loadedGameSounds.end()) {
		buf = loadedGameSounds.at(fname);
	} else {
		buf = gameSounds.loadAudio(gameSoundPath + fname);
		if (buf == 0) return nullptr;
		loadedGameSounds[fname] = buf;
	}
	//also needs to register the audio source
	_SoundInstance inst;
	inst.id = ent;
	inst.src = new AudioSource;
	auto rbc = ent.get<BulletRigidBodyComponent>();
	if (irr) {
		inst.src->setPos(srcPos);
	}
	if (rbc) {
		inst.src->setVel(rbc->rigidBody->getLinearVelocity());
	}
	
	inst.src->setRefDist(refDist);
	inst.src->setMaxDist(maxDist);
	inst.src->setGain(gain*gameGain);
	inst.src->setLoop(loop);

	if(!disablePitchFuzz) inst.src->setPitch(random.frange(.75f, 1.25f));

	inst.src->play(buf);
	curGameSounds.push_back(inst);
	return inst.src;
}

AudioSource* AudioDriver::playGameSound(vector3df position, std::string fname, f32 gain, f32 refDist, f32 maxDist, bool loop, bool disablePitchFuzz)
{
	vector3df srcPos = position;

	ALfloat lPos[3] = { 0.f, 0.f, 0.f };
	alGetListener3f(AL_POSITION, &lPos[0], &lPos[1], &lPos[2]);
	vector3df listener(lPos[0], lPos[1], -lPos[2]);

	if (std::abs((srcPos - listener).getLength()) >= 1200.f) {
		return nullptr; //returns a null if the sound is more than a kilometer away
	}
	ALuint buf = 0;
	if (loadedGameSounds.find(fname) != loadedGameSounds.end()) {
		buf = loadedGameSounds.at(fname);
	}
	else {
		buf = gameSounds.loadAudio(gameSoundPath + fname);
		if (buf == 0) return nullptr;
		loadedGameSounds[fname] = buf;
	}
	//also needs to register the audio source
	_SoundInstance inst;
	inst.id = INVALID_ENTITY;
	inst.src = new AudioSource;
	inst.src->setPos(srcPos);

	inst.src->setRefDist(refDist);
	inst.src->setMaxDist(maxDist);
	inst.src->setGain(gain * gameGain);
	inst.src->setLoop(loop);

	if (!disablePitchFuzz) inst.src->setPitch(random.frange(.75f, 1.25f));

	inst.src->play(buf);
	curGameSounds.push_back(inst);
	return inst.src;
}

void AudioDriver::playMenuSound(std::string fname)
{
	ALuint buf = 0;
	if (loadedMenuSounds.find(fname) != loadedMenuSounds.end()) {
		buf = loadedMenuSounds.at(fname);
	}
	else {
		buf = menuSounds.loadAudio(menuSoundPath + fname);
		if (buf == 0) return;
		loadedMenuSounds[fname] = buf;
	}
	AudioSource* src = new AudioSource;
	src->setGain(menuGain);
	src->play(buf);
	curMenuSounds.push_back(src);
	//menuSource->play(buf);
}

void AudioDriver::playMusic(std::string fname, u32 track)
{
	if (!musicSources[track]) {
		std::cerr << "Cannot play music on track " << track << ": above max track num\n";
		return;
	}
	if (currentMusic[track] != 0) {
		musicSources[track]->stop();
		menuSounds.removeAudio(currentMusic[track]);
	}

	auto music = menuSounds.loadAudio(musicPath + fname);
	currentMusic[track] = music;
	musicSources[track]->play(music);
}

void AudioDriver::stopMusic(u32 track)
{
	if (currentMusic[track] != 0) {
		musicSources[track]->stop();
		menuSounds.removeAudio(currentMusic[track]);
	}
}

void AudioDriver::setMusicGain(u32 track, f32 gain)
{
	musicGains[track] = gain;
	musicSources[track]->setGain(globalMusicGain * musicGains[track]);
}

void AudioDriver::cleanupGameSounds()
{
	setListenerPosition(vector3df(0, 0, 0));
	for (auto inst : curGameSounds) {
		inst.src->stop();
		delete inst.src;
	}
	curGameSounds.clear();

	gameSounds.removeAllAudio();
	loadedGameSounds.clear();
}

void AudioDriver::setListenerPosition(vector3df pos, vector3df up, vector3df forward, btVector3 vel)
{
	auto err = alGetError();
	ALfloat orient[] = {forward.X, forward.Y, -forward.Z, up.X, up.Y, -up.Z};
	alListener3f(AL_POSITION, pos.X, pos.Y, -pos.Z);
	alListener3f(AL_VELOCITY, vel.x(), vel.y(), -vel.z());
	alListenerfv(AL_ORIENTATION, orient);
	for (u32 i = 0; i < simultaneousTracks; ++i) {
		musicSources[i]->setPos(pos);
		musicSources[i]->setVel(vel);
	}
	for (auto src : curMenuSounds) {
		src->setPos(pos);
		src->setVel(vel);
	}
}

void AudioDriver::fadeTracks(u32 trackToFade, u32 trackToGain, f32 timeToFade, f32 fadingTrackGainTarget, f32 gainingTrackGainTarget)
{
	_trackfade fader = {trackToFade, fadingTrackGainTarget, trackToGain, gainingTrackGainTarget, timeToFade, 0.f};
	fades.push_back(fader);
}

void AudioDriver::clearFades()
{
	fades.clear();
}

void AudioDriver::menuSoundUpdate(bool inGame)
{
	auto it = curMenuSounds.begin();
	while (it != curMenuSounds.end()) {
		AudioSource* src = *it;
		if (src->isFinished()) {
			delete src;
			it = curMenuSounds.erase(it);
		}
		if (it == curMenuSounds.end()) break;

		++it;
	}
	if (!inGame) {
		setListenerPosition(vector3df(0, 0, 0));
		for (auto src : curMenuSounds) {
			src->setPos(vector3df(0,0,0));
			src->setVel(btVector3(0, 0, 0));
		}
	}
}

void AudioDriver::ingameMusicUpdate(f32 dt)
{
	if (!fades.empty()) {
		auto it = fades.begin();
		while (it != fades.end()) {
			it->elapsedTime += dt;
			bool done = false;
			if (it->halftime) {
				f32 fadeGain = it->getFade();
				if (fadeGain == 0.f) {
					done = true;
				}
				setMusicGain(it->fadingTrack, fadeGain);
			}
			else {
				f32 gain = it->getGain();
				if (gain == 1.f) {
					it->halftime = true;
					it->elapsedTime = 0.f; //restart
				}
				setMusicGain(it->gainingTrack, gain);
			}
			if (done) {
				it = fades.erase(it);
				continue;
			}
			++it;
		}
	}

}

void AudioDriver::setGlobalGains(f32 master, f32 music, f32 game, f32 menu)
{
	masterGain = master;
	globalMusicGain = music;
	gameGain = game;
	menuGain = menu;
	m_updateGains();
}

void AudioDriver::m_updateGains()
{
	auto err = alGetError();
	alListenerf(AL_GAIN, masterGain);
	for (u32 i = 0; i < simultaneousTracks; ++i) {
		musicSources[i]->setGain(globalMusicGain * musicGains[i]);
	}
	for (auto src : curMenuSounds) {
		src->setGain(menuGain);
	}
	for (auto snd : curGameSounds) {
		snd.src->setGain(gameGain);
	}
}