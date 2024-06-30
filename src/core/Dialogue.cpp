#include "Dialogue.h"
#include "IrrlichtUtils.h"
#include "DialogueEffects.h"
#include "GvReader.h"
#include "CrashLogger.h"

const std::unordered_map<std::wstring, DialogueCallback> dialogueEffects {
	{L"small_supply", smallSupply},
	{L"medium_supply", mediumSupply},
	{L"large_supply", largeSupply},
	{L"small_ammo", smallAmmo},
	{L"medium_ammo", mediumAmmo},
	{L"large_ammo", largeAmmo},
	{L"none", noDialogueEffect},
	{L"exit", exitDialogue},
	{L"increase_ablative", randEventAblative},
	{L"free_tuxedo", randEventFreeTux},
	{L"sean_cache", randEventSeanCache},
	{L"recruit_arthur", recruitArthur},
	{L"kate_project", kateProject},
	{L"ship_afterburn", shipAfterburn},
	{L"arthur_activate", arthurActivate},
	{L"aim_gain", aimGain},
	{L"aim_loss", aimLoss},
	//these for backward compatibility cause cowardice got renaimed
	{L"coward_gain", resolveLoss},
	{L"coward_loss", resolveGain},
	{L"resolve_gain", resolveGain},
	{L"resolve_loss", resolveLoss},

	{L"react_gain", reactionGain},
	{L"react_loss", reactionLoss},
	{L"aggro_gain", aggroGain},
	{L"aggro_loss", aggroLoss},
	{L"whistle", dialogueWhistle},

	{L"theod_buff", randEventTheodBuff},
	{L"add_bfg", addBFG},
	{L"tauran_tko", tauranTKO},
	{L"lee_chomped", theodChomp},
	{L"lee_weapon", leeBuff},
	{L"lee_debriefed", leeDebriefed},
	{L"lee_grounded", leeGrounded},
	{L"arthur_left", arthurLeft},
	{L"lee_mission_engage", leeMission},
	{L"arnold_mission_engage", arnoldMission},
	{L"floorhit", arnoldFloorHit},
	{L"tauran_mission_engage", tauranMission}
};

DialogueTree::DialogueTree(std::string filename)
{
	IrrXMLReader* xml = createIrrXMLReader(filename.c_str());
	if (!xml) return;

	const stringw treeTag(L"tree");
	const stringw textTag(L"text");
	const stringw nodeTag(L"node");
	const stringw speakerListTag(L"speakers");
	const stringw speakerTag(L"speaker");
	const stringw choiceTag(L"choice");

	const stringw prereqTag(L"prerequisites");
	const stringw requiredTreeTag(L"required_tree");
	const stringw requiredFlagTag(L"required_flag");
	const stringw setFlagTag(L"sets_flag");

	stringw previous=L"";
	std::wstring currentNodeId;
	DialogueChoice curChoice = DialogueChoice();

	while (xml->read()) {
		switch (xml->getNodeType()) {

		case EXN_ELEMENT: {
			const stringw type = xml->getNodeName();
			if (type.equals_ignore_case(treeTag)) m_parseTreeInfo(xml);
			else if (type.equals_ignore_case(speakerListTag)) previous = speakerListTag;
			else if (type.equals_ignore_case(prereqTag)) previous = prereqTag;

			if (previous == speakerListTag && type.equals_ignore_case(speakerTag)) {
				std::string spkr = xml->getAttributeValueSafe("name");
				if (!spkr.empty()) m_speakers.push_back(wstr(spkr));
			}
			if (previous == prereqTag && type.equals_ignore_case(requiredTreeTag)) {
				std::string tree = xml->getAttributeValueSafe("id");
				if (!tree.empty()) m_requiredTrees.push_back(wstr(tree));
			}else if (previous == prereqTag && type.equals_ignore_case(requiredFlagTag)) {
				std::string flag = xml->getAttributeValueSafe("id");
				if (!flag.empty()) m_requiredFlags.push_back(wstr(flag));
			}
			
			if (type.equals_ignore_case(nodeTag)) {
				if (previous == choiceTag) {
					m_allNodes[currentNodeId].addChoice(curChoice);
				}

				std::string data(xml->getAttributeValueSafe("id"));
				if (!data.empty()) {
					m_allNodes[wstr(data)] = DialogueNode(wstr(data));
					currentNodeId = wstr(data);
					previous = nodeTag;
				}
			}
			if (previous == nodeTag && type.equals_ignore_case(speakerTag)) {
				m_allNodes[currentNodeId].setSpeaker(wstr(xml->getAttributeValueSafe("name")));
			}
			if (previous == nodeTag && type.equals_ignore_case(textTag)) {
				m_allNodes[currentNodeId].setText(wstr(xml->getAttributeValueSafe("text")));
			}
			if (previous == choiceTag && type.equals_ignore_case(choiceTag)) {
				m_allNodes[currentNodeId].addChoice(curChoice);
				curChoice = m_parseChoiceInfo(xml);
			}
			if (previous == nodeTag && type.equals_ignore_case(choiceTag)) {
				curChoice = m_parseChoiceInfo(xml);
				previous = choiceTag;
			}
			if (previous == choiceTag && type.equals_ignore_case(requiredFlagTag)) {
				curChoice.addRequiredFlag(wstr(xml->getAttributeValueSafe("id")));
			}
			if (previous == choiceTag && type.equals_ignore_case(requiredTreeTag)) {
				curChoice.addRequiredTree(wstr(xml->getAttributeValueSafe("id")));
			}
			if (previous == choiceTag && type.equals_ignore_case(setFlagTag)) {
				curChoice.addSetFlag(wstr(xml->getAttributeValueSafe("id")));
			}
			break;
		}
			break;
		}
	}
	if (previous == choiceTag) {
		m_allNodes[currentNodeId].addChoice(curChoice);
	}
	delete xml;
}

