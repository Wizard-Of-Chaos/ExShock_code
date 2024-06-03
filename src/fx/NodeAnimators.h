#pragma once
#ifndef ISCENENODEFADEINANIMATOR_H
#define ISCENENODEFADEINANIMATOR_H

#include "ISceneNodeAnimator.h"
#include "ISceneNode.h"
#include "SColor.h"

namespace irr
{
	namespace scene
	{
		class CSceneNodeFadeInAnimator : public ISceneNodeAnimator
		{
		public:
			CSceneNodeFadeInAnimator(u32 startms, u32 fadeTime, ISceneManager* smgr = 0, video::SColor targColor = video::SColor(255, 255, 255, 255)) : targColor(targColor), startTime(startms), smgr(smgr)
			{
				fadeInTime = fadeTime ? static_cast<f32>(fadeTime) : 1.0f;
			}
			virtual void animateNode(ISceneNode* node, u32 timeMs);
			virtual ISceneNodeAnimator* createClone(ISceneNode* node, ISceneManager* newManager = 0);
			virtual bool hasFinished(void) const;
		private:
			bool done = false;
			video::SColor startColor = video::SColor(0, 0, 0, 0);
			video::SColor targColor;
			f32 fadeInTime;
			u32 startTime;
			irr::scene::ISceneManager* smgr=0;
		};

		class CSceneNodeScalePulseAnimator : public ISceneNodeAnimator
		{
		public:
			CSceneNodeScalePulseAnimator(u32 startms, core::vector3df startScale = core::vector3df(1, 1, 1), core::vector3df endScale = core::vector3df(5, 5, 5), u32 timePerPulse = 500, bool looped = false, ISceneManager* smgr = 0)
				: minScale(startScale), maxScale(endScale), smgr(smgr), then(startms), pulseTime(timePerPulse), looped(looped)
			{
				//pulseTime = timePerPulse ? static_cast<f32>(timePerPulse) : 5.0f;
			}
			virtual void animateNode(ISceneNode* node, u32 timeMs);
			virtual ISceneNodeAnimator* createClone(ISceneNode* node, ISceneManager* newManager = 0);
			virtual bool hasFinished(void) const;
		private:
			core::vector3df minScale;
			core::vector3df maxScale;
			u32 pulseTime;
			u32 then = 0;
			u32 dt = 0;
			bool looped = false;
			bool done = false;
			bool isExpanding = true;
			irr::scene::ISceneManager* smgr = 0;
		};

		class CScaleToNothingAndDeleteAnimator : public ISceneNodeAnimator
		{
		public:
			CScaleToNothingAndDeleteAnimator(u32 startms, core::vector3df initialScale = core::vector3df(1.f), u32 timeToFade = 1000U, ISceneManager* smgr = 0) 
				: then(startms), fadeTime(timeToFade), smgr(smgr), initialScale(initialScale) {}
		private:
			virtual void animateNode(ISceneNode* node, u32 timeMs);
			virtual ISceneNodeAnimator* createClone(ISceneNode* node, ISceneManager* newManager = 0);
			virtual bool hasFinished(void) const;
			core::vector3df initialScale;
			u32 then = 0;
			u32 fadeTime;
			bool done = false;
			irr::scene::ISceneManager* smgr = 0;

		};
	}
}
#endif