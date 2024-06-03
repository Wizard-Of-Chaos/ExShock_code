#pragma once

#ifndef INPUTCOMPONENT_H
#define INPUTCOMPONENT_H
#include "BaseHeader.h"

/*
* The input enum tracks all the keybinds for the game and applies them accordingly. Input keybinds are mapped in the keyConfig structure.
* A key press is bound to one of these, which is then passed to the ship.
*/
enum INPUT {
	IN_THRUST_FORWARDS,
	IN_STRAFE_BACKWARDS,
	IN_STRAFE_LEFT,
	IN_STRAFE_RIGHT,
	IN_STRAFE_UP,
	IN_STRAFE_DOWN,
	IN_AFTERBURNER,
	IN_PITCH_UP,
	IN_PITCH_DOWN,
	IN_YAW_LEFT,
	IN_YAW_RIGHT,
	IN_ROLL_LEFT,
	IN_ROLL_RIGHT,
	IN_FIRE_REGULAR,
	IN_FIRE_PHYS,
	IN_FIRE_HEAVY,
	IN_STOP_LIN_MOVEMENT,
	IN_STOP_ANG_MOVEMENT,
	IN_PAUSE_MENU,
	IN_TOGGLE_SAFETY,
	IN_TOGGLE_MOUSE,
	IN_RELOAD,
	IN_REVERSE_CAMERA,
	//IN_TOGGLE_HUD,
	//IN_TOGGLE_THROTTLE,
	IN_TOGGLE_LIN_FRICTION,
	IN_TOGGLE_ANG_FRICTION,
	IN_SELECT_TARGET,
	IN_OPEN_COMMS,
	IN_COMMS_1,
	IN_COMMS_2,
	IN_COMMS_3,
	IN_COMMS_4,
	IN_COMMS_5,
	IN_COMMS_6,
	IN_COMMS_7,
	IN_MAX_ENUM //doubles as error
};
/*
* The input component takes in player input as part of the OnEvent call. Anything with an InputComponent
* attached gets updated in the ShipControlSystem. It tracks which keys are down, whether or not mouse control is on,
* the position (in pixels) of the mouse, the normalized position of the mouse, and whether or not right/left mouse are down.
*
*/

struct InputComponent {
	bool keysDown[KEY_KEY_CODES_COUNT];
	bool mouseControlEnabled = true;
	bool safetyOverride = false;
	bool usingReverseCamera = false;
	vector2df mousePosition;
	position2di mousePixPosition;
	line3df cameraRay;
	const bool isKeyDown(EKEY_CODE key) const;
	const bool isKeyDown(INPUT key) const;
	const bool commsInput() const;
	const INPUT whichCommsInput() const;
};

#endif