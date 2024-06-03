#include "IrrlichtUtils.h"
#include "GeometryCreators.h"
#include "GameFunctions.h"
#include "GameAssets.h"
#include <string>
#include <iostream>
#include <codecvt>
#include <filesystem>


const std::map<BUTTON_COLOR, std::string> buttonColorStrings =
{
	{BCOL_LIGHTBLUE, "lightblue"},
	{BCOL_BLUE, "blue"},
	{BCOL_GREEN, "green"},
	{BCOL_YELLOW, "yellow"},
	{BCOL_RED, "red"},
	{BCOL_ORANGE, "orange"},
	{BCOL_GREYED, "greyed_out"}
};

std::string vecStr(vector3df vec)
{
	return std::to_string(vec.X) + ", " + std::to_string(vec.Y) + ", " + std::to_string(vec.Z);
}

std::string vecStr(btVector3 vec)
{
	return std::to_string(vec.x()) + ", " + std::to_string(vec.y()) + ", " + std::to_string(vec.z());
}

vector3df getNodeForward(ISceneNode* node)
{
	return node->getAbsoluteTransformation().getRotationDegrees().rotationToDirection(vector3df(0, 0, 1));
}

vector3df getNodeBackward(ISceneNode* node)
{
	return -getNodeForward(node);
}

vector3df getNodeUp(ISceneNode* node)
{
	return node->getAbsoluteTransformation().getRotationDegrees().rotationToDirection(vector3df(0, 1, 0));
}

vector3df getNodeDown(ISceneNode* node)
{
	return -getNodeUp(node);
}

vector3df getNodeRight(ISceneNode* node)
{
	return node->getAbsoluteTransformation().getRotationDegrees().rotationToDirection(vector3df(1, 0, 0));
}

vector3df getNodeLeft(ISceneNode* node)
{
	return -getNodeRight(node);
}

vector3df makeVectorInaccurate(vector3df dir, f32 accuracy)
{
	f32 xspread, yspread, zspread;
	xspread = random.frange(-5.f, 5.f);
	yspread = random.frange(-5.f, 5.f);
	zspread = random.frange(-5.f, 5.f);

	xspread /= (accuracy * 100.f);
	yspread /= (accuracy * 100.f);
	zspread /= (accuracy * 100.f);
	vector3df newDir = dir;
	newDir.X += xspread;
	newDir.Y += yspread;
	newDir.Z += zspread;
	return newDir;
}
vector3df makeRotationVectorInaccurate(vector3df rot, f32 maxOff)
{
	f32 xspread, yspread, zspread;
	xspread = random.frange(-maxOff, maxOff);
	yspread = random.frange(-maxOff, maxOff);
	zspread = random.frange(-maxOff, maxOff);
	vector3df newRot = rot;
	newRot.X += xspread;
	newRot.Y += yspread;
	newRot.Z += zspread;

	return newRot;
}

flecs::entity strToId(std::string id)
{
	flecs::entity_t num = std::stoull(id);
	return flecs::entity(game_world->get_world(), num);
}
std::string idToStr(flecs::entity id)
{
	return std::to_string(id.id());
}

void scaleAlign(IGUIElement* elem)
{
	elem->setAlignment(EGUIA_SCALE, EGUIA_SCALE, EGUIA_SCALE, EGUIA_SCALE);
}

void scaleAlign(IGUIImage* img)
{
	img->setAlignment(EGUIA_SCALE, EGUIA_SCALE, EGUIA_SCALE, EGUIA_SCALE);
	img->setScaleImage(true);
}

const stringc DEFAULTFONT10_OUTLINE = "assets/fonts/swansea/10outline.xml";
const stringc DEFAULTFONT12_OUTLINE = "assets/fonts/swansea/12outline.xml";
const stringc DEFAULTFONT14_OUTLINE = "assets/fonts/swansea/14outline.xml";
const stringc DEFAULTFONT16_OUTLINE = "assets/fonts/swansea/16outline.xml";
const stringc DEFAULTFONT18_OUTLINE = "assets/fonts/swansea/18outline.xml";
const stringc DEFAULTFONT20_OUTLINE = "assets/fonts/swansea/20outline.xml";

