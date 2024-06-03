#include "Config.h"
#include "AIComponent.h"
#include "AudioDriver.h"
const std::map <std::string, E_DRIVER_TYPE> drivers =
{
	{"Direct3D9", EDT_DIRECT3D9},
	//{"Direct3D8", EDT_DIRECT3D8},
	{"OpenGL", EDT_OPENGL}
};
const std::map<std::string, VIDEO_TOGGLE> videoToggleNames =
{
	{"Vsync", TOG_VSYNC},
	{"Shadows", TOG_STENCILBUF},
	{"Antialiasing", TOG_ALIASING},
	{"Fullscreen", TOG_FULLSCREEN},
	{"Filtering", TOG_FILTER},
	{"Anisotropic", TOG_ANISOTROPIC},
	{"Bump-mapping", TOG_BUMP}
};

const std::map<DIFFICULTY, std::string> difficultyNames =
{
	{DIF_PLAYER_DMG, "Player Damage"},
	{DIF_ENEMY_DMG, "Enemy Damage"},
	{DIF_AI_INT, "AI Intelligence"},
	{DIF_AIM, "AI Aim"},
	{DIF_RESOLVE, "AI Cowardice"},
	{DIF_WINGNUM, "AI Wing Count"}
};

const std::map<DIFFICULTY_LEVEL, std::string> difLevelNames =
{
	{DIFFICULTY_LEVEL::GAMES_JOURNALIST, "Pathetic"},
	{DIFFICULTY_LEVEL::LOW, "Low"},
	{DIFFICULTY_LEVEL::NORMAL, "Normal"},
	{DIFFICULTY_LEVEL::HIGH, "High"},
	{DIFFICULTY_LEVEL::UNFAIR, "Unfair"}
};

const std::map<DIFFICULTY_LEVEL, u32> aiBehaviorDifficulties =
{
	{DIFFICULTY_LEVEL::GAMES_JOURNALIST, (AIBHVR_MORON | AIBHVR_SUICIDAL_FLEE)},
	{DIFFICULTY_LEVEL::LOW, (AIBHVR_SUICIDAL_FLEE)},
	{DIFFICULTY_LEVEL::NORMAL, (AIBHVR_SUICIDAL_FLEE | AIBHVR_RANDOM_WALK_ON_FLEE | AIBHVR_FLEES_WHEN_TARGETED)},
	{DIFFICULTY_LEVEL::HIGH, (AIBHVR_SUICIDAL_FLEE | AIBHVR_RANDOM_WALK_ON_FLEE | AIBHVR_FLEES_WHEN_TARGETED
	| AIBHVR_DEFENSIVE_BOOST | AIBHVR_SELF_VELOCITY_TRACKING | AIBHVR_VELOCITY_TRACKING | AIBHVR_AVOIDS_OBSTACLES)},
	{DIFFICULTY_LEVEL::UNFAIR, (AIBHVR_SUICIDAL_FLEE | AIBHVR_RANDOM_WALK_ON_FLEE | AIBHVR_FLEES_WHEN_TARGETED | AIBHVR_AGGRESSIVE_BOOST
	| AIBHVR_DEFENSIVE_BOOST | AIBHVR_SELF_VELOCITY_TRACKING | AIBHVR_VELOCITY_TRACKING | AIBHVR_AVOIDS_OBSTACLES)},
	{DIFFICULTY_LEVEL::DIFFICULTY_MAXVAL, (AIBHVR_SUICIDAL_FLEE | AIBHVR_RANDOM_WALK_ON_FLEE | AIBHVR_FLEES_WHEN_TARGETED | AIBHVR_AGGRESSIVE_BOOST
	| AIBHVR_STAY_AT_MAX_RANGE | AIBHVR_DEFENSIVE_BOOST | AIBHVR_SELF_VELOCITY_TRACKING | AIBHVR_VELOCITY_TRACKING | AIBHVR_AVOIDS_OBSTACLES)}
};
const std::map<DIFFICULTY_LEVEL, f32> aiResolveDifficulties =
{
	{DIFFICULTY_LEVEL::GAMES_JOURNALIST, .35f},
	{DIFFICULTY_LEVEL::LOW, .6f},
	{DIFFICULTY_LEVEL::NORMAL, .75f},
	{DIFFICULTY_LEVEL::HIGH, .9f},
	{DIFFICULTY_LEVEL::UNFAIR, .95f},
	{DIFFICULTY_LEVEL::DIFFICULTY_MAXVAL, .99f}
};
const std::map<DIFFICULTY_LEVEL, f32> aiAimDifficulties =
{
	{DIFFICULTY_LEVEL::GAMES_JOURNALIST, .5f},
	{DIFFICULTY_LEVEL::LOW, 1.5f},
	{DIFFICULTY_LEVEL::NORMAL, 3.f},
	{DIFFICULTY_LEVEL::HIGH, 6.f},
	{DIFFICULTY_LEVEL::UNFAIR, 11.f},
	{DIFFICULTY_LEVEL::DIFFICULTY_MAXVAL, 20.f}
};
const std::map<DIFFICULTY_LEVEL, u32> aiNumDifficulties =
{
	{DIFFICULTY_LEVEL::GAMES_JOURNALIST, 1},
	{DIFFICULTY_LEVEL::LOW, 2},
	{DIFFICULTY_LEVEL::NORMAL, 3},
	{DIFFICULTY_LEVEL::HIGH, 5},
	{DIFFICULTY_LEVEL::UNFAIR, 8},
	{DIFFICULTY_LEVEL::DIFFICULTY_MAXVAL, 13}
};
const std::map<GAME_TOGGLE, std::string> gameToggleNames =
{
	{GTOG_LIN_SPACE_FRICTION, "Linear Space Friction"},
	{GTOG_ANG_SPACE_FRICTION, "Angular Space Friction"},
	{GTOG_CONST_THRUST, "Constant Thrust"},
	{GTOG_FRIENDLY_FIRE, "Friendly Fire"},
	{GTOG_IMPACT, "Impact Damage"},
	{GTOG_BANTER, "Wingman Banter"}
};

