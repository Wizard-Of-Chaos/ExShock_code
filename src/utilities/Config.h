#pragma once
#ifndef CONFIG_H
#define CONFIG_H
#include "BaseHeader.h"
#include "GvReader.h"
#include "InputComponent.h"
#include <unordered_map>
#include <vector>

const extern std::map <std::string, E_DRIVER_TYPE> drivers;
class GameStateController;

/*
* Holds the configuration for the game - things like fullscreen, the resolution, filtering, shadows, and other
* graphics stuff.
*/

enum class SETTING_LEVEL
{
	LOW,
	MED,
	HIGH,
	SETTINGLEVEL_MAX
};

enum class DIFFICULTY_LEVEL
{
	GAMES_JOURNALIST = 1,
	LOW = 2,
	NORMAL = 3,
	HIGH = 4,
	UNFAIR = 5,
	DIFFICULTY_MAXVAL = 6
};

enum VIDEO_TOGGLE
{
	TOG_FULLSCREEN,
	TOG_VSYNC,
	TOG_ALIASING,
	TOG_FILTER,
	TOG_ANISOTROPIC,
	TOG_BUMP,
	TOG_MAX
};

enum DIFFICULTY
{
	DIF_PLAYER_DMG,
	DIF_ENEMY_DMG,
	DIF_AI_INT,
	DIF_AIM,
	DIF_RESOLVE,
	DIF_WINGNUM,
	DIF_MAX
};

enum GAME_TOGGLE
{
	GTOG_LIN_SPACE_FRICTION,
	GTOG_ANG_SPACE_FRICTION,
	GTOG_CONST_THRUST,
	GTOG_FRIENDLY_FIRE,
	GTOG_IMPACT,
	GTOG_BANTER,
	GTOG_MAX
};

extern const std::map<DIFFICULTY, std::string> difficultyNames;
extern const std::map<DIFFICULTY_LEVEL, std::string> difLevelNames;

extern const std::map<DIFFICULTY_LEVEL, u32> aiBehaviorDifficulties;
extern const std::map<DIFFICULTY_LEVEL, f32> aiResolveDifficulties;
extern const std::map<DIFFICULTY_LEVEL, f32> aiAimDifficulties;
extern const std::map<DIFFICULTY_LEVEL, u32> aiNumDifficulties;

extern const std::map<std::string, VIDEO_TOGGLE> videoToggleNames;
extern const std::map<GAME_TOGGLE, std::string> gameToggleNames;

extern const std::map<EKEY_CODE, std::wstring> keyDesc;
extern const std::map<INPUT, std::wstring> inputNames;

//Returns the key description (i.e. a string for the key like "K") for the given input.
std::wstring getKeyDesc(INPUT in);
//Returns the key-code for the given input.
EKEY_CODE key(INPUT in);

struct VideoConfig
{
	VideoConfig();
	void loadConfig(std::string filename);
	void saveConfig(std::string filename);
	E_DRIVER_TYPE driver;
	SETTING_LEVEL particleLevel;
	SETTING_LEVEL renderDist;
	bool toggles[TOG_MAX];

	bool useScreenRes;
	int resX;
	int resY;
};

struct KeyConfig
{
	KeyConfig();
	void loadConfig(std::string filename);
	void saveConfig(std::string filename);
	const bool keyBound(EKEY_CODE which) const {
		for (u32 i = 0; i < IN_MAX_ENUM; ++i) {
			if (key[i] == which) return true;
		}
		return false;
	}
	const INPUT boundTo(EKEY_CODE which) const {
		if (!keyBound(which)) return IN_MAX_ENUM;
		for (u32 i = 0; i < IN_MAX_ENUM; ++i) {
			if (key[i] == which) return (INPUT)i;
		}
		return IN_MAX_ENUM;
	}
	EKEY_CODE key[IN_MAX_ENUM];
};

struct GameConfig
{
	GameConfig() {
		for (u32 i = 0; i < DIF_MAX; ++i) {
			difficulties[i] = DIFFICULTY_LEVEL::NORMAL;
		}
		for (u32 i = 0; i < GTOG_MAX; ++i) {
			u32 thr = (u32)GTOG_CONST_THRUST;
			if (i == thr) continue;
			toggles[i] = true;
		}
	}
	void loadConfig(std::string filename);
	void saveConfig(std::string filename);
	DIFFICULTY_LEVEL difficulties[DIF_MAX];
	bool toggles[GTOG_MAX];

	s32 volumes[4] = { 100, 100, 70, 70 };
	void setAudioGains();
};

struct Config
{
	VideoConfig vid;
	KeyConfig keys;
	GameConfig game;
};
#endif 