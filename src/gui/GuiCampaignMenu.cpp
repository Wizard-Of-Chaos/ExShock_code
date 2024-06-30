#include "GuiCampaignMenu.h"
#include "GameController.h"
#include "GameStateController.h"
#include "GuiController.h"
#include "AudioDriver.h"

void GuiCampaignMenu::init()
{
	if (root) root->remove();

	root = guienv->addImage(rect<s32>(position2di(0, 0), baseSize));
	IGUIImage* img = (IGUIImage*)root;
	img->setImage(driver->getTexture("assets/ui/starbg.png"));
	scaleAlign(img);
	
	bg = guienv->addImage(rect<s32>(position2di(0, 0), dimension2du(960, 540)), root);
	bg->setImage(driver->getTexture("assets/ui/campaignScreen.png"));
	scaleAlign(bg);

	position2di start(295,99);
	dimension2du realEstate(960 * (1473.f / 3840.f), 540 * (864.f / 2160.f));

	desc = guienv->addStaticText(L"", recti(start + position2di(realEstate.Width * .05f, realEstate.Height * .2f), dimension2du(realEstate.Width * .9f, realEstate.Height * .8f)), false, true, bg);
	setUIText(desc);

	advance = guienv->addButton(rect<s32>(start + position2di(0,realEstate.Height*.1f), dimension2du(realEstate.Width, realEstate.Height * .1f)), root, -1, L"Advance", L"Move to the next scenario.");
	setHoloButton(advance);
	guiController->setCallback(advance, std::bind(&GuiCampaignMenu::advanceConfirm, this, std::placeholders::_1), GUI_CAMPAIGN_MENU);

	dimension2du barSize(realEstate.Width / 9, 4);
	u32 buf = (realEstate.Width / 9) / 8;
	buf = buf / 2;
	for (u32 i = 0; i < MAX_ENCOUNTERS; ++i) {
		barTracker[i] = guienv->addImage(recti(start + vector2di((barSize.Width * i) + (buf * i) + (buf*8), realEstate.Height * .275f), barSize), root);
		scaleAlign(barTracker[i]);
		barTracker[i]->setImage(driver->getTexture("assets/ui/upgradeblock.png"));
		barTracker[i]->setUseAlphaChannel(true);
	}

	carrierSprite = guienv->addImage(recti(vector2di(), dimension2du(barSize.Width, realEstate.Height * .075f)), root);
	scaleAlign(carrierSprite);
	carrierSprite->setImage(driver->getTexture("assets/ui/carriersprite.png"));
	carrierSprite->setUseAlphaChannel(true);

	dimension2du iconSize(18, 18);
	start.Y += (realEstate.Height * .235f) + iconSize.Height;
	auto supplyImg = guienv->addImage(recti(start + vector2di((buf * 8), 0), iconSize), root);
	supplyImg->setImage(driver->getTexture("assets/ui/supplies.png"));
	scaleAlign(supplyImg);

	supplies = guienv->addStaticText(L"", recti(vector2di(iconSize.Width, 0), dimension2du(100, 18)), false, true, supplyImg);
	setAltUITextSmall(supplies);
	supplies->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	supplies->setNotClipped(true);


	u32 detectWidth = realEstate.Width - 236;
	detection = guienv->addStaticText(L"", recti(start + vector2di(iconSize.Width + 100, 0), dimension2du(detectWidth, 18)), false, true, root);
	setAltUIText(detection);

	start.X += (realEstate.Width - 18);
	start.X -= (buf * 8);
	auto ammoImg = guienv->addImage(recti(start, iconSize), root);
	ammoImg->setImage(driver->getTexture("assets/ui/ammo.png"));
	scaleAlign(ammoImg);

	ammo = guienv->addStaticText(L"", recti(vector2di(-100, 0), dimension2du(100, 18)), false, true, ammoImg);
	setAltUITextSmall(ammo);
	ammo->setTextAlignment(EGUIA_LOWERRIGHT, EGUIA_CENTER);
	ammo->setNotClipped(true);

	steven = guienv->addButton(rect<s32>(position2di(100,200), dimension2du(166,346)), bg, -1, L"", L"Talk to Steven.");
	setButtonImg(steven, "assets/ui/steven.png", "assets/ui/stevenhover.png");
	steven->setUseAlphaChannel(true);
	steven->setDrawBorder(false);

	kate = guienv->addButton(rect<s32>(position2di(575, 280), dimension2du(204 * .8f, 308 *.8f)), bg, -1, L"", L"Talk to Kate.");
	setButtonImg(kate, "assets/ui/kate.png", "assets/ui/katehover.png");
	kate->setUseAlphaChannel(true);
	kate->setDrawBorder(false);

	guiController->setCallback(steven, std::bind(&GuiCampaignMenu::onSteven, this, std::placeholders::_1), GUI_CAMPAIGN_MENU);
	guiController->setCallback(kate, std::bind(&GuiCampaignMenu::onKate, this, std::placeholders::_1), GUI_CAMPAIGN_MENU);

	nav.build(bg, GUI_CAMPAIGN_MENU);

	hide();
}