const stringc DEFAULTFONT10 = "assets/fonts/swansea/10.xml";
const stringc DEFAULTFONT12 = "assets/fonts/swansea/12.xml";
const stringc DEFAULTFONT14 = "assets/fonts/swansea/14.xml";
const stringc DEFAULTFONT16 = "assets/fonts/swansea/16.xml";
const stringc DEFAULTFONT18 = "assets/fonts/swansea/18.xml";
const stringc DEFAULTFONT20 = "assets/fonts/swansea/20.xml";

const stringc DIALOGUEFONT10 = "assets/fonts/qaz_sans/10.xml";
const stringc DIALOGUEFONT12 = "assets/fonts/qaz_sans/12.xml";
const stringc DIALOGUEFONT14 = "assets/fonts/qaz_sans/14.xml";
const stringc DIALOGUEFONT16 = "assets/fonts/qaz_sans/16.xml";
const stringc DIALOGUEFONT18 = "assets/fonts/qaz_sans/18.xml";
const stringc DIALOGUEFONT20 = "assets/fonts/qaz_sans/20.xml";

const stringc ALTFONT10 = "assets/fonts/delta_block/10.xml";
const stringc ALTFONT12 = "assets/fonts/delta_block/12.xml";
const stringc ALTFONT14 = "assets/fonts/delta_block/14.xml";
const stringc ALTFONT16 = "assets/fonts/delta_block/16.xml";
const stringc ALTFONT18 = "assets/fonts/delta_block/18.xml";
const stringc ALTFONT20 = "assets/fonts/delta_block/20.xml";

static const stringc _largeSizing()
{
	u32 width = driver->getScreenSize().Width;
	if (width >= 1900) return "20";
	if (width >= 1400) return "18";
	if (width >= 900) return "16";
	return "14"; //if your screen is less than 900 pixels wide in 2024 or beyond, I have a message for you -- stop running my game on a fucking wristwatch
}
static const stringc _mediumSizing()
{
	u32 width = driver->getScreenSize().Width;
	if (width >= 1900) return "18";
	if (width >= 1400) return "16";
	if (width >= 900) return "14";
	return "12";
}
static const stringc _smallSizing()
{
	u32 width = driver->getScreenSize().Width;
	if (width >= 1900) return "14";
	if (width >= 1400) return "12";
	return "10";
}

static IGUIFont* _sizedFont(stringc which, TEXT_SIZE size, bool outline = false)
{
	stringc str = which;
	if (size == TEXT_SIZE::TXT_LARGE) str += _largeSizing();
	else if (size == TEXT_SIZE::TXT_MED) str += _mediumSizing();
	else str += _smallSizing();
	if (outline) str += "outline";
	str += ".xml";
	return guienv->getFont(str);
}

static IGUIFont* _defaultFont(TEXT_SIZE size)
{
	return _sizedFont("assets/fonts/unitblock/", size);
}
static IGUIFont* _defaultFontOutline(TEXT_SIZE size)
{
	return _sizedFont("assets/fonts/swansea/", size, true);
}
static IGUIFont* _dialogueFont(TEXT_SIZE size)
{
	return _sizedFont("assets/fonts/qaz_sans/", size);
}
static IGUIFont* _altFont(TEXT_SIZE size)
{
	return _sizedFont("assets/fonts/delta_block/", size);
}

void setUIText(IGUIStaticText* text)
{
	scaleAlign(text);
	text->setOverrideColor(SColor(255, 200, 200, 200));
	text->setOverrideFont(_defaultFont(TEXT_SIZE::TXT_LARGE));
	text->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
}

void setUITextSmall(IGUIStaticText* text)
{
	scaleAlign(text);
	text->setOverrideColor(SColor(255, 200, 200, 200));
	text->setOverrideFont(_defaultFont(TEXT_SIZE::TXT_SMALL));
	text->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
}

void setAltUIText(IGUIStaticText* text)
{
	scaleAlign(text);
	text->setOverrideColor(SColor(255, 200, 200, 200));
	text->setOverrideFont(_altFont(TEXT_SIZE::TXT_LARGE));
	text->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);

}
void setAltUITextSmall(IGUIStaticText* text)
{
	scaleAlign(text);
	text->setOverrideColor(SColor(255, 200, 200, 200));
	text->setOverrideFont(_altFont(TEXT_SIZE::TXT_SMALL));
	text->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);

}

void setDialogueText(IGUIStaticText* text)
{
	scaleAlign(text);
	text->setOverrideColor(SColor(255, 200, 200, 200));
	text->setOverrideFont(_dialogueFont(TEXT_SIZE::TXT_LARGE));
	text->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
}

