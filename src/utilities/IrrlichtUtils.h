#pragma once

#ifndef IRRLICHTUTILS_H
#define IRRLICHTUTILS_H
#include "BaseHeader.h"
#include "BeamSceneNode.h"
#include "MotionTrailSceneNode.h"
#include <map>
/*
* Utility functions for use with Irrlicht nodes to determine direction.
* Also includes functions to yank out the EntityId associated with an Irrlicht node.
*/

//The different types of ID an irrlicht node might have. Used to determine whether or not a player can
//"select" the object (or whether or not an AI can avoid the object). Tack on more if you need them.
enum SELECTIONS
{
	ID_IsNotSelectable = 1,
	ID_IsSelectable = 2,
	ID_IsAvoidable = 4
};

enum class TEXT_SIZE
{
	TXT_SMALL,
	TXT_MED,
	TXT_LARGE
};

enum BUTTON_COLOR
{
	BCOL_LIGHTBLUE,
	BCOL_BLUE,
	BCOL_GREEN,
	BCOL_YELLOW,
	BCOL_RED,
	BCOL_ORANGE,
	BCOL_GREYED,
	BCOL_MAX
};

enum EXPLOSION_TYPE
{
	EXTYPE_REGULAR,
	EXTYPE_TECH,
	EXTYPE_ALIEN
};

extern const std::map<BUTTON_COLOR, std::string> buttonColorStrings;

std::string vecStr(vector3df vec);
std::string vecStr(btVector3 vec);

CBeamNode* createBeam(ISceneNode* parent = 0, s32 id = -1, ITexture* beam = 0, ITexture* cap = 0);

//Normalized forward vector for a given node.
vector3df getNodeForward(ISceneNode* node);
//Normalized backward vector for a given node.
vector3df getNodeBackward(ISceneNode* node);
//Normalized left vector for a given node.
vector3df getNodeLeft(ISceneNode* node);
//Normalized right vector for a given node.
vector3df getNodeRight(ISceneNode* node);
//Normalized up vector for a given node.
vector3df getNodeUp(ISceneNode* node);
//Normalized down vector for a given node.
vector3df getNodeDown(ISceneNode* node);

//This function will return a direction vector that is slightly off from the initial vector, modified by the accuracy float given (lower is crappier accuracy).
vector3df makeVectorInaccurate(vector3df initVec, f32 accuracyFactor);
//Same as the other inaccurate vector, but for rotations. The accuracy factor here is higher = less accurate.
vector3df makeRotationVectorInaccurate(vector3df rot, f32 maxOff);

//Transforms a string to an EntityId.
flecs::entity strToId(std::string id);
//Transforms an EntityId to a string.
std::string idToStr(flecs::entity id);
//Convenience function to call on a GUI element to force it to scale with its root node.
void scaleAlign(IGUIElement* elem);
//Convenience function to call on a GUI image to force it to scale with its root node.
void scaleAlign(IGUIImage* img);
//Sets the text element to the current UI font and color.
void setUIText(IGUIStaticText* text);
void setUITextSmall(IGUIStaticText* text);
void setAltUIText(IGUIStaticText* text);
void setAltUITextSmall(IGUIStaticText* text);
//Sets the text element to the current dialogue font and color.
void setDialogueText(IGUIStaticText* text);
//Sets the text element to the current HUD font and color.
void setHUDText(IGUIStaticText* text);
void setHUDTextSmall(IGUIStaticText* text);
//Sets the image of a button to the textures pointed at by normal and clicked.
void setButtonImg(IGUIButton* button, std::string normal, std::string clicked);
//Sets the button to a metallic button.
void setMetalButton(IGUIButton* elem);
//Sets the button to a holo-button, without a background if bg is false and with a background otherwise.
void setHoloButton(IGUIButton* elem, BUTTON_COLOR color=BCOL_LIGHTBLUE);
//Sets the button to the repair button icon.
void setRepairButton(IGUIButton* elem);
//Sets the button to the reload button icon.
void setReloadButton(IGUIButton* elem);
//Sets the button to the loadout button icon.
void setLoadoutButton(IGUIButton* elem);

