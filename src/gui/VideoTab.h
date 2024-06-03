#pragma once
#ifndef VIDEOTAB_H
#define VIDEOTAB_H
#include "BaseHeader.h"
#include "SettingsTab.h"

class VideoTab : public SettingsTab
{
	public:
		virtual void show();
		void build(IGUIElement* root, VideoConfig* cfg);
		bool onToggle(const SEvent& event);
		bool onParticles(const SEvent& event);
		bool onRenderDist(const SEvent& event);
		bool onResolution(const SEvent& event);
	private:
		VideoConfig* vConfig;

		IGUIButton* renderDist;
		IGUIButton* aliasing;
		IGUIButton* fullscreen;
		IGUIButton* vsync;
		IGUIButton* shadows;
		IGUIButton* particles;
		IGUIButton* filtering;
		IGUIButton* anisotropic;
		IGUIButton* bump;
		IGUIButton* resolution;
};

#endif 