void setHUDText(IGUIStaticText* text)
{
	scaleAlign(text);
	text->setOverrideColor(SColor(255, 5, 255, 156));
	text->setOverrideFont(_defaultFontOutline(TEXT_SIZE::TXT_LARGE));
	text->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
}
void setHUDTextSmall(IGUIStaticText* text)
{
	scaleAlign(text);
	text->setOverrideColor(SColor(255, 5, 255, 156));
	text->setOverrideFont(_defaultFontOutline(TEXT_SIZE::TXT_SMALL));
	text->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
}

void setButtonImg(IGUIButton* button, std::string normal, std::string clicked)
{
	scaleAlign(button);
	button->setImage(driver->getTexture(normal.c_str()));
	button->setPressedImage(driver->getTexture(clicked.c_str()));
	button->setScaleImage(true);
	button->setOverrideFont(_defaultFont(TEXT_SIZE::TXT_LARGE));
}

void setMetalButton(IGUIButton* elem)
{
	scaleAlign(elem);
	elem->setImage(driver->getTexture("assets/ui/button1.png"));
	elem->setPressedImage(driver->getTexture("assets/ui/button2.png"));
	elem->setScaleImage(true);
	elem->setOverrideFont(_defaultFontOutline(TEXT_SIZE::TXT_LARGE));
}

void setHoloButton(IGUIButton* elem, BUTTON_COLOR color)
{
	std::string path = "assets/ui/buttoncolors/";
	path += buttonColorStrings.at(color);
	std::string base = path + "1.png";
	std::string press = path + "2.png";

	setButtonImg(elem, base, press);
	elem->setDrawBorder(false);
	elem->setUseAlphaChannel(true);
	elem->setOverrideFont(_defaultFont(TEXT_SIZE::TXT_LARGE));
}

void setThinHoloButton(IGUIButton* elem, BUTTON_COLOR color)
{
	std::string path = "assets/ui/buttoncolors/thin";
	path += buttonColorStrings.at(color);
	std::string base = path + "1.png";
	std::string press = path + "2.png";

	setButtonImg(elem, base, press);
	elem->setDrawBorder(false);
	elem->setUseAlphaChannel(true);
	elem->setOverrideFont(_defaultFont(TEXT_SIZE::TXT_MED));
}

void setRepairButton(IGUIButton* elem)
{
	scaleAlign(elem);
	elem->setUseAlphaChannel(true);
	elem->setDrawBorder(false);
	elem->setScaleImage(true);

	elem->setImage(driver->getTexture("assets/ui/repair1.png"));
	elem->setPressedImage(driver->getTexture("assets/ui/repair2.png"));
}
void setReloadButton(IGUIButton* elem)
{
	scaleAlign(elem);
	elem->setUseAlphaChannel(true);
	elem->setDrawBorder(false);
	elem->setScaleImage(true);

	elem->setImage(driver->getTexture("assets/ui/reload1.png"));
	elem->setPressedImage(driver->getTexture("assets/ui/reload2.png"));
}

void setLoadoutButton(IGUIButton* elem)
{
	scaleAlign(elem);
	elem->setUseAlphaChannel(true);
	elem->setDrawBorder(false);
	elem->setScaleImage(true);

	elem->setImage(driver->getTexture("assets/ui/loadout1.png"));
	elem->setPressedImage(driver->getTexture("assets/ui/loadout2.png"));
}

ISceneNodeAnimator* getEngineTextureAnim()
{
	return getTextureAnim("assets/effects/ahr_engine/", 30, true);
}
ISceneNodeAnimator* getExplosionTextureAnim(EXPLOSION_TYPE type)
{
	std::string path = "assets/effects/explosiontex/"; //location
	if (type == EXTYPE_TECH) path = "assets/effects/techsplosiontex/";
	else if (type == EXTYPE_ALIEN) path = "assets/effects/aliensplode/";
	return getTextureAnim(path, 16, false, false);
}

ISceneNodeAnimator* getExplosionSphereTextureAnim(EXPLOSION_TYPE type)
{
	std::string path = "assets/effects/explosion/";
	if (type == EXTYPE_TECH) {
		path = "assets/effects/techsplosion/";
	}
	return getTextureAnim(path, 20, false, false);
}

