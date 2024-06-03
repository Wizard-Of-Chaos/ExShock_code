#include "GuiDialogueMenu.h"
#include "GameController.h"
#include "GuiController.h"
#include "Dialogue.h"
#include "Campaign.h"
#include <iostream>

void GuiDialogueMenu::init()
{
	if (root) root->remove();
	auto screenroot = guienv->addStaticText(L"", rect<s32>(position2di(0, 0), baseSize));
	screenroot->setBackgroundColor(SColor(175, 20, 20, 20));
	screenroot->setDrawBackground(true);
	root = screenroot;
	speaker = guienv->addImage(rect<s32>(position2di(105, 65), dimension2du(384, 475)), root);
	speaker->setImage(driver->getTexture("assets/ui/portraits/unknown.png"));
	scaleAlign(speaker);

	bg = guienv->addImage(rect<s32>(position2di(0, 0), baseSize), root);
	bg->setImage(driver->getTexture("assets/ui/dialoguewindow.png"));
	scaleAlign(bg);
	//the big "block" starts at 493
	textBg = guienv->addStaticText(L"", recti(position2di(493, 0), dimension2du(315, 300)), false, true, bg);
	setDialogueText(textBg);
	header = guienv->addStaticText(L"", recti(position2di(0, 149), dimension2du(315, 1)), false, true, textBg);
	setDialogueText(header);

	speakerName = guienv->addStaticText(L"", rect<s32>(position2di(161, 11), dimension2du(283, 46)), false, true, bg);
	setDialogueText(speakerName);

	hide();
}
void GuiDialogueMenu::show()
{
	root->setRelativePosition(rect<s32>(position2di(0, 0), driver->getScreenSize()));
	root->setVisible(true);
}

void GuiDialogueMenu::m_cleanChoices()
{ 
	for (auto& c : choices) { 
		if (c.button) { 
			guiController->removeCallback(c.button); c.button->remove(); 
		} 
	} 
	choices.clear(); 
}

void GuiDialogueMenu::setDialogue(DialogueTree tree)
{
	currentTree = tree;
	m_setNextNode(L"root");
}

static const bool _choiceValid(DialogueChoice& choice) {
	for (const auto& flag : choice.requiredFlags()) {
		if (!campaign->getFlag(flag)) {
			return false;
		}
	}
	for (const auto& tree : choice.requiredTrees()) {
		if (!campaign->isTreeUsed(tree)) {
			return false;
		}
	}
	return true;
}

IGUIStaticText* GuiDialogueMenu::m_addTextToScroll(std::wstring str)
{
	auto& screen = driver->getScreenSize();

	s32 fullHeight = (300.f / 540.f) * screen.Height;
	dimension2du fullTxtSize((315.f / 960.f) * screen.Width, fullHeight);

	IGUIStaticText* prev = header;
	if (previousTxt) prev = previousTxt;
	auto txt = guienv->addStaticText(str.c_str(), recti(vector2di(0, 0), fullTxtSize), false, true, prev);
	setDialogueText(txt);
	txt->setNotClipped(true);

	s32 height = txt->getTextHeight();
	s32 buf = (6.f / 540.f) * screen.Height;
	txt->setRelativePosition(recti(vector2di(0, prev->getRelativePosition().getHeight() + buf), dimension2du(fullTxtSize.Width, height)));

	s32 headerHeight = (header->getRelativePosition().UpperLeftCorner.Y - (height / 2));
	header->setRelativePosition(vector2di(0, headerHeight));
	s32 bottomY = txt->getAbsolutePosition().LowerRightCorner.Y;
	if (bottomY > .52f * screen.Height) {
		headerHeight -= (bottomY - (.52f * screen.Height));
		header->setRelativePosition(vector2di(0, headerHeight));
	}

	if (prev == header) firstAfterHeader = txt;
	previousTxt = txt;
	return txt;
}

void GuiDialogueMenu::m_setNextNode(std::wstring id)
{
	m_cleanChoices();
	currentNode = currentTree.getNode(id);
	std::wstring speakerStr = currentNode.speaker();
	if (speakerStr == L"Entity") speakerStr = L"ARTHUR";
	std::string path = "assets/ui/fullres_people/" + wstrToStr(speakerStr);
	path += ".png";
	auto spkr = driver->getTexture(path.c_str());
	if (!spkr) spkr = driver->getTexture("assets/ui/people/unknown.png");

	m_addTextToScroll(currentNode.text().c_str());

	speakerName->setText(currentNode.speaker().c_str());
	speaker->setImage(spkr);

	s32 choiceNum = 0;
	auto rel = root->getRelativePosition();
	s32 numChoices = (s32)currentNode.choices().size();
	for (DialogueChoice choice : currentNode.choices()) {
		if (!_choiceValid(choice)) --numChoices; //so we don't leave any holes
	}
	dimension2du relDim((691.f / 1920.f) * rel.getWidth(), (90.0f / 1080.f) * rel.getHeight());
	s32 startHeight = rel.getHeight() - (relDim.Height * numChoices);
	position2di startPos((956.f / 1920.f) * rel.getWidth(), startHeight);

	for (DialogueChoice choice : currentNode.choices()) {
		if (!_choiceValid(choice)) continue;

		position2di relPos = startPos + (choiceNum * position2di(0, relDim.Height));

		IGUIButton* button = guienv->addButton(rect<s32>(relPos, relDim), bg);
		setButtonImg(button, "assets/ui/dialoguechoice.png", "assets/ui/dialoguechoicehighlight.png");
		IGUIStaticText* buttonTxt = guienv->addStaticText(choice.text().c_str(), recti(vector2di(0, 0), relDim), false, true, button);
		setUIText(buttonTxt);
		//button->setText(choice.text().c_str());
		button->setName(choice.next().c_str());
		button->setNotClipped(true);
		button->setUseAlphaChannel();
		button->setDrawBorder(false);
		choices.push_back({ button, choice });
		guiController->setCallback(button, std::bind(&GuiDialogueMenu::onChoice, this, std::placeholders::_1), GUI_DIALOGUE_MENU);

		++choiceNum;
	}
}

bool GuiDialogueMenu::onChoice(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	auto namestr = std::string(event.GUIEvent.Caller->getName());
	auto next = wstr(namestr);
	for (auto& c : choices) {
		if (c.button != event.GUIEvent.Caller) continue;

		for (auto& flag : c.choice.setFlags()) {
			campaign->setFlag(flag);
		}
		c.choice.callback();

		auto txt = m_addTextToScroll(c.choice.text().c_str());
		txt->setOverrideColor(SColor(255, 255, 255, 255));
		txt->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

		break;
	}

	if (next == L"none") {
		MENU_TYPE type = previousDialog;
		if (currentTree.dialogueId() == L"steven_newgame") {
			campaign->setDialogueTreeUsed(L"steven_newgame");
			type = GUI_CAMPAIGN_MENU;
		}
		guiController->setActiveDialog(type);

		firstAfterHeader->remove();
		previousTxt = nullptr;
		firstAfterHeader = nullptr;
		auto screen = driver->getScreenSize();
		header->setRelativePosition(vector2di(header->getRelativePosition().UpperLeftCorner.X, textBg->getRelativePosition().getHeight() / 2));
		return false;
	}
	m_setNextNode(next);
	return false;
}

bool GuiDialogueMenu::onTextBgScroll(const SEvent& event)
{
	return true;
}