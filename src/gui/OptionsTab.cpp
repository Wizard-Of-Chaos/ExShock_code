#include "OptionsTab.h"
#include "GuiController.h"
#include "GameStateController.h"
#include <filesystem>

void OptionsTab::m_baseBuild(IGUIElement* root)
{
	IGUIImage* img = guienv->addImage(rect<s32>(position2di(240, 85), dimension2du(480, 336)), root);
	img->setImage(driver->getTexture("assets/ui/baselistitem.png"));
	scaleAlign(img);
	base = img;

	img = guienv->addImage(rect<s32>(position2di(0, 40), dimension2du(480, 2)), base);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);

	img = guienv->addImage(rect<s32>(position2di(239, 40), dimension2du(2, 336)), base);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);

	savedesc = guienv->addStaticText(L"None", rect<s32>(position2di(241, 62), dimension2du(231, 180)), false, true, base);
	setUIText(savedesc);

	confirm = guienv->addButton(rect<s32>(position2di(244, 42), dimension2du(227, 20)), base, -1, L"Confirm", L"Confirm what you want done with this save.");
	setHoloButton(confirm, BCOL_GREEN); //ditto
	confirm->setVisible(false);
	remove = guienv->addButton(rect<s32>(position2di(244, 242), dimension2du(227, 20)), base, -1, L"Delete Save", L"Confirm save removal.");
	setHoloButton(remove, BCOL_RED); //ditto
	remove->setVisible(false);

	guiController->setCallback(remove, std::bind(&OptionsTab::onRemove, this, std::placeholders::_1), GUI_OPTIONS_MENU);
}

dimension2du OptionsTab::m_listItemSize()
{
	dimension2du scrn = driver->getScreenSize();
	return dimension2du(listItemHorizSizeRatio * scrn.Width, listItemVertSizeRatio * scrn.Height);
}
position2di OptionsTab::m_listItemStart()
{
	return position2di(listStartXRatio * driver->getScreenSize().Width, listStartYRatio * driver->getScreenSize().Height);
}
u32 OptionsTab::m_listItemHeightDiff()
{
	dimension2du scrn = driver->getScreenSize();
	return (u32)((listItemVertSizeRatio * scrn.Height) + (listBufferRatio * scrn.Height));
}

bool OptionsTab::onGameSelect(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 num = event.GUIEvent.Caller->getID();
	m_currentSelection = num;
	m_displaySave();
	return false;
}
bool OptionsTab::onRemove(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (m_currentSelection == -1) return false;
	guiController->setYesNoPopup("Delete Save", "The current save selection will be deleted. All progress will be lost. \n\n Are you sure you're okay with this?",
		std::bind(&OptionsTab::removeConfirm, this, std::placeholders::_1));
	guiController->showYesNoPopup();
	return false;
}
bool OptionsTab::removeConfirm(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	std::string loc = saves[m_currentSelection]->getName();
	std::filesystem::remove(loc);
	m_loadSaves();
	return false;
}
void OptionsTab::m_clearList()
{
	for (auto item : saves) {
		if (item) item->remove();
	}
	saves.clear();
}
void OptionsTab::m_displaySave()
{
	if (m_currentSelection == -1) {
		savedesc->setText(L"");
		confirm->setVisible(false);
		remove->setVisible(false);
		return;
	}
	savedesc->setText(saves[m_currentSelection]->getToolTipText().c_str());
	confirm->setVisible(true);
	remove->setVisible(true);
}

void OptionsTab::m_loadSaves()
{
	m_clearList();
	std::string path = "saves/";
	u32 count = 0;
	for (const auto& file : std::filesystem::directory_iterator(path)) {
		std::string savepath = file.path().string();
		IrrXMLReader* xml = createIrrXMLReader(savepath.c_str());
		if (!xml) continue; //wtf?
		std::string saveinfo = "";
		SECTOR_TYPE sector;

		while (xml->read()) {
			std::string node = xml->getNodeName();
			if (node == "campaign") {
				saveinfo += "Time: " + std::to_string((xml->getAttributeValueAsInt("time") / 1000) / 60) + " minutes\n";
				saveinfo += "Supplies: " + fprecis(xml->getAttributeValueAsFloat("supplies"), 5) + " | Ammo: " + std::to_string(xml->getAttributeValueAsInt("ammo")) + "\n";
				sector = (SECTOR_TYPE)xml->getAttributeValueAsInt("sector_type");
				std::string sec = environmentTypeStrings.at(sector);
				std::replace(sec.begin(), sec.end(), '_', ' ');
				saveinfo += "Sector: " + sec + " | Encounter: " + std::to_string(xml->getAttributeValueAsInt("encounter")) + "\n";
				break;
			}
		}
		delete xml;

		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		std::string name = "Save " + std::to_string(count + 1);
		if (savepath == "saves/autosave.xml") name = "Autosave";
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), base, count, wstr(name).c_str(), L"Select this save.");
		setThinHoloButton(button, BCOL_GREEN);
#ifdef _DEMOVERSION
		if (sector != SECTOR_DEBRIS && sector != SECTOR_ASTEROID) {
			button->setEnabled(false);
			setThinHoloButton(button, BCOL_YELLOW);
			saveinfo += "\n\n NOT AVAILABLE FOR DEMO";
		}
#endif
		button->setName(savepath.c_str());
		button->setToolTipText(wstr(saveinfo).c_str());
		guiController->setCallback(button, std::bind(&OptionsTab::onGameSelect, this, std::placeholders::_1), GUI_OPTIONS_MENU);
		saves.push_back(button);
		++count;
	}
}
