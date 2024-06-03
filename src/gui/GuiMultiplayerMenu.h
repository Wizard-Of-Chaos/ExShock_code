#pragma once
#ifndef GUIMULTIPLAYERMENU_H
#define GUIMULTIPLAYERMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"

class GuiMultiplayerMenu : public GuiDialog
{
	public:
		virtual void init();
		bool onHost(const SEvent& event);
		bool onConnect(const SEvent& event);
		bool onBack(const SEvent& event);
		bool onStartMatch(const SEvent& event);
		void addTextToLog(std::wstring text);
		void setPlayerName(u32 slot, std::wstring name = L"No Connected Player");
	private:
		IGUIButton* host;
		IGUIButton* connect;
		IGUIButton* back;

		IGUIButton* startMatch;

		IGUIStaticText* log;
		IGUIStaticText* connected[4];
};
#endif