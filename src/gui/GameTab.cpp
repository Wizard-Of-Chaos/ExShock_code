#include "GameTab.h"
#include "GuiController.h"
#include "GameStateController.h"

void GameTab::build(IGUIElement* root)
{
	m_baseBuild(root);
	dimension2du itemSize(227, 17);
	position2di itemStart(10, 45);
	u32 buf = itemSize.Height + 1;
	std::wstring str = L"Player Damage: ";
	str += wstr(difLevelNames.at(cfg->game.difficulties[DIF_PLAYER_DMG]));
	playerDmg = guienv->addButton(rect<s32>(itemStart, itemSize), base, DIF_PLAYER_DMG, str.c_str(), L"Set player damage.");
	playerDmg->setName("Set the player (and wingmen)'s damage values.\nValues on different difficulties:\n\nPathetic: 25%\nLow: 50%\nNormal: 100%\nHigh: 200%\nUnfair: 400%");
	setThinHoloButton(playerDmg, BCOL_GREEN);
	itemStart.Y += buf;
	str = L"Enemy Damage: ";
	str += wstr(difLevelNames.at(cfg->game.difficulties[DIF_ENEMY_DMG]));
	enemyDmg = guienv->addButton(rect<s32>(itemStart, itemSize), base, DIF_ENEMY_DMG, str.c_str(), L"Set enemy damage.");
	enemyDmg->setName("Set the enemy's damage values.\nValues on different difficulties:\n\nPathetic: 25%\nLow: 50%\nNormal: 100%\nHigh: 200%\nUnfair: 400%");
	setThinHoloButton(enemyDmg, BCOL_GREEN);

	itemStart.Y += buf;
	str = L"AI Intelligence: ";
	str += wstr(difLevelNames.at(cfg->game.difficulties[DIF_AI_INT]));
	aiInt = guienv->addButton(rect<s32>(itemStart, itemSize), base, DIF_AI_INT, str.c_str(), L"Set AI intelligence.");
	aiInt->setName("Set the enemy's flying intelligence.\n\nThis setting is vague, but effectively it determines what 'behaviors' the AI uses during flight - whether or not they boost, or flee effectively, or fly like a drunk.");
	setThinHoloButton(aiInt, BCOL_ORANGE);
	itemStart.Y += buf;
	str = L"AI Aim: ";
	str += wstr(difLevelNames.at(cfg->game.difficulties[DIF_AIM]));
	aiAim = guienv->addButton(rect<s32>(itemStart, itemSize), base, DIF_AIM, str.c_str(), L"Set AI aim.");
	aiAim->setName("Set the enemy's aiming ability.\n\nThis determines how often the AI misses a shot. Higher difficulties mean the AI is less likely to miss. This is less fun than you think.");
	setThinHoloButton(aiAim, BCOL_ORANGE);
	itemStart.Y += buf;
	str = L"AI Cowardice: ";
	str += wstr(difLevelNames.at(cfg->game.difficulties[DIF_RESOLVE]));
	aiCoward = guienv->addButton(rect<s32>(itemStart, itemSize), base, DIF_RESOLVE, str.c_str(), L"Set enemy resolve.");
	aiCoward->setName("Set the enemy's 'resolve'.\n\nThe AI will flee after taking enough damage. Lower values here mean the AI is more likely to run away screaming instead of shooting you.");
	setThinHoloButton(aiCoward, BCOL_ORANGE);
	itemStart.Y += buf;
	str = L"AI Wing Count: ";
	str += wstr(difLevelNames.at(cfg->game.difficulties[DIF_WINGNUM]));
	aiNum = guienv->addButton(rect<s32>(itemStart, itemSize), base, DIF_WINGNUM, str.c_str(), L"Set enemy numbers.");
	aiNum->setName("Set the amount of enemies on a wing.\n\nThis will increase the amount of enemies you face in any given scenario.");
	setThinHoloButton(aiNum, BCOL_ORANGE);
	itemStart.Y += buf;
	str = L"Linear Space Friction: ";
	str += cfg->game.toggles[GTOG_LIN_SPACE_FRICTION] ? L"On" : L"Off";
	linSpaceFriction = guienv->addButton(rect<s32>(itemStart, itemSize), base, GTOG_LIN_SPACE_FRICTION, str.c_str(), L"Toggle space friction.");
	linSpaceFriction->setName("Toggle linear 'space friction'.\n\nEnabling linear space friction means that you will slow down when not accelerating, and eventually stop. Turning this off can give you an advantage, but is more likely to just confuse you.");
	setThinHoloButton(linSpaceFriction, BCOL_BLUE);
	itemStart.Y += buf;
	str = L"Angular Space Friction: ";
	str += cfg->game.toggles[GTOG_ANG_SPACE_FRICTION] ? L"On" : L"Off";
	angSpaceFriction = guienv->addButton(rect<s32>(itemStart, itemSize), base, GTOG_ANG_SPACE_FRICTION, str.c_str(), L"Toggle space friction.");
	angSpaceFriction->setName("Toggle angular 'space friction'.\n\nEnabling angular space friction means that you will slow your rotation when not actively rotating, and eventually stop. Turning this off can give you an advantage, but is more likely to just confuse you.");
	setThinHoloButton(angSpaceFriction, BCOL_BLUE);
	itemStart.Y += buf;
	str = L"Constant Thrust: ";
	str += cfg->game.toggles[GTOG_CONST_THRUST] ? L"On" : L"Off";
	constThrust = guienv->addButton(rect<s32>(itemStart, itemSize), base, GTOG_CONST_THRUST, str.c_str(), L"Toggle constant thrust.");
	constThrust->setName("Toggle constant thrust.\n\nConstant thrust means that your ship will attempt to stay at the same forward velocity at all times. This means you don't need to hit the 'accelerate' button, but can get a little weird.");
	setThinHoloButton(constThrust, BCOL_BLUE);
	itemStart.Y += buf;
	str = L"Friendly Fire: ";
	str += cfg->game.toggles[GTOG_FRIENDLY_FIRE] ? L"On" : L"Off";
	friendlyFire = guienv->addButton(rect<s32>(itemStart, itemSize), base, GTOG_FRIENDLY_FIRE, str.c_str(), L"Toggle friendly fire.");
	friendlyFire->setName("Toggle friendly fire.\n\nThis setting, if disabled, means you won't be able to damage your allies. Enemy ships, however, will always have this turned on. They have it too good anyway.");
	setThinHoloButton(friendlyFire, BCOL_RED);
	itemStart.Y += buf;
	str = L"Impact Damage: ";
	str += cfg->game.toggles[GTOG_IMPACT] ? L"On" : L"Off";
	impact = guienv->addButton(rect<s32>(itemStart, itemSize), base, GTOG_IMPACT, str.c_str(), L"Toggle impact damage.");
	impact->setName("Toggle impact damage.\n\nIf enabled, neither you nor your allies will take damage from slamming into objects. Enemy ships still will, however.");
	setThinHoloButton(impact, BCOL_RED);
	itemStart.Y += buf;
	str = L"Wingman Banter: ";
	str += cfg->game.toggles[GTOG_BANTER] ? L"On" : L"Off";
	banter = guienv->addButton(rect<s32>(itemStart, itemSize), base, GTOG_BANTER, str.c_str(), L"Toggle banter.");
	banter->setName("Toggle banter.\n\nYour wingmen will occasionally chat with each other mid mission. If you want them to shut up, turn this off.");
	setThinHoloButton(banter, BCOL_YELLOW);
	itemStart.Y += buf;

	dimension2du volumeSize(50, 17);
	for (u32 i = 0; i < 4; ++i) {
		volumes[i].l = guienv->addButton(rect<s32>(itemStart, volumeSize), base, i, L"Decrease", L"Decrease volume.");
		volumes[i].r = guienv->addButton(rect<s32>(itemStart + position2di(177, 0), volumeSize), base, i, L"Increase", L"Increase volume.");
		volumes[i].txt = guienv->addStaticText(L"", rect<s32>(itemStart + position2di(50, 0), dimension2du(100, 17)), false, true, base);
		volumes[i].lvl = guienv->addStaticText(std::to_wstring(cfg->game.volumes[i]).c_str(), rect<s32>(itemStart + position2di(150, 0), dimension2du(27, 17)), false, true, base);
		setThinHoloButton(volumes[i].l, BCOL_RED);
		setThinHoloButton(volumes[i].r, BCOL_GREEN);
		setUIText(volumes[i].txt);
		setUIText(volumes[i].lvl);
		guiController->setCallback(volumes[i].r, std::bind(&GameTab::onIncreaseVol, this, std::placeholders::_1), GUI_SETTINGS_MENU);
		guiController->setCallback(volumes[i].l, std::bind(&GameTab::onDecreaseVol, this, std::placeholders::_1), GUI_SETTINGS_MENU);
		itemStart.Y += buf;
	}
	std::wstring txt = L"Master Volume: ";
	volumes[0].txt->setText(txt.c_str());
	txt = L"Music Volume: ";
	volumes[1].txt->setText(txt.c_str());
	txt = L"Game Volume: ";
	volumes[2].txt->setText(txt.c_str());
	txt = L"UI Volume: ";
	volumes[3].txt->setText(txt.c_str());

	guiController->setCallback(playerDmg, std::bind(&GameTab::onLevelSet, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(enemyDmg, std::bind(&GameTab::onLevelSet, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(aiInt, std::bind(&GameTab::onLevelSet, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(aiAim, std::bind(&GameTab::onLevelSet, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(aiCoward, std::bind(&GameTab::onLevelSet, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(aiNum, std::bind(&GameTab::onLevelSet, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(linSpaceFriction, std::bind(&GameTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(angSpaceFriction, std::bind(&GameTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(constThrust, std::bind(&GameTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(friendlyFire, std::bind(&GameTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(impact, std::bind(&GameTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(banter, std::bind(&GameTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);

	restart->setText(L"Game setting toggles will be applied on the next scenario.");
	hide();
}
void GameTab::show()
{
	SettingsTab::show();
}

bool GameTab::onToggle(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED && event.GUIEvent.EventType != EGET_ELEMENT_HOVERED) return true;
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		explain->setText(wstr(std::string(event.GUIEvent.Caller->getName())).c_str());
		return false;
	}
	GAME_TOGGLE tog = (GAME_TOGGLE)event.GUIEvent.Caller->getID();
	cfg->game.toggles[tog] = !cfg->game.toggles[tog];
	std::string txt = gameToggleNames.at(tog);
	txt += ": ";
	txt += cfg->game.toggles[tog] ? "On" : "Off";
	event.GUIEvent.Caller->setText(wstr(txt).c_str());
	return false;
}
bool GameTab::onLevelSet(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED && event.GUIEvent.EventType != EGET_ELEMENT_HOVERED) return true;
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		explain->setText(wstr(std::string(event.GUIEvent.Caller->getName())).c_str());
		return false;
	}
	DIFFICULTY dif = (DIFFICULTY)event.GUIEvent.Caller->getID();
	u32 lvl = (u32)cfg->game.difficulties[dif];
	++lvl;
	if ((DIFFICULTY_LEVEL)lvl == DIFFICULTY_LEVEL::DIFFICULTY_MAXVAL) lvl = 1;
	DIFFICULTY_LEVEL newDif = (DIFFICULTY_LEVEL)lvl;
	cfg->game.difficulties[dif] = newDif;
	std::string txt = difficultyNames.at(dif);
	txt += ": ";
	txt += difLevelNames.at(newDif);
	event.GUIEvent.Caller->setText(wstr(txt).c_str());
	return false;
}

bool GameTab::onIncreaseVol(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	u32 which = event.GUIEvent.Caller->getID();
	u32 newsnd = cfg->game.volumes[which] + 5;
	if (newsnd > 100) newsnd = 100;
	cfg->game.volumes[which] = newsnd;
	volumes[which].lvl->setText(std::to_wstring(cfg->game.volumes[which]).c_str());

	cfg->game.setAudioGains();
	return false;
}
bool GameTab::onDecreaseVol(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	u32 which = event.GUIEvent.Caller->getID();
	u32 newsnd = cfg->game.volumes[which] - 5;
	if (newsnd < 0) newsnd = 0;
	cfg->game.volumes[which] = newsnd;
	volumes[which].lvl->setText(std::to_wstring(cfg->game.volumes[which]).c_str());

	cfg->game.setAudioGains();
	return false;

}