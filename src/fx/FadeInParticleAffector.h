#pragma once
#ifndef __C_PARTICLE_FADE_OUT_AFFECTOR_H_INCLUDED__
#define __C_PARTICLE_FADE_OUT_AFFECTOR_H_INCLUDED__

#include "IParticleFadeOutAffector.h"
#include "SColor.h"

namespace irr
{
	namespace scene
	{

		//! Particle Affector for fading in a color
		class CParticleFadeInAffector : public IParticleAffector
		{
		public:

			CParticleFadeInAffector(const video::SColor& targetColor, u32 fadeOutTime);

			//! Affects a particle.
			virtual void affect(u32 now, SParticle* particlearray, u32 count);

			//! Sets the targetColor, i.e. the color the particles will interpolate
			//! to over time.
			virtual void setTargetColor(const video::SColor& targetColor) { StartColor = targetColor; }

			//! Sets the amount of time it takes for each particle to fade out.
			virtual void setFadeInTime(u32 fadeInTime) { fadeInTime = fadeInTime ? static_cast<f32>(fadeInTime) : 1.0f; }

			//! Sets the targetColor, i.e. the color the particles will interpolate
			//! to over time.
			virtual const video::SColor& getTargetColor() const { return StartColor; }

			//! Sets the amount of time it takes for each particle to fade out.
			virtual u32 getFadeOutTime() const { return static_cast<u32>(fadeInTime); }

			//! Get emitter type
			virtual E_PARTICLE_AFFECTOR_TYPE getType() const { return EPAT_NONE; }

		private:
			video::SColor StartColor;
			f32 fadeInTime;
		};

	} // end namespace scene
} // end namespace irr


#endif