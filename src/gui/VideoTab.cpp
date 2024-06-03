#include "VideoTab.h"
#include "GuiController.h"
#include "Config.h"
#include "AudioDriver.h"
#include <iostream>

void VideoTab::build(IGUIElement* root, VideoConfig* cfg)
{
	m_baseBuild(root);
	vConfig = cfg;
	dimension2du itemSize(227, 20);
	position2di itemStart(10, 45);
	u32 buf = itemSize.Height + 4;

	std::wstring str = L"Render distance: ";
	str += ((u32)vConfig->renderDist == 0) ? L"Low" : ((u32)vConfig->renderDist == 1) ? L"Medium" : L"High";
	renderDist = guienv->addButton(rect<s32>(itemStart, itemSize), base, -1, str.c_str(), L"Set render distance.");
	renderDist->setName("Set the render distance for the game.\n\nThis setting determines how far away your game will attempt to visually draw objects. Lowering this can massively increase performance, but will look worse as objects 'pop in' at range.");
	setThinHoloButton(renderDist, BCOL_GREEN);
	itemStart.Y += buf;
	str = L"Antialiasing: ";
	str += vConfig->toggles[TOG_ALIASING] ? L"On" : L"Off";
	aliasing = guienv->addButton(rect<s32>(itemStart, itemSize), base, TOG_ALIASING, str.c_str(), L"Toggle anti-aliasing.");
	aliasing->setName("Toggle anti-aliasing.\n\nAnti-aliasing blurs the outlines of objects to make them less pixellated. Turning anti-aliasing off can increase performance, but the game will look worse.");
	setThinHoloButton(aliasing, BCOL_BLUE);
	itemStart.Y += buf;
	str = L"Fullscreen: ";
	str += vConfig->toggles[TOG_FULLSCREEN] ? L"On" : L"Off";
	fullscreen = guienv->addButton(rect<s32>(itemStart, itemSize), base, TOG_FULLSCREEN, str.c_str(), L"Toggle fullscreen.");
	fullscreen->setName("Toggle fullscreen.\n\nFullscreen is convenient, but trying to alt-tab will get a little weird with this on. If you are easily distracted, run this game in a window.");
	setThinHoloButton(fullscreen, BCOL_BLUE);
	itemStart.Y += buf;
	str = L"Vsync: ";
	str += vConfig->toggles[TOG_VSYNC] ? L"On" : L"Off";
	vsync = guienv->addButton(rect<s32>(itemStart, itemSize), base, TOG_VSYNC, str.c_str(), L"Toggle vsync.");
	vsync->setName("Toggle vsync.\n\nVsync locks the frame-rate of the game to the refresh rate of your monitor. This can increase performance and reduce screen tearing, but also might cause weird framerate-dependent bugs (an example found in development was music failing to swap with vsync on).\n\nIf you find one, let the devs know!");
	setThinHoloButton(vsync, BCOL_BLUE);
	itemStart.Y += buf;
	str = L"Shadows: ";
	str += vConfig->toggles[TOG_STENCILBUF] ? L"On" : L"Off";
	shadows = guienv->addButton(rect<s32>(itemStart, itemSize), base, TOG_STENCILBUF, str.c_str(), L"Toggle shadows.");
	shadows->setName("Toggle shadows.\n\nShadows make the lighting look more realistic, but require a ton of math under the hood (and are poorly optimized), and leaving them on will degrade performance significantly. Only enable this if your PC has some junk in its trunk.");
	setThinHoloButton(shadows, BCOL_BLUE);
	itemStart.Y += buf;
	str = L"Filtering: ";
	str += vConfig->toggles[TOG_FILTER] ? L"Trilinear" : L"Bilinear";
	filtering = guienv->addButton(rect<s32>(itemStart, itemSize), base, TOG_FILTER, str.c_str(), L"Set filtering type.");
	filtering->setName("Set the filtering type for the game.\n\nThis determines how blurry distant things look, with bilinear filtering being best performance and lowest quality and trilinear being lowest performance and best quality.");
	setThinHoloButton(filtering, BCOL_BLUE);
	itemStart.Y += buf;
	str = L"Anisotropic: ";
	str += vConfig->toggles[TOG_ANISOTROPIC] ? L"On" : L"Off";
	anisotropic = guienv->addButton(rect<s32>(itemStart, itemSize), base, TOG_ANISOTROPIC, str.c_str(), L"Toggle anisotropic.");
	anisotropic->setName("Set whether anisotropic filtering should be used. \n\n Anisotropic filtering is an additional layer of filtering on top of the other two that eliminates a lot of the side effects those filters have. Turning it off will degrade visual quality but improve performance.");
	setThinHoloButton(anisotropic, BCOL_BLUE);
	itemStart.Y += buf;
	str = L"Bump-mapping: ";
	str += vConfig->toggles[TOG_BUMP] ? L"On" : L"Off";
	bump = guienv->addButton(rect<s32>(itemStart, itemSize), base, TOG_BUMP, str.c_str(), L"Toggle bump-maps.");
	bump->setName("Set whether bump maps should be turned on or off. \n\n Bump-mapping is a graphical technique that adjusts pixel colors on the fly to pretend that a texture has depth. Turning it off will degrade visual quality, but increase performance.");
	setThinHoloButton(bump, BCOL_BLUE);
	itemStart.Y += buf;
	str = L"Particle level: ";
	str += ((u32)vConfig->particleLevel == 0) ? L"Low" : ((u32)vConfig->particleLevel == 1) ? L"Medium" : L"High";
	particles = guienv->addButton(rect<s32>(itemStart, itemSize), base, -1, str.c_str(), L"Set particles.");
	particles->setName("Set the particle level for the game.\n\nThis determines how many particles the game is allowed to spew out. Lowering the particle level can increase performance, but the game will look worse.");
	setThinHoloButton(particles, BCOL_ORANGE);
	itemStart.Y += buf;

	guiController->setCallback(aliasing, std::bind(&VideoTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(fullscreen, std::bind(&VideoTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(vsync, std::bind(&VideoTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(shadows, std::bind(&VideoTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(particles, std::bind(&VideoTab::onParticles, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(renderDist, std::bind(&VideoTab::onRenderDist, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(filtering, std::bind(&VideoTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(anisotropic, std::bind(&VideoTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(bump, std::bind(&VideoTab::onToggle, this, std::placeholders::_1), GUI_SETTINGS_MENU);

	restart->setText(L"A restart is required to apply video settings.");
}

void VideoTab::show()
{
	SettingsTab::show();
}

bool VideoTab::onToggle(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED && event.GUIEvent.EventType != EGET_ELEMENT_HOVERED) return true;
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		explain->setText(wstr(std::string(event.GUIEvent.Caller->getName())).c_str());
		return false;
	}
	VIDEO_TOGGLE tog = (VIDEO_TOGGLE)event.GUIEvent.Caller->getID();
	vConfig->toggles[tog] = !vConfig->toggles[tog];
	std::string txt="";
	for (auto pair : videoToggleNames) {
		if (pair.second == tog) {
			txt += pair.first;
		}
	}
	txt += ": ";
	if (tog == TOG_FILTER) txt += vConfig->toggles[tog] ? "Trilinear" : "Bilinear";
	else txt += vConfig->toggles[tog] ? "On" : "Off";
	event.GUIEvent.Caller->setText(wstr(txt).c_str());
	return false;
}
bool VideoTab::onParticles(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED && event.GUIEvent.EventType != EGET_ELEMENT_HOVERED) return true;
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		explain->setText(wstr(std::string(event.GUIEvent.Caller->getName())).c_str());
		return false;
	}
	u32 lvl = (u32)vConfig->particleLevel;
	++lvl;
	if ((SETTING_LEVEL)lvl == SETTING_LEVEL::SETTINGLEVEL_MAX) {
		lvl = 0;
	}
	vConfig->particleLevel = (SETTING_LEVEL)lvl;
	//chained conditionals make me feel so dirty.
	std::string level = "Particle level: ";
	level += (lvl == 0) ? "Low" : (lvl == 1) ? "Medium" : "High";
	particles->setText(wstr(level).c_str());
	return false;
}
bool VideoTab::onRenderDist(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED && event.GUIEvent.EventType != EGET_ELEMENT_HOVERED) return true;
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		explain->setText(wstr(std::string(event.GUIEvent.Caller->getName())).c_str());
		return false;
	}
	u32 lvl = (u32)vConfig->renderDist;
	++lvl;
	if ((SETTING_LEVEL)lvl == SETTING_LEVEL::SETTINGLEVEL_MAX) {
		lvl = 0;
	}
	vConfig->renderDist = (SETTING_LEVEL)lvl;
	//chained conditionals make me feel so dirty.
	std::string level = "Render distance: ";
	level += (lvl == 0) ? "Low" : (lvl == 1) ? "Medium" : "High";
	renderDist->setText(wstr(level).c_str());
	return false;
}

bool VideoTab::onResolution(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED && event.GUIEvent.EventType != EGET_ELEMENT_HOVERED) return true;
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		explain->setText(wstr(std::string(event.GUIEvent.Caller->getName())).c_str());
		return false;
	}
	if (cfg->vid.useScreenRes) {
		audioDriver->playMenuSound("menu_error.ogg");
		guiController->setOkPopup("Screen Res Is On", "The game is currently using the default screen resolution. To disable this, set 'useScreenRes' in assets/cfg/videoconfig.gdat to 0.");
		guiController->showOkPopup();
		return false;
	}
	return false;
}