ISceneNodeAnimator* getFireTextureAnim()
{
	return getTextureAnim("assets/effects/flamesparks/", 20, true);
}

ISceneNodeAnimator* getSmokeTextureAnim()
{
	return getTextureAnim("assets/effects/smoke/", 20, true);
}

ISceneNodeAnimator* getBolasTextureAnim()
{
	return getTextureAnim("assets/effects/physlatch/", 18, true);
}

ISceneNodeAnimator* getGasTextureAnim(std::string which)
{
	return getTextureAnim(which, 32, true);
}

ISceneNodeAnimator* getTextureAnim(std::string path, s32 ms, bool loop, bool randomizeStart)
{
	//use a list initially for speed
	std::list<ITexture*> anim;
	for (const auto& file : std::filesystem::directory_iterator(path)) {
		std::string name = file.path().string();
		auto tex = assets->getTexture(name);
		if (tex) anim.push_back(tex);
	}

	//start at a random point in the animation
	if (randomizeStart) {
		u32 startPos = random.urange(0, anim.size());
		for (u32 i = 0; i < startPos; ++i) {
			anim.push_back(anim.front());
			anim.pop_front();
		}
	}

	array<ITexture*> animArray;
	animArray.reallocate(anim.size());
	for (auto& elem : anim) {
		animArray.push_back(elem);
	}
	return smgr->createTextureAnimator(animArray, ms, loop);
}

std::string fprecis(f32 num, s32 round)
{
	std::string ret = std::to_string(num);
	ret.resize(ret.size() - round);
	return ret;
}

std::wstring wstr(std::string str)
{
	return std::wstring(str.begin(), str.end());
}
std::string wstrToStr(std::wstring str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(str);
}

bool smoothGuiMove(IGUIElement* elem, f32 animTime, f32& curTime, position2di desiredPos, position2di startPos, f32 dt)
{
	curTime += dt;
	if (curTime > animTime) curTime = animTime;
	s32 xDiff = startPos.X - desiredPos.X;
	s32 yDiff = startPos.Y - desiredPos.Y;
	s32 xMove = (s32)((dt / animTime) * xDiff);
	s32 yMove = (s32)((dt / animTime) * yDiff);
	elem->move(vector2di(xMove, yMove));
	if (animTime == curTime) {
		curTime = 0;
		return false;
	}
	return true;
}

bool smoothFade(IGUIElement* elem, f32 animTime, f32& curTime, f32 desiredAlpha, f32 startAlpha, f32 dt)
{
	return true;
}

position2di screenCoords(
	const core::vector3df& pos3d, const ICameraSceneNode* camera, bool& isBehind, bool useViewPort)
{
	if (!smgr || !driver)
		return position2di(-1000, -1000);

	if (!camera)
		camera = smgr->getActiveCamera();

	if (!camera)
		return position2di(-1000, -1000);

	dimension2du dim;
	if (useViewPort)
		dim.set(driver->getViewPort().getWidth(), driver->getViewPort().getHeight());
	else
		dim = (driver->getCurrentRenderTargetSize());

	dim.Width /= 2;
	dim.Height /= 2;

	matrix4 trans = camera->getProjectionMatrix();
	trans *= camera->getViewMatrix();

	f32 transformedPos[4] = { pos3d.X, pos3d.Y, pos3d.Z, 1.0f };

	trans.multiplyWith1x4Matrix(transformedPos);

	bool behind = transformedPos[3] < 0;

	const f32 zDiv = transformedPos[3] == 0.0f ? 1.0f :
		core::reciprocal(transformedPos[3]);

	position2di returnedVal = position2di(
		dim.Width + core::round32(dim.Width * (transformedPos[0] * zDiv)),
		dim.Height - core::round32(dim.Height * (transformedPos[1] * zDiv)));

	if (behind) {
		//invert the sucker
		returnedVal.X = dim.Width - returnedVal.X;
		returnedVal.Y = dim.Height - returnedVal.Y;
	}

	isBehind = behind;

	return returnedVal;
}

