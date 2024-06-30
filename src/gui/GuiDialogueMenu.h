#pragma once
#ifndef GUI_DIALOGUE_MENU_H
#define GUI_DIALOGUE_MENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"
#include "Dialogue.h"

class GuiDialogueMenu : public GuiDialog
{
	public:
		GuiDialogueMenu() : GuiDialog(), bg(0), speaker(0), textBg(0), speakerName(0) {}
		virtual void init();
		virtual void show();
		void setPreviousDialog(MENU_TYPE menu) { previousDialog = menu; }
		void setDialogue(DialogueTree tree);
		bool onChoice(const SEvent& event);
		std::wstring getCurSpeaker() { return currentNode.speaker(); }
		bool onTextBgScroll(const SEvent& event);
	private:
		void m_cleanChoices();
		void m_setNextNode(std::wstring id);
		IGUIStaticText* m_addTextToScroll(std::wstring spkr, std::wstring str);
		DialogueNode currentNode;
		DialogueTree currentTree;
		MENU_TYPE previousDialog;
		IGUIImage* bg;
		IGUIImage* speaker;
		IGUIStaticText* textBg;
		IGUIStaticText* previousTxt = nullptr;
		IGUIStaticText* header = nullptr;
		IGUIStaticText* firstAfterHeader = nullptr;
		IGUIStaticText* speakerName;
		struct _choice {
			IGUIButton* button;
			DialogueChoice choice;
		};
		std::vector<_choice> choices;
};
#endif 