//Sets the button to a thin holo-button with the given color.
void setThinHoloButton(IGUIButton* elem, BUTTON_COLOR color = BCOL_LIGHTBLUE);

//Gets a texture animator for an engine.
ISceneNodeAnimator* getEngineTextureAnim();
//Gets a texture animator for an explosion.
ISceneNodeAnimator* getExplosionTextureAnim(EXPLOSION_TYPE type=EXTYPE_REGULAR);
//Gets a texture animator for an explosion sphere.
ISceneNodeAnimator* getExplosionSphereTextureAnim(EXPLOSION_TYPE type = EXTYPE_REGULAR);
//Gets a texture animator for a ship on fire.
ISceneNodeAnimator* getFireTextureAnim();
ISceneNodeAnimator* getSmokeTextureAnim();
//Gets a texture animator for a gravity bolas effect.
ISceneNodeAnimator* getBolasTextureAnim();
//Gets a texture animator for a gas cloud.
ISceneNodeAnimator* getGasTextureAnim(std::string which);
//Gets a texture animator pointed at by the given path. Sets animation speed and loop as applicable.
ISceneNodeAnimator* getTextureAnim(std::string path, s32 ms =16, bool loop=false, bool randomizeStart=true);

IMotionTrailSceneNode* addMotionTrailTimeBased(ISceneNode* attachedTo, u32 segmentCount=32U, u32 lifetime=1000U, bool useCenterVertex = false, const vector3df& lockAxis=vector3df(0));
IMotionTrailSceneNode* addMotionTrailLengthBased(ISceneNode* attachedTo, u32 segmentCount = 32U, f32 length = 10.f, bool useCenterVertex = false, const vector3df& lockAxis = vector3df(0));

/*
* Smoothly moves the GUI element from one position to the next.
* curTime is assumed to be a timer stored somewhere by whatever is calling this function. 
*/
bool smoothGuiMove(IGUIElement* elem, f32 animTime, f32& curTime, position2di desiredPos, position2di startPos, f32 dt);

bool smoothFade(IGUIElement* elem, f32 animTime, f32& curTime, f32 desiredAlpha, f32 startAlpha, f32 dt);

position2di screenCoords(
	const core::vector3df& pos3d, const ICameraSceneNode* camera, bool& isBehind, bool useViewPort = false);

//Convenience funtion to return a string that is a rounded float. Thank you, GUI work.
//Chops off the last so-many digits, specified by "round".
std::string fprecis(f32 num, s32 round);

//Converts a string to a std::wstring (irrlicht text only accepts wide characters).
std::wstring wstr(std::string str);

//Converts a wide-string back to an std::string (might screw up).
std::string wstrToStr(std::wstring str);

IMeshSceneNode* createLightWithVolumeMesh(const u32 layers = 64, const u32 pointsPerSphere = 8, f32 radius = 100.f,
	const video::SColor centerColor = video::SColor(50, 255, 255, 255), const video::SColor outerColor = video::SColor(0, 255, 255, 255),
	ISceneNode* parent = 0, s32 id = -1, core::vector3df position = core::vector3df(0, 0, 0), core::vector3df rotation = core::vector3df(0, 0, 0),
	core::vector3df scale = core::vector3df(1, 1, 1));

IMeshSceneNode* createCloudMeshNode(std::string meshLocation, const u32 layers = 64, 
	const video::SColor centerColor = SColor(25, 255, 255, 255), const video::SColor outerColor = SColor(0, 255, 255, 255),
	vector3df position=vector3df(0,0,0), vector3df rotation = vector3df(0, 0, 0), vector3df scale = vector3df(0, 0, 0),
	s32 id=-1, ISceneNode* parent=0);
#endif