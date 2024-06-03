#pragma once
#ifndef HUDFILLBAR_H
#define HUDFILLBAR_H
#include "BaseHeader.h"

//IMPORTANT: this is the direction the bar *FILLS UP IN.* This means that health, dropping from 100 to 80 and a bar fill of RIGHT, means the
//bar decreases to the left.
enum class BARFILLDIR
{
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class FillBar
{
	public:
		FillBar() {}
		FillBar(IGUIImage* barBg, IGUIImage* barFill, BARFILLDIR dir, 
			std::wstring namestr = L"", IGUIImage* lazy = nullptr, bool nameAndValSameLine=false, IGUIStaticText* name = nullptr, IGUIStaticText* bartext = nullptr);
		~FillBar();
		void updateBar(const f32 val, const f32 max);
		void togVis(const bool vis) { background->setVisible(vis); }
		void setLazyLerp(const f32 lerp) { lazylerp = lerp; }
		IGUIStaticText* name = nullptr;
		void setRounding(u32 round = 5) { rounding = round; }
		void setIncludeMaxTxt(bool include = true) { includeMax = include; }
		std::wstring nameStr = L"";

	private:
		u32 rounding = 5;
		bool includeMax = false;
		void m_leftFill(const f32& newVal, const f32& max);
		void m_rightFill(const f32& newVal, const f32& max);
		void m_upFill(const f32& newVal, const f32& max);
		void m_downFill(const f32& newVal, const f32& max);
		void m_txtUpdate(const f32& newVal, const f32& max);
		bool nameAndValSameLine;
		BARFILLDIR fillDir=BARFILLDIR::RIGHT;
		IGUIImage* background = nullptr;
		IGUIImage* filler = nullptr;
		IGUIImage* lazy = nullptr;
		//IGUIStaticText* name = nullptr;
		IGUIStaticText* bartext = nullptr;
		f32 lazylerp;
		f32 oldPercent = 1.f;
};

#endif 