#pragma once
#ifndef DIALOGUE_H
#define DIALOGUE_H
#include "BaseHeader.h"

//Typedef for any sort of function that can be used as part of a dialogue choice. Can be generic or specialized.
typedef std::function<void()> DialogueCallback;

/*
* Dialogue choices represent an individual choice of dialogue, and include the text and an std::function that references whatever
* consequences might come of said choice. It also includes a bool saying whether or not the choice has previously been used.
*/
class DialogueChoice
{
	public:
		DialogueChoice() : m_used(false) {}
		DialogueChoice(std::wstring text, DialogueCallback cb, std::wstring next) :
			m_text(text), m_callback(cb), m_next(next), m_used(false) {}

		void callback() { m_callback(); }

		const std::wstring text() { return m_text; }
		const std::wstring next() { return m_next; }

		const bool used() { return m_used; }
		bool setUsed(bool used = true) { m_used = used; }

		const std::vector<std::wstring>& setFlags() { return m_setFlags; }
		const std::vector<std::wstring>& requiredFlags() { return m_requiredFlags; }
		const std::vector<std::wstring>& requiredTrees() { return m_requiredTrees; }

		void addSetFlag(std::wstring flag) { m_setFlags.push_back(flag); }
		void addRequiredFlag(std::wstring flag) { m_requiredFlags.push_back(flag); }
		void addRequiredTree(std::wstring tree) { m_requiredTrees.push_back(tree); }
	private:
		std::vector<std::wstring> m_setFlags;
		std::vector<std::wstring> m_requiredFlags;
		std::vector<std::wstring> m_requiredTrees;
		std::wstring m_text;
		DialogueCallback m_callback;
		std::wstring m_next;
		bool m_used;

};

/*
* A dialogue node represents an individual chunk of conversation in a dialogue. This includes whoever is currently speaking, the text of the
* dialogue in question, the ID of the node (which is used to identify what gets played next from dialogue choices), and the different choices available
* to that node.
*/
class DialogueNode
{
	public:
		DialogueNode() {}
		DialogueNode(std::wstring id) :
			m_id(id) {}
		DialogueNode(std::wstring id, std::wstring speaker, std::wstring text, std::vector<DialogueChoice>& choices) :
			m_id(id), m_speaker(speaker), m_text(text), m_choices(choices) {}

		const std::wstring speaker() { return m_speaker; }
		const std::wstring text() { return m_text; }
		const std::vector<DialogueChoice>& choices() {return m_choices; }
		void setSpeaker(std::wstring speaker) { m_speaker = speaker; }
		void setText(std::wstring text) { m_text = text; }
		void setId(std::wstring id) { m_id = id; }
		void addChoice(DialogueChoice choice) { m_choices.push_back(choice); }

		void print();
	private:
		std::wstring m_id;
		std::wstring m_speaker;
		std::wstring m_text;
		std::vector<DialogueChoice> m_choices;
};
/*
* A dialogue tree contains all of the choices and nodes for a given conversation. It includes all of those and their various features, as well
* as utilities for loading a dialogue tree from file, the different speakers in a tree, the minimum sector this tree can be played at and a bool
* determining whether or not this conversation has previously been played.
*/
class DialogueTree
{
	public:
		DialogueTree() {}
		//reads tree from XML file
		DialogueTree(std::string filename);
		const bool isUsed() { return m_used; }
		const u32 minSector() { return m_minSector; }
		const std::vector<std::wstring> speakers() { return m_speakers; }
		const std::vector<std::wstring> requiredTrees() { return m_requiredTrees; }
		const std::vector<std::wstring> requiredFlags() { return m_requiredFlags; }
		const std::wstring dialogueId() { return m_id; }

		const DialogueNode& getNode(std::wstring id) { return m_allNodes[id]; }

		void setUsed(bool used = true) { m_used = used; }

		void print();
	private:
		bool m_parseTreeInfo(IrrXMLReader* xml);
		DialogueChoice m_parseChoiceInfo(IrrXMLReader* xml);

		std::wstring m_id=L"";
		bool m_used=false;
		std::vector<std::wstring> m_speakers;
		std::vector<std::wstring> m_requiredTrees;
		std::vector<std::wstring> m_requiredFlags;
		std::unordered_map<std::wstring, DialogueNode> m_allNodes; //not accessible from the outside; dialogue should trawl the tree
		u32 m_minSector=0;
};

class Banter
{
	public:
		struct _line {
			std::wstring spkr;
			std::wstring line;
		};
		std::wstring id() { return m_id; }
		void load(std::string fname);
		std::list<_line>& lines() { return m_dialogueLines; }
		std::list<std::wstring>& speakers() { return m_speakers; }
		const std::list<std::wstring>& requiredTrees() { return m_requiredTrees; }
		const std::list<std::wstring>& requiredFlags() { return m_requiredFlags; }
		const std::list<std::wstring>& requiredBanter() { return m_requiredBanter; }
		const u32 minSector() const { return m_minSector; }
		void setUsed(bool used = true) { m_used = used; }
		const bool used() const { return m_used; }
	private:
		bool m_used = false;
		u32 m_minSector = 0;
		std::wstring m_id = L"";
		std::list<_line> m_dialogueLines;
		std::list<std::wstring> m_speakers;
		std::list<std::wstring> m_requiredTrees;
		std::list<std::wstring> m_requiredFlags;
		std::list<std::wstring> m_requiredBanter;

};

#endif 