void GuiCampaignMenu::show()
{
	dimension2du screen = driver->getScreenSize();
	root->setRelativePosition(recti(position2di(0, 0), screen));
	root->setVisible(true);

	struct __counter {
		Scenario* scen;
		u32 pos;
	};

	std::vector<__counter> counted;

	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {

		if (scenarioSelects[i]) scenarioSelects[i]->remove();
		scenarioSelects[i] = nullptr;

		Scenario* scen = campaign->getSector()->getScenario(i);
		if (!scen) continue;
		bool sameType = false;
		for (auto& count : counted) {
			if (count.scen->type() == scen->type()) {
				sameType = true;
				break;
			}
		}
		if (sameType) continue;
		counted.push_back({ scen, i });
	}

	dimension2du realEstate(screen.Width * (1473.f / 3840.f), screen.Height * (864.f / 2160.f));
	dimension2du buttonSize(realEstate.Width / counted.size(), realEstate.Height * .1f);
	vector2di start((295 / 960.f)* screen.Width, (99.f / 540.f)* screen.Height);

	for (u32 i = 0; i < counted.size(); ++i) {
		Scenario* scen = counted[i].scen;
		auto button = guienv->addButton(recti(start + vector2di(buttonSize.Width * i, realEstate.Height * .075f), buttonSize), root, counted[i].pos);
		scenarioSelects[counted[i].pos] = button;
		guiController->setCallback(button, std::bind(&GuiCampaignMenu::onShowScenarioInfo, this, std::placeholders::_1), GUI_CAMPAIGN_MENU);
		setThinHoloButton(button);
		if (showing == i) setThinHoloButton(button, BCOL_ORANGE);

		std::string loc = scen->location();
		std::string title = objectiveTypeStrings.at(scen->type());
		std::replace(title.begin(), title.end(), '_', ' ');
		scenarioSelects[counted[i].pos]->setText(wstr(title).c_str());
	}

	ammo->setText(std::to_wstring(campaign->getAmmo()).c_str());
	supplies->setText(wstr(fprecis(campaign->getSupplies(), 5)).c_str());

	if (!campaign->getSector()->getCurrentScenario()) {
		desc->setText(L"");
		detection->setText(L"");
	}

	u32 encounter = campaign->getSector()->getEncounterNum();
	for (u32 i = 0; i < 8; ++i) {
		if (i < encounter) barTracker[i]->setColor(SColor(255, 0, 255, 255));
		else barTracker[i]->setColor(SColor(255, 255, 255, 255));
	}
	if (encounter != 0) encounter -= 1;
	vector2di carrierPos = barTracker[encounter]->getRelativePosition().UpperLeftCorner;
	carrierPos.Y -= ((4.f / 540.f) * screen.Height) + carrierSprite->getRelativePosition().getHeight();
	carrierSprite->setRelativePosition(carrierPos);

	if (campaign->getSector()->hasMoved()) {
		for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
			if(campaign->getSector()->getScenario(i) && scenarioSelects[i]) scenarioSelects[i]->setVisible(true);
		}
		advance->setVisible(false);
	}
	else {
		for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
			if(scenarioSelects[i]) scenarioSelects[i]->setVisible(false);
		}
		advance->setVisible(true);
	}
}

