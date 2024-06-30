#include "GuiMessHallMenu.h"
#include "GuiController.h"
#include "AudioDriver.h"

void GuiMessHallMenu::init()
{
	if (root) root->remove();
	auto bg = guienv->addImage(rect<s32>(position2di(0, 0), baseSize));
	bg->setImage(driver->getTexture("assets/ui/messHallScreen.png"));
	bg->setScaleImage(true);
	scaleAlign(bg);
	root = bg;
	dimension2du boardsize(200, 155);
	killboard = guienv->addStaticText(L"", rect<s32>(position2di(291,121), boardsize), false, true, root);
	setUIText(killboard);

	for (u32 i = 0; i < WINGMEN_PER_CAMPAIGN+1; ++i) {
		dimension2du imgSize(15, 15);
		pilotRecords[i].bg = guienv->addStaticText(L"", recti(position2di(0, i*15), dimension2du(200, 15)), false, true, killboard);
		setUIText(pilotRecords[i].bg);
		pilotRecords[i].name = guienv->addStaticText(L"", recti(position2di(0, 0), dimension2du(100, 15)), false, true, pilotRecords[i].bg);
		setAltUITextSmall(pilotRecords[i].name);

		auto img = guienv->addImage(recti(position2di(100, 0), imgSize), pilotRecords[i].bg);
		img->setImage(driver->getTexture("assets/ui/kills.png"));
		scaleAlign(img);

		pilotRecords[i].kills = guienv->addStaticText(L"", recti(position2di(115, 0), dimension2du(30, 15)), false, true, pilotRecords[i].bg);
		setAltUITextSmall(pilotRecords[i].kills);
		pilotRecords[i].kills->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

		img = guienv->addImage(recti(position2di(145, 0), imgSize), pilotRecords[i].bg);
		img->setImage(driver->getTexture("assets/ui/injuries.png"));
		scaleAlign(img);

		pilotRecords[i].injuries = guienv->addStaticText(L"", recti(position2di(160, 0), dimension2du(30, 15)), false, true, pilotRecords[i].bg);
		setAltUITextSmall(pilotRecords[i].injuries);
		pilotRecords[i].injuries->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	}

	totalKills = guienv->addStaticText(L"", rect<s32>(position2di(20, 135), dimension2du(80,15)), false, true, killboard);
	setAltUITextSmall(totalKills);
	totalKills->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

	totalInjuries = guienv->addStaticText(L"", rect<s32>(position2di(120, 135), dimension2du(80, 15)), false, true, killboard);
	setAltUITextSmall(totalInjuries);
	totalInjuries->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

	nav.build(root, GUI_MESS_HALL_MENU);

	blixten = guienv->addButton(recti(vector2di(805,365), dimension2du(65, 45)), root);
	blixten->setUseAlphaChannel(true);
	blixten->setDrawBorder(false);
	setButtonImg(blixten, "assets/ui/transparent_dummy_texture.png", "assets/ui/transparent_dummy_texture.png");
	scaleAlign(blixten);
	guiController->setCallback(blixten, std::bind(&GuiMessHallMenu::onBlixten, this, std::placeholders::_1), GUI_MESS_HALL_MENU);
	hide();
}

