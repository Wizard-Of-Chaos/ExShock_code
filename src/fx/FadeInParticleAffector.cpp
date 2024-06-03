#include "FadeInParticleAffector.h"
#include "IAttributes.h"

namespace irr
{
	namespace scene
	{

		//! constructor
		CParticleFadeInAffector::CParticleFadeInAffector(
			const video::SColor& targetColor, u32 fadeOutTime)
			: IParticleAffector(), StartColor(targetColor)
		{

			fadeInTime = fadeOutTime ? static_cast<f32>(fadeOutTime) : 1.0f;
		}


		//! Affects an array of particles.
		void CParticleFadeInAffector::affect(u32 now, SParticle* particlearray, u32 count)
		{
			if (!Enabled)
				return;
			f32 d = 0.f;
			for (u32 i = 0; i < count; ++i)
			{
				if (now - particlearray[i].startTime < fadeInTime) {
					d = (now - particlearray[i].startTime) / fadeInTime;
					if (d > 1.f) d = 1.f;
					particlearray[i].color = particlearray[i].startColor.getInterpolated(StartColor, d);
				}
			}

			if (d >= 1.f) {
				Enabled = false;
			}
		}

	} // end namespace scene
} // end namespace irr