bool DialogueTree::m_parseTreeInfo(IrrXMLReader* xml)
{
	std::string id(xml->getAttributeValueSafe("id"));
	std::string minsec(xml->getAttributeValueSafe("minsector"));
	if (id.empty()) {
		baedsLogger::log("Could not get ID for this tree. Aborting.\n");
		return false;
	}
	m_id = wstr(id);
	if (minsec.empty()) {
		baedsLogger::log("Dialogue tree does not have min sector - defaulting to 0.\n");
		m_minSector = 0;
	}
	else {
		m_minSector = std::stoi(minsec);
	}
	return true;
}
DialogueChoice DialogueTree::m_parseChoiceInfo(IrrXMLReader* xml)
{
	std::string strtext(xml->getAttributeValueSafe("text"));
	std::string strnext(xml->getAttributeValueSafe("next"));
	std::string streffect(xml->getAttributeValueSafe("effect"));
	std::wstring effect = L"none";
	if (dialogueEffects.contains(wstr(streffect)))
		effect = wstr(streffect);

	return DialogueChoice(wstr(strtext), dialogueEffects.at(effect), wstr(strnext));
}

void DialogueNode::print()
{
	std::wcout << L"Node: " << m_id << L" | Speaker: " << m_speaker << "\n Text: " << m_text << L"\n";
	for (auto choice : m_choices) {
		std::wcout << L"CHOICE: " << choice.text() << L" | NEXT NODE: " << choice.next() << L"\n";
		std::wcout << L"FLAGS TO SET: ";
		for (auto flag : choice.setFlags()) {
			std::wcout << flag << ", ";
		}
		std::wcout << "\n";
		std::wcout << L"FLAGS REQUIRED: ";
		for (auto flag : choice.requiredFlags()) {
			std::wcout << flag << ", ";
		}
		std::wcout << "\n";

		std::wcout << "TREES REQUIRED: ";
		for (auto tree : choice.requiredTrees()) {
			std::wcout << tree << ", ";
		}
		std::wcout << "\n";

	}
	std::wcout << L"\n";
}

void DialogueTree::print()
{
	std::wcout << L"Tree: " << m_id << L" | Minimum sector: " << m_minSector << L"\n";
	std::wcout << L"Speakers: ";
	for (auto speaker : m_speakers) {
		std::wcout << speaker << L", ";
	}
	std::wcout << L"\n";
	for (auto [id, node] : m_allNodes) {
		node.print();
	}
	std::wcout << L"\n";
}

void Banter::load(std::string fname)
{
	gvReader in;
	in.read(fname);
	in.readLinesToValues();
	auto trees = in.getString("trees");

	std::string token = "";
	if (trees != "none") {
		std::stringstream treesplit(trees);
		while (std::getline(treesplit, token, ',')) {
			m_requiredTrees.push_back(wstr(token));
		}
	}
	auto flags = in.getString("flags");
	if (flags != "none") {
		std::stringstream flagsplit(flags);
		while (std::getline(flagsplit, token, ',')) {
			m_requiredTrees.push_back(wstr(token));
		}
	}

	auto banter = in.getString("banter");
	if (banter != "none") {
		std::stringstream bantsplit(flags);
		while (std::getline(bantsplit, token, ',')) {
			m_requiredBanter.push_back(wstr(token));
		}
	}
	m_minSector = in.getUint("minSector");

	m_id = wstr(in.getString("id"));
	if (in.lines.empty()) return;
	for (auto& line : in.lines) {
		if (line.substr(0, 5) == "trees" || line.substr(0, 5) == "flags" || line.substr(0,6) == "banter" || line.substr(0,9) == "minSector" || line.substr(0,2) == "id") continue;
		std::string spkr;
		std::string dialogue;
		std::stringstream split(line);
		std::getline(split, spkr, '=');
		std::getline(split, dialogue, '=');
		bool speakerTracked = false;
		for (auto& str : m_speakers) {
			if (str == wstr(spkr)) speakerTracked = true;
		}
		if (!speakerTracked) m_speakers.push_back(wstr(spkr));
		m_dialogueLines.push_back({ wstr(spkr), wstr(dialogue) });
	}
}