void GuiMessHallMenu::show()
{
	GuiDialog::show();

	if (bar) bar->remove();
	for (u32 i = 0; i < 2; ++i) {
		if (talkers[i]) talkers[i]->remove();
		talkers[i] = nullptr;
	}

	for (u32 i = 0; i < WINGMEN_PER_CAMPAIGN; ++i) {
		pilotRecords[i].bg->setVisible(false);
	}
	dimension2du size = driver->getScreenSize();
	u32 height = size.Height;
	u32 width = size.Width;
	f32 talkerStartVertRatio = 520.f / 1080.f;
	f32 talkerStartHorizRatio = 455.f / 1920.f;
	f32 talkerVertRatio = 512.f / 1080.f;
	f32 talkerHorizRatio = 415.f / 1920.f;

	const std::vector<DialogueTree>& available = campaign->getCurAvailableCharacterDialogues();
	s32 i = 0;
	dimension2du talkerSize(width * talkerHorizRatio, height * talkerVertRatio);
	for (DialogueTree tree : available) {
		if (i == 2) break;
		std::wstring speaker = tree.speakers().front();

		//booster seats
		if (speaker == L"Theod Tantrus" || speaker == L"Mi Cha Lee" || speaker == L"James Lavovar") {
			if (i == 1) talkerStartVertRatio = 435.f / 1080.f;
			else talkerStartVertRatio = 470.f / 1080.f;
		}
		if (speaker == L"Tauran Druugas" || speaker == L"ARTHUR" || speaker == L"Sarah Ivanova") {
			if (i == 1) talkerStartVertRatio = 460.f / 1080.f;
			else talkerStartVertRatio = 490.f / 1080.f;
		}

		talkers[i] = guienv->addButton(
			rect<s32>(position2di((talkerStartHorizRatio*width) + ((talkerHorizRatio * i * width) * .9f), talkerStartVertRatio * height), talkerSize),
			root, i);
		std::string regularImg = "assets/ui/people/" + wstrToStr(speaker) + ".png";
		std::string pressedImg = "assets/ui/people/" + wstrToStr(speaker) + "_click.png";
		if (!driver->getTexture(regularImg.c_str())) {
			regularImg = "assets/ui/people/unknown.png";
			pressedImg = regularImg;
		}
		setButtonImg(talkers[i], regularImg, pressedImg);
		talkers[i]->setDrawBorder(false);
		talkers[i]->setUseAlphaChannel(true);
		guiController->setCallback(talkers[i], std::bind(&GuiMessHallMenu::onTalker, this, std::placeholders::_1), GUI_MESS_HALL_MENU);
		++i;
	}
	//bar pos and size ratios
	//note to self: why did I use shitty var names here when the ones immediately above are actually descriptive
	f32 bV = 770.f / 1080.f;
	f32 bH = 500.f / 1920.f;
	f32 bsV = 360.f / 1080.f;
	f32 bsH = 737.f / 1920.f;
	bar = guienv->addImage(rect<s32>(position2di((s32)(bH * width), (s32)(bV * height)), dimension2du((s32)(bsH * width), (s32)(bsV * height))), root);
	bar->setImage(driver->getTexture("assets/ui/messtable.png"));
	bar->setScaleImage(true);
	scaleAlign(bar);
	bar->setUseAlphaChannel(true);
	bar->setVisible(true);

	u32 kills = 0;
	u32 injuries = 0;

	struct _sorter {
		WingmanInstance* man;
		bool operator < (const _sorter& other) const {
			return (man->totalKills > other.man->totalKills);
		}
	};
	array<_sorter> sortarr;

	for (auto wingman : campaign->wingmen()) {
		kills += wingman->totalKills;
		injuries += wingman->totalInjuries;
		sortarr.push_back({ wingman });
	}
	sortarr.sort();

	for (u32 i = 0; i < sortarr.size(); ++i) {
		auto man = sortarr[i].man;
		auto name = man->name;
		std::transform(name.begin(), name.end(), name.begin(), ::toupper);
		if (man->injured) {
			if (man->turnsInjured >= 0)
				name += " (INJURED)";
			else if (man->turnsInjured >= -100)
				name += " (ON MISSION)";
			else
				name += " (GROUNDED)";
		}
		pilotRecords[i].bg->setVisible(true);
		pilotRecords[i].name->setText(wstr(name).c_str());
		pilotRecords[i].kills->setText(std::to_wstring(man->totalKills).c_str());
		pilotRecords[i].injuries->setText(std::to_wstring(man->totalInjuries).c_str());
	}

	std::wstring killsText = guiController->getKillName();
	killsText += L": " + std::to_wstring(kills);
	totalKills->setText(killsText.c_str());
	std::wstring injuriesText = guiController->getInjuryName();
	injuriesText += L": " + std::to_wstring(injuries);
	totalInjuries->setText(injuriesText.c_str());
	nav.show();
}

void GuiMessHallMenu::hide()
{
	GuiDialog::hide();
;
}

bool GuiMessHallMenu::onTalker(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	DialogueTree tree = campaign->getCurAvailableCharacterDialogues()[event.GUIEvent.Caller->getID()];
	if (campaign->isTreeUsed(tree.dialogueId())) {
		audioDriver->playMenuSound("menu_error.ogg");
		return false;
	}
	campaign->setDialogueTreeUsed(tree.dialogueId());
	guiController->setDialogueTree(tree);
	guiController->setActiveDialog(GUI_DIALOGUE_MENU);
	return false;
}

bool GuiMessHallMenu::onBlixten(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setOkPopup("Hands To Yourself", "Please do not bother the ship's cat while he is working. The mice are known traitors to the Republic.");
	guiController->showOkPopup();

	return false;
}