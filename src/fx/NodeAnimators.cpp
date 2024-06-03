#include "NodeAnimators.h"
#include "ISceneManager.h"
#include "IMeshManipulator.h"
#include "IMeshSceneNode.h"

namespace irr
{
	namespace scene
	{
		void CSceneNodeFadeInAnimator::animateNode(ISceneNode* node, u32 timeMs)
		{
			if (done || !node || !smgr) return;
			if (node->getType() != ESNT_MESH) {
				done = true;
				return;
			}
			IMeshSceneNode* mshNode = (IMeshSceneNode*)node;
			f32 d = 0.f;
			d = (f32)(timeMs - startTime) / fadeInTime;
			if (d >= 1.f) {
				d = 1.f;
				done = true;
			}
			auto manip = smgr->getMeshManipulator();
			u32 desiredAlpha = (u32)(d * 255);
			//std::cout << "desired alph: " << desiredAlpha << std::endl;
			manip->setVertexColorAlpha(mshNode->getMesh(), desiredAlpha);
		}
		ISceneNodeAnimator* CSceneNodeFadeInAnimator::createClone(ISceneNode* node, ISceneManager* newManager)
		{
			CSceneNodeFadeInAnimator* anim = new CSceneNodeFadeInAnimator(startTime, fadeInTime, newManager, targColor);
			return anim;
		}
		bool CSceneNodeFadeInAnimator::hasFinished(void) const
		{
			return done;
		}

		void CSceneNodeScalePulseAnimator::animateNode(ISceneNode* node, u32 timeMs)
		{
			u32 diff = timeMs - then;
			dt += diff;
			then = timeMs;

			f32 percentage = ((f32)dt / (f32)pulseTime);
			core::vector3df startScale, desiredScale;
			if (isExpanding) {
				desiredScale = maxScale;
				startScale = minScale;
			}
			else {
				desiredScale = minScale;
				startScale = maxScale;
			}
			core::vector3df newScale = core::lerp(desiredScale, startScale, percentage);
			node->setScale(newScale);
			if (dt >= pulseTime) {
				isExpanding = !isExpanding;
				dt = 0;
				if (!looped) done = true;
			}
		}
		ISceneNodeAnimator* CSceneNodeScalePulseAnimator::createClone(ISceneNode* node, ISceneManager* newManager)
		{
			CSceneNodeScalePulseAnimator* anim = new CSceneNodeScalePulseAnimator(then, minScale, maxScale, pulseTime, looped, smgr);
			return anim;
		}
		bool CSceneNodeScalePulseAnimator::hasFinished(void) const
		{
			return done;
		}

		void CScaleToNothingAndDeleteAnimator::animateNode(ISceneNode* node, u32 timeMs)
		{
			f32 completion = (f32)((f32)(timeMs - then) / (f32)fadeTime);
			core::vector3df newScale = core::lerp(initialScale, core::vector3df(0.f), completion);
			node->setScale(newScale);

			if (timeMs > (then + fadeTime)) {
				done = true;
				if (node && smgr) {
					smgr->addToDeletionQueue(node);
				}
			}
		}

		ISceneNodeAnimator* CScaleToNothingAndDeleteAnimator::createClone(ISceneNode* node, ISceneManager* newManager)
		{
			return new CScaleToNothingAndDeleteAnimator(then, initialScale, fadeTime, newManager);
		}

		bool CScaleToNothingAndDeleteAnimator::hasFinished(void) const
		{
			return done;
		}
	}//end namespace scene
}//end namespace irr