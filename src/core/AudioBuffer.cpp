#include "AudioBuffer.h"
#include <fstream>
#include "CrashLogger.h"

static ALuint loadOgg(const char* path)
{
	ALenum error = 0;
	ALuint sound = 0;
	FILE* fp = 0;
	OggVorbis_File vf;
	vorbis_info* vi = 0;
	ALenum format = 0;
	short* pcmout = 0;

	size_t dataLength;

	fp = fopen(path, "rb");
	if (!fp) {
		std::cerr << "Could not open file: " << path << std::endl;
		return 0;
	}
	alGenBuffers(1, &sound);
	error = alGetError();
	if (error != AL_NO_ERROR) {
		std::cerr << "Error creating buffer: " << path << ", buffer=" << sound << ", error=" << error << std::endl;
		fclose(fp);
		return 0;
	}

	if (ov_open_callbacks(fp, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0) {
		std::cerr << "Stream is not a valid OggVorbis stream.\n";
		goto fail;
	}

	vi = ov_info(&vf, -1);
	format = vi->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

	dataLength = ov_pcm_total(&vf, -1) * vi->channels * 2;
	pcmout = (short*)malloc(dataLength);
	if (pcmout == 0) {
		std::cerr << "Out of memory. What the hell are you doing?\n";
		goto fail;
	}
	for (size_t size = 0, offset = 0, sel = 0;
		(size = ov_read(&vf, (char*)pcmout + offset, 4096, 0, 2, 1, (int*)&sel)) != 0;
		offset += size) {
		if (size < 0) {
			std::cerr << "This ogg file is faulty.\n";
		}
	}
	alBufferData(sound, format, pcmout, dataLength, (ALsizei)vi->rate);
	error = alGetError();
	if (error != AL_NO_ERROR) {
		std::cerr << "Failed to send audio info to OpenAL.\n";
		goto fail;
	}

	fclose(fp);
	ov_clear(&vf);
	free(pcmout);
	return sound;

fail:
	std::cerr << "Could not load .ogg file.\n";
	fclose(fp);
	ov_clear(&vf);
	free(pcmout);
	return 0;
}

ALuint AudioBuffer::loadAudio(std::string fname)
{
	if (buffers.find(fname) != buffers.end()) return buffers[fname];

	ALuint sound = loadOgg(fname.c_str());
	if (sound == 0) {
		baedsLogger::errLog("Could not load " + fname + "!\n");
		return sound;
	}
	baedsLogger::log("Loaded " + fname + "\n");
	buffers[fname] = sound;
	return sound;
}
bool AudioBuffer::removeAudio(const ALuint& buf)
{
	std::string key = "";
	for (auto& [str, val] : buffers) {
		if (val == buf) {
			alGetError();
			alDeleteBuffers(1, &val);
			auto err = alGetError();
			if (err != AL_NO_ERROR) {
				baedsLogger::errLog("Something went long on removing a buffer: ALError " + std::to_string(err) + "\n");
			}
		}
		key = str;
		break;
	}
	if (key == "") return false;

	buffers.erase(key);
	return true;
}
void AudioBuffer::removeAllAudio()
{
	for (auto& [key, val] : buffers) {
		alDeleteBuffers(1, &val);
	}
	buffers.clear();
}