VideoConfig::VideoConfig() //default configuration
{
	driver = EDT_DIRECT3D9;
	toggles[TOG_FULLSCREEN] = true;
	useScreenRes = true;
	resX = 960;
	resY = 540;
	toggles[TOG_VSYNC] = true;
	toggles[TOG_ALIASING] = true;
	toggles[TOG_STENCILBUF] = false;
	toggles[TOG_FILTER] = true;
	toggles[TOG_ANISOTROPIC] = true;
	toggles[TOG_BUMP] = true;
	particleLevel = SETTING_LEVEL::HIGH;
	renderDist = SETTING_LEVEL::HIGH;
}

void VideoConfig::loadConfig(std::string filename)
{
	gvReader in;
	in.read(filename);
	in.readLinesToValues();
	if (in.lines.empty()) return;

	driver = drivers.at(in.values["driver"]);
	toggles[TOG_FULLSCREEN] = std::stoi(in.values["fullscreen"]);
	useScreenRes = std::stoi(in.values["useScreenRes"]);
	resX = std::stoi(in.values["resX"]);
	resY = std::stoi(in.values["resY"]);
	toggles[TOG_VSYNC] = std::stoi(in.values["vsync"]);
	toggles[TOG_ALIASING] = std::stoi(in.values["antialiasing"]);
	toggles[TOG_STENCILBUF] = std::stoi(in.values["shadows"]);
	toggles[TOG_FILTER] = in.getInt("trilinear");
	toggles[TOG_ANISOTROPIC] = in.getInt("anisotropic");
	toggles[TOG_BUMP] = in.getInt("bump");
	u32 lvl = in.getUint("particleLevel");
	if (lvl >= 3) lvl = 2;
	particleLevel = (SETTING_LEVEL)lvl;
	lvl = in.getUint("renderDist");
	if (lvl >= 3) lvl = 2;
	renderDist = (SETTING_LEVEL)lvl;
}

void VideoConfig::saveConfig(std::string filename)
{
	gvReader out;
	std::string driverstr;
	for (auto pair : drivers) {
		if (pair.second == driver) {
			driverstr = pair.first;
			break;
		}
	}
	out.values["driver"] = driverstr;
	out.values["trilinear"] = std::to_string(toggles[TOG_FILTER]);
	out.values["anisotropic"] = std::to_string(toggles[TOG_ANISOTROPIC]);
	out.values["fullscreen"] = std::to_string(toggles[TOG_FULLSCREEN]);
	out.values["useScreenRes"] = std::to_string(useScreenRes);
	out.values["resX"] = std::to_string(resX);
	out.values["resY"] = std::to_string(resY);
	out.values["vsync"] = std::to_string(toggles[TOG_VSYNC]);
	out.values["shadows"] = std::to_string(toggles[TOG_STENCILBUF]);
	out.values["antialiasing"] = std::to_string(toggles[TOG_ALIASING]);
	out.values["bump"] = std::to_string(toggles[TOG_BUMP]);
	out.values["particleLevel"] = std::to_string((u32)particleLevel);
	out.values["renderDist"] = std::to_string((u32)renderDist);
	out.readValuesToLines();
	out.write(filename);
}

