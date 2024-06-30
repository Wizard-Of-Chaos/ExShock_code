#pragma once
#ifndef DIALOGUE_EFFECTS_H
#include "BaseHeader.h"

//Effects for various generic dialogue choices. Includes things like supply and ammo adds to the game and an exit.

void smallSupply();
void mediumSupply();
void largeSupply();

void smallAmmo();
void mediumAmmo();
void largeAmmo();

void aimGain();
void aimLoss();
void resolveGain();
void resolveLoss();
void reactionGain();
void reactionLoss();
void aggroGain();
void aggroLoss();

void randEventTheodBuff();

void randEventAblative();
void randEventFreeTux();
void randEventSeanCache();

void recruitArthur();
void kateProject();
void addBFG();
void tauranTKO();
void theodChomp();
void arnoldMission();
void leeMission();
void leeDebriefed();
void leeBuff();
void leeGrounded();
void arthurLeft();
void tauranMission();

//sound effects
void shipAfterburn();
void arthurActivate();
void dialogueWhistle();
void arnoldFloorHit();

void noDialogueEffect();
void exitDialogue();
#endif 