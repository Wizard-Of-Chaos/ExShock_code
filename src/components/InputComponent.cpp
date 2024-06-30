#include "InputComponent.h"
#include "Config.h"

const bool InputComponent::isKeyDown(EKEY_CODE key) const { return keysDown[key]; }
const bool InputComponent::isKeyDown(INPUT key) const { return keysDown[cfg->keys.key[key]]; }
const bool InputComponent::commsInput() const {
	if (isKeyDown(IN_COMMS_1) || isKeyDown(IN_COMMS_2) || isKeyDown(IN_COMMS_3) || isKeyDown(IN_COMMS_4) ||
		isKeyDown(IN_COMMS_5) || isKeyDown(IN_COMMS_6) || isKeyDown(SUMMON_CHAOS_THEORY)) return true;
	return false;
}
const INPUT InputComponent::whichCommsInput() const {
	if (!commsInput()) return IN_MAX_ENUM;

	if (isKeyDown(IN_COMMS_1)) return IN_COMMS_1;
	else if (isKeyDown(IN_COMMS_2)) return IN_COMMS_2;
	else if (isKeyDown(IN_COMMS_3)) return IN_COMMS_3;
	else if (isKeyDown(IN_COMMS_4)) return IN_COMMS_4;
	else if (isKeyDown(IN_COMMS_5)) return IN_COMMS_5;
	else if (isKeyDown(IN_COMMS_6)) return IN_COMMS_6;

	return SUMMON_CHAOS_THEORY;

}