KeyConfig::KeyConfig()
{
	key[IN_THRUST_FORWARDS] = KEY_KEY_W;
	key[IN_STRAFE_BACKWARDS] = KEY_KEY_S;
	key[IN_STRAFE_LEFT] = KEY_KEY_A;
	key[IN_STRAFE_RIGHT] = KEY_KEY_D;
	key[IN_STRAFE_UP] = KEY_SPACE;
	key[IN_STRAFE_DOWN] = KEY_LCONTROL;
	key[IN_AFTERBURNER] = KEY_TAB;
	key[IN_PITCH_UP] = KEY_KEY_R;
	key[IN_PITCH_DOWN] = KEY_KEY_F;
	key[IN_YAW_LEFT] = KEY_KEY_Z;
	key[IN_YAW_RIGHT] = KEY_KEY_C;
	key[IN_ROLL_LEFT] = KEY_KEY_Q;
	key[IN_ROLL_RIGHT] = KEY_KEY_E;
	key[IN_STOP_LIN_MOVEMENT] = KEY_KEY_X;
	key[IN_STOP_ANG_MOVEMENT] = KEY_KEY_X;
	key[IN_PAUSE_MENU] = KEY_ESCAPE;

	key[IN_TOGGLE_SAFETY] = KEY_KEY_U;
	key[IN_TOGGLE_MOUSE] = KEY_KEY_Y;
	key[IN_RELOAD] = KEY_KEY_V;
	key[IN_REVERSE_CAMERA] = KEY_KEY_H;
	//key[IN_TOGGLE_HUD] = KEY_KEY_M;
	//key[IN_TOGGLE_THROTTLE] = KEY_KEY_J;
	key[IN_TOGGLE_LIN_FRICTION] = KEY_KEY_L;
	key[IN_TOGGLE_ANG_FRICTION] = KEY_KEY_K;

	key[IN_SELECT_TARGET] = KEY_KEY_T;
	key[IN_OPEN_COMMS] = KEY_KEY_C;
	key[IN_COMMS_1] = KEY_KEY_1;
	key[IN_COMMS_2] = KEY_KEY_2;
	key[IN_COMMS_3] = KEY_KEY_3;
	key[IN_COMMS_4] = KEY_KEY_4;
	key[IN_COMMS_5] = KEY_KEY_5;
	key[IN_COMMS_6] = KEY_KEY_6;
	key[IN_COMMS_7] = KEY_KEY_7;

	key[IN_FIRE_REGULAR] = KEY_LBUTTON;
	key[IN_FIRE_HEAVY] = KEY_LSHIFT;
	key[IN_FIRE_PHYS] = KEY_RBUTTON;
}

void KeyConfig::loadConfig(std::string filename)
{
	gvReader in;
	in.read(filename);
	if (in.lines.empty()) return;

	in.readLinesToValues();
	for (u32 i = 0; i < IN_MAX_ENUM; ++i) {
		key[i] = (EKEY_CODE)in.getInt(std::to_string(i));
	}
}

void KeyConfig::saveConfig(std::string filename)
{
	gvReader out;
	for (u32 i = 0; i < IN_MAX_ENUM; ++i) {
		out.values[std::to_string(i)] = std::to_string(key[i]);
	}
	out.readValuesToLines();
	out.write(filename);
}

void GameConfig::loadConfig(std::string filename)
{
	gvReader in;
	in.read(filename);
	in.readLinesToValues();
	if (in.lines.empty()) return;

	toggles[GTOG_LIN_SPACE_FRICTION] = std::stoi(in.values["linSpaceFriction"]);
	toggles[GTOG_ANG_SPACE_FRICTION] = in.getUint("angSpaceFriction");
	toggles[GTOG_CONST_THRUST] = std::stoi(in.values["flightAssist"]);
	toggles[GTOG_FRIENDLY_FIRE] = std::stoi(in.values["friendlyFire"]);
	toggles[GTOG_IMPACT] = std::stoi(in.values["impactDamage"]);
	toggles[GTOG_BANTER] = std::stoi(in.values["banter"]);
	difficulties[DIF_PLAYER_DMG] = (DIFFICULTY_LEVEL)in.getUint("playerDmg");
	difficulties[DIF_ENEMY_DMG] = (DIFFICULTY_LEVEL)in.getUint("enemyDmg");
	difficulties[DIF_AI_INT] = (DIFFICULTY_LEVEL)in.getUint("aiSmart");
	difficulties[DIF_AIM] = (DIFFICULTY_LEVEL)in.getUint("aiAim");
	difficulties[DIF_RESOLVE] = (DIFFICULTY_LEVEL)in.getUint("aiCoward");
	difficulties[DIF_WINGNUM] = (DIFFICULTY_LEVEL)in.getUint("wingSize");
	volumes[0] = in.getInt("masterVolume");
	volumes[1] = in.getInt("musicVolume");
	volumes[2] = in.getInt("gameSoundVolume");
	volumes[3] = in.getInt("menuSoundVolume");
}