IMotionTrailSceneNode* addMotionTrailTimeBased(
	ISceneNode* attach, u32 segmentCount, u32 lifetime,
	bool useCenterVertex, const core::vector3df& lockAxis)
{
	if (!attach)
		return 0;

	bool hasLockedAxis;
	if (lockAxis == core::vector3df(0))
		hasLockedAxis = false;
	else
		hasLockedAxis = true;

	CMotionTrailSceneNode* motionTrail = new CMotionTrailSceneNode(
		attach,						//parent
		smgr,						//smgr
		-1,							//id
		core::vector3df(0, 0, 0),	//position
		segmentCount,				//segmentCount
		true,						//timeBased
		true,						//globalSpace
		useCenterVertex,			//useCenterVertex
		hasLockedAxis ? EMTF_AXIS : EMTF_CAMERA);	//facingDirection

	motionTrail->setLifetime(lifetime);
	if (hasLockedAxis)
		motionTrail->setLockedAxis(lockAxis);

	CSceneNodeAnimatorMotionTrail* anim = new CSceneNodeAnimatorMotionTrail(attach);
	motionTrail->addAnimator(anim);
	anim->drop();

	motionTrail->drop();

	return motionTrail;
}

IMotionTrailSceneNode* addMotionTrailLengthBased(
	ISceneNode* attach, u32 segmentCount, f32 length,
	bool useCenterVertex, const core::vector3df& lockAxis)
{
	if (!attach)	//without attaching, this doesn't make sense...
		return 0;

	bool hasLockedAxis;
	if (lockAxis == core::vector3df(0))
		hasLockedAxis = false;
	else
		hasLockedAxis = true;

	CMotionTrailSceneNode* motionTrail = new CMotionTrailSceneNode(
		attach,						//parent
		smgr,						//smgr
		-1,							//id
		core::vector3df(0, 0, 0),	//position
		segmentCount,				//segmentCount
		false,						//timeBased
		true,						//globalSpace
		useCenterVertex,			//useCenterVertex
		hasLockedAxis ? EMTF_AXIS : EMTF_CAMERA);	//facingDirection

	motionTrail->setLength(length);
	if (hasLockedAxis)
		motionTrail->setLockedAxis(lockAxis);

	CSceneNodeAnimatorMotionTrail* anim = new CSceneNodeAnimatorMotionTrail(attach);
	motionTrail->addAnimator(anim);
	anim->drop();

	motionTrail->drop();

	return motionTrail;
}

CBeamNode* createBeam(ISceneNode* parent, s32 id, ITexture* beam, ITexture* cap)
{
	ISceneNode* par;
	if (parent) par = parent;
	else par = smgr->getRootSceneNode();
	CBeamNode* node = new CBeamNode(par, smgr, id, beam, cap);
	node->drop();
	return node;
}

IMeshSceneNode* createLightWithVolumeMesh(const u32 layers, const u32 pointsPerSphere, f32 radius,
	const video::SColor centerColor, const video::SColor outerColor,
	ISceneNode* parent, s32 id, core::vector3df position, core::vector3df rotation, core::vector3df scale)
{
	if (!smgr) return nullptr;
	auto mesh = createPointVolumeLightMesh(layers, pointsPerSphere, centerColor, outerColor);
	assets->setMesh(std::to_string(device->getTimer()->getRealTime()), mesh);
	IMeshSceneNode* msh = smgr->addMeshSceneNode(
		mesh, parent, id, position, rotation, scale
	);

	msh->setMaterialType(video::EMT_TRANSPARENT_VERTEX_ALPHA);
	msh->setMaterialFlag(video::EMF_LIGHTING, false);
	auto node = smgr->addLightSceneNode(msh, core::vector3df(0, 0, 0), video::SColorf(centerColor), radius, ID_IsNotSelectable);
	node->setName("light");
	return msh;
}

IMeshSceneNode* createCloudMeshNode(std::string meshLocation, const u32 layers, const video::SColor centerColor, const video::SColor outerColor,
	vector3df position, vector3df rotation, vector3df scale, s32 id, ISceneNode* parent)
{
	bool firstload;
	IMesh* newMesh = createCloudVolumeMesh(driver, assets->getMesh(meshLocation, firstload), layers, centerColor, outerColor);
	assets->setMesh(std::to_string(device->getTimer()->getRealTime()), newMesh); //so we can still clear it later
	IMeshSceneNode* meshNode = smgr->addMeshSceneNode(newMesh, parent, id, position, rotation, scale);
	meshNode->setMaterialType(EMT_TRANSPARENT_VERTEX_ALPHA);
	meshNode->setMaterialFlag(EMF_LIGHTING, false);
	meshNode->setVisible(true);
	return meshNode;
}