#include "GuiMultiplayerMenu.h"
#include "GuiController.h"
#include "GameStateController.h"
#include "GameController.h"
#include "AudioDriver.h"

void GuiMultiplayerMenu::init()
{
	if (root) root->remove();
	root = guienv->addStaticText(L"", recti(position2di(0, 0), baseSize), false, false);
	scaleAlign(root);

	dimension2du buttonSize(180, 40);
	host = guienv->addButton(recti(vector2di(buttonSize.Width,(baseSize.Height / 2) - (buttonSize.Height / 2)), buttonSize), root, -1, L"Host", L"Host a socket.");
	connect = guienv->addButton(recti(vector2di(baseSize.Width - (buttonSize.Width*2), (baseSize.Height / 2) - (buttonSize.Height / 2)), buttonSize), root, -1, L"Connect", L"Connect to a socket.");
	back = guienv->addButton(recti(position2di(420, 490), dimension2du(120, 40)), root, -1, L"Back", L"What, don't have any friends?");
	startMatch = guienv->addButton(recti(position2di(420, 450), dimension2du(120, 40)), root, -1, L"Start Game", L"Test out the match feature");

	log = guienv->addStaticText(L"The log should go HERE\n", recti(position2di(0, 0), dimension2du(400, 200)), true, true, root);
	setUIText(log);
	log->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);

	for (u32 i = 0; i < 4; ++i) {
		connected[i] = guienv->addStaticText(L"No Connected Player", recti(position2di(400, 0 + (50*i)), dimension2du(200, 50)), true, true, root);
		setUIText(connected[i]);
	}

	setHoloButton(host);
	setHoloButton(back);
	setHoloButton(connect);
	setHoloButton(startMatch);

	guiController->setCallback(host, std::bind(&GuiMultiplayerMenu::onHost, this, std::placeholders::_1), GUI_MULTIPLAYER_MENU);
	guiController->setCallback(back, std::bind(&GuiMultiplayerMenu::onBack, this, std::placeholders::_1), GUI_MULTIPLAYER_MENU);
	guiController->setCallback(connect, std::bind(&GuiMultiplayerMenu::onConnect, this, std::placeholders::_1), GUI_MULTIPLAYER_MENU);
	guiController->setCallback(startMatch, std::bind(&GuiMultiplayerMenu::onStartMatch, this, std::placeholders::_1), GUI_MULTIPLAYER_MENU);
	hide();
}

bool GuiMultiplayerMenu::onBack(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	stateController->closeListenerSocket();
	guiController->setActiveDialog(GUI_MAIN_MENU);
	return true;
}

void GuiMultiplayerMenu::addTextToLog(std::wstring text)
{
	std::wstring txt = log->getText();
	txt += text + L"\n";
	log->setText(txt.c_str());
}

void GuiMultiplayerMenu::setPlayerName(u32 slot, std::wstring name)
{
	connected[slot+1]->setText(name.c_str());
}

bool GuiMultiplayerMenu::onHost(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	stateController->openListenerSocket();
	gameController->setNetworked(true);
	addTextToLog(L"Hosted game");
	return false;
}

bool GuiMultiplayerMenu::onConnect(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	//launch steam multiplayer shift-tab thingy
	return false;
}

bool GuiMultiplayerMenu::onStartMatch(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	//actually start the scenario here
	guiController->setActiveDialog(GUI_LOADING_MENU);
	audioDriver->playMenuSound("menu_confirm.ogg");
	stateController->setState(GAME_RUNNING);
	audioDriver->playMusic("combat_gas.ogg", 0);
	audioDriver->setMusicGain(0, 0.f);
	audioDriver->playMusic("ambient_gas.ogg", 1);
	audioDriver->setMusicGain(1, 1.f);
	stateController->stateUpdatePacket->packetType = PTYPE_GAME_STATE_CHANGE;
	stateController->stateUpdatePacket->numEntries = 1;
	*reinterpret_cast<GAME_STATE*>(stateController->stateUpdatePacket->data) = GAME_RUNNING;
	stateController->stateUpdatePacketIsValid = true;
	addTextToLog(L"state update ready to send");
	return false;
}