void GameConfig::saveConfig(std::string filename)
{
	gvReader out;
	out.values["linSpaceFriction"] = std::to_string(toggles[GTOG_LIN_SPACE_FRICTION]);
	out.values["angSpaceFriction"] = std::to_string(toggles[GTOG_ANG_SPACE_FRICTION]);
	out.values["flightAssist"] = std::to_string(toggles[GTOG_CONST_THRUST]);
	out.values["friendlyFire"] = std::to_string(toggles[GTOG_FRIENDLY_FIRE]);
	out.values["impactDamage"] = std::to_string(toggles[GTOG_IMPACT]);
	out.values["banter"] = std::to_string(toggles[GTOG_BANTER]);
	out.values["playerDmg"] = std::to_string((u32)difficulties[DIF_PLAYER_DMG]);
	out.values["enemyDmg"] = std::to_string((u32)difficulties[DIF_ENEMY_DMG]);
	out.values["aiSmart"] = std::to_string((u32)difficulties[DIF_AI_INT]);
	out.values["aiAim"] = std::to_string((u32)difficulties[DIF_AIM]);
	out.values["aiCoward"] = std::to_string((u32)difficulties[DIF_RESOLVE]);
	out.values["wingSize"] = std::to_string((u32)difficulties[DIF_WINGNUM]);
	out.values["masterVolume"] = std::to_string(volumes[0]);
	out.values["menuSoundVolume"] = std::to_string(volumes[3]);
	out.values["gameSoundVolume"] = std::to_string(volumes[2]);
	out.values["musicVolume"] = std::to_string(volumes[1]);

	out.readValuesToLines();
	out.write(filename);
}

void GameConfig::setAudioGains()
{
	f32 newgains[4];
	for(u32 i = 0; i < 4; ++i) {
		newgains[i] = (f32)volumes[i];
		newgains[i] /= 100.f;
	}
	audioDriver->setGlobalGains(newgains[0], newgains[1], newgains[2], newgains[3]);
}

//send help
const std::map<EKEY_CODE, std::wstring> keyDesc =
{
	{KEY_KEY_1, L"1"},
	{KEY_KEY_2, L"2"},
	{KEY_KEY_3, L"3"},
	{KEY_KEY_4, L"4"},
	{KEY_KEY_5, L"5"},
	{KEY_KEY_6, L"6"},
	{KEY_KEY_7, L"7"},
	{KEY_KEY_8, L"8"},
	{KEY_KEY_9, L"9"},
	{KEY_KEY_0, L"0"},
	{KEY_MINUS, L"-"},
	{KEY_PLUS, L"+"},
	{KEY_SPACE, L"Space"},
	{KEY_TAB, L"Tab"},
	{KEY_CAPITAL, L"Caps"},
	{KEY_LSHIFT, L"Left-Shift"},
	{KEY_LCONTROL, L"Left-Control"},
	{KEY_RETURN, L"Enter"},
	{KEY_RSHIFT, L"Right-Shift"},
	{KEY_RCONTROL, L"Right-Control"},
	{KEY_BACK, L"Backspace"},
	{KEY_ESCAPE, L"Escape"},
	{KEY_LMENU, L"Left-Alt"},
	{KEY_RMENU, L"Right-Alt"},
	{KEY_OEM_3, L"`"},
	{KEY_OEM_4, L"["},
	{KEY_OEM_5, L"|"},
	{KEY_OEM_6, L"]"},
	{KEY_OEM_1, L";"},
	{KEY_OEM_7, L"'"},
	{KEY_KEY_Q, L"Q"},
	{KEY_KEY_W, L"W"},
	{KEY_KEY_E, L"E"},
	{KEY_KEY_R, L"R"},
	{KEY_KEY_T, L"T"},
	{KEY_KEY_Y, L"Y"},
	{KEY_KEY_U, L"U"},
	{KEY_KEY_I, L"I"},
	{KEY_KEY_O, L"O"},
	{KEY_KEY_P, L"P"},
	{KEY_KEY_A, L"A"},
	{KEY_KEY_S, L"S"},
	{KEY_KEY_D, L"D"},
	{KEY_KEY_F, L"F"},
	{KEY_KEY_G, L"G"},
	{KEY_KEY_H, L"H"},
	{KEY_KEY_J, L"J"},
	{KEY_KEY_K, L"K"},
	{KEY_KEY_L, L"L"},
	{KEY_KEY_Z, L"Z"},
	{KEY_KEY_X, L"X"},
	{KEY_KEY_C, L"C"},
	{KEY_KEY_V, L"V"},
	{KEY_KEY_B, L"B"},
	{KEY_KEY_N, L"N"},
	{KEY_KEY_M, L"M"},
	{KEY_COMMA, L","},
	{KEY_PERIOD, L"."},
	{KEY_OEM_2, L"/"},
	{KEY_F1, L"F1"},
	{KEY_F2, L"F2"},
	{KEY_F3, L"F3"},
	{KEY_F4, L"F4"},
	{KEY_F5, L"F5"},
	{KEY_F6, L"F6"},
	{KEY_F7, L"F7"},
	{KEY_F8, L"F8"},
	{KEY_F9, L"F9"},
	{KEY_F10, L"F10"},
	{KEY_F11, L"F11"},
	{KEY_F12, L"F12"},
	{KEY_NUMPAD0, L"Numpad-0"},
	{KEY_NUMPAD1, L"Numpad-1"},
	{KEY_NUMPAD2, L"Numpad-2"},
	{KEY_NUMPAD3, L"Numpad-3"},
	{KEY_NUMPAD4, L"Numpad-4"},
	{KEY_NUMPAD5, L"Numpad-5"},
	{KEY_NUMPAD6, L"Numpad-6"},
	{KEY_NUMPAD7, L"Numpad-7"},
	{KEY_NUMPAD8, L"Numpad-8"},
	{KEY_NUMPAD9, L"Numpad-9"},
	{KEY_MBUTTON, L"Middle-Mouse"},
	{KEY_LBUTTON, L"Left-Mouse"},
	{KEY_RBUTTON, L"Right-Mouse"},
	{KEY_KEY_CODES_COUNT, L"Not Bound"}
};