bool GuiCampaignMenu::onSteven(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"steven_explanatory"));
	guiController->setActiveDialog(GUI_DIALOGUE_MENU);
	return false;
}

bool GuiCampaignMenu::onKate(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"kate_explanatory"));
	guiController->setActiveDialog(GUI_DIALOGUE_MENU);
	return false;
}

void GuiCampaignMenu::deselect()
{
	desc->setVisible(false);
	detection->setText(L"");
	sectorInfoShowing = false;
	if (campaign->getSector()) campaign->getSector()->deselectCurrentScenario();
	showing = -1;
}

bool GuiCampaignMenu::onShowScenarioInfo(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (id == -1) return false;

	if (sectorInfoShowing && id == showing) {
		desc->setVisible(false);
		detection->setText(L"");
		sectorInfoShowing = false;
		campaign->getSector()->deselectCurrentScenario();
		if (scenarioSelects[showing]) setThinHoloButton(scenarioSelects[showing]);
		showing = -1;
		return false;
	}

	Scenario* scen = campaign->getSector()->getScenario(id);
	desc->setText(wstr(scen->description()).c_str());
	u32 chance = scen->detectionChance();
	if (scen->type() == SCENARIO_BOSSFIGHT) chance = 100;
	detection->setText(wstr("Interception chance: " + std::to_string(chance) + "%").c_str());

	if (showing != -1) setThinHoloButton(scenarioSelects[showing]);
	if(scenarioSelects[id]) setThinHoloButton(scenarioSelects[id], BCOL_ORANGE);

	showing = id;

	SColor start(255, 100, 255, 50);
	SColor end(255, 255, 50, 0);

	f32 ratio = std::min((f32)chance / 50.f, 1.f);

	detection->setOverrideColor(end.getInterpolated(start, ratio));

	desc->setVisible(true);
	sectorInfoShowing = true;
	campaign->getSector()->selectCurrentScenario(id);
	return false;
}

bool GuiCampaignMenu::advanceConfirm(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	/*
	if (campaign->getSector()->getType() == SECTOR_FLEET_GROUP) {
		guiController->setOkPopup("End of Content", "You've reached the end of the content the devs currently have in the game.\n\nThanks for playing!", "More?");
		guiController->showOkPopup();
		return false;
	}
	*/
	bool boss = campaign->advance();
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		Scenario* scen = campaign->getSector()->getScenario(i);
		if (!scen) {
			if(scenarioSelects[i]) scenarioSelects[i]->setVisible(false);
			continue;
		}
		std::string loc = scen->location();
		std::string title = objectiveTypeStrings.at(scen->type());
		std::replace(title.begin(), title.end(), '_', ' ');
		if (scenarioSelects[i]) {
			scenarioSelects[i]->setText(wstr(title).c_str());
			scenarioSelects[i]->setVisible(true);
		}
	}
	audioDriver->playMenuSound("scenario_advance.ogg");
	campaign->saveCampaign("saves/autosave.xml");
	advance->setVisible(false);
	show();
	m_showShipInfo();
	if (boss || campaign->getSector()->getType() == SECTOR_FLEET_GROUP) {
		guiController->setActiveDialog(GUI_DIALOGUE_MENU); //dialogue shoulda been set by the sector
	}
	return false;
}

void GuiCampaignMenu::m_showShipInfo()
{
	ammo->setText(std::to_wstring(campaign->getAmmo()).c_str());
	supplies->setText(wstr(fprecis(campaign->getSupplies(), 5)).c_str());
}