std::wstring getKeyDesc(INPUT in)
{
	return keyDesc.at(key(in));
}

EKEY_CODE key(INPUT in)
{
	return cfg->keys.key[in];
}

const std::map<INPUT, std::wstring> inputNames =
{
	{IN_PAUSE_MENU, L"Pause"},
	{IN_STRAFE_DOWN, L"Strafe Down"},
	{IN_THRUST_FORWARDS, L"Forward Thrust"},
	{IN_STRAFE_BACKWARDS, L"Backward Thrust"},
	{IN_STRAFE_LEFT, L"Strafe Left"},
	{IN_STRAFE_RIGHT, L"Strafe Right"},
	{IN_STRAFE_UP, L"Strafe Up"},
	{IN_STRAFE_DOWN, L"Strafe Down"},
	{IN_AFTERBURNER, L"Boost"},
	{IN_PITCH_UP, L"Pitch Up"},
	{IN_PITCH_DOWN, L"Pitch Down"},
	{IN_YAW_LEFT, L"Yaw Left"},
	{IN_YAW_RIGHT, L"Yaw Right"},
	{IN_ROLL_LEFT, L"Roll Left"},
	{IN_ROLL_RIGHT, L"Roll Right"},
	{IN_STOP_LIN_MOVEMENT, L"Linear Brakes"},
	{IN_STOP_ANG_MOVEMENT, L"Angular Brakes"},
	{IN_RELOAD, L"Force Reload"},

	{IN_TOGGLE_SAFETY, L"Toggle Safety Override"},
	{IN_TOGGLE_MOUSE, L"Toggle Mouse Flight"},
	{IN_TOGGLE_LIN_FRICTION, L"Toggle Linear Friction"},
	{IN_TOGGLE_ANG_FRICTION, L"Toggle Angular Friction"},
	{IN_REVERSE_CAMERA, L"Toggle Reverse Camera"},

	{IN_SELECT_TARGET, L"Select Target"},
	{IN_OPEN_COMMS, L"Open Comm Menu"},
	{IN_COMMS_1, L"Comm Menu 1"},
	{IN_COMMS_2, L"Comm Menu 2"},
	{IN_COMMS_3, L"Comm Menu 3"},
	{IN_COMMS_4, L"Comm Menu 4"},
	{IN_COMMS_5, L"Comm Menu 5"},
	{IN_COMMS_6, L"Comm Menu 6"},
	{IN_COMMS_7, L"Comm Menu 7"},

	{IN_FIRE_REGULAR, L"Fire Regular Weapons"},
	{IN_FIRE_PHYS, L"Fire Physical Weapon"},
	{IN_FIRE_HEAVY, L"Fire Heavy Weapon"},
};