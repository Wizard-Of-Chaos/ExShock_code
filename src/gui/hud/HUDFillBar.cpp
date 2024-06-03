#include "HUDFillBar.h"
#include "IrrlichtUtils.h"

FillBar::FillBar(IGUIImage* barBg, IGUIImage* barFill, BARFILLDIR dir,
	std::wstring namestr, IGUIImage* lazy, bool nameAndValSameLine, IGUIStaticText* name, IGUIStaticText* bartext) :
	nameStr(namestr), lazy(lazy), name(name), bartext(bartext), fillDir(dir), filler(barFill), background(barBg), nameAndValSameLine(nameAndValSameLine)
{
	scaleAlign(barBg);
	scaleAlign(barFill);
	barBg->setVisible(true);
	barFill->setVisible(true);
	if (lazy) {
		scaleAlign(lazy);
		lazy->setVisible(true);
	}
	if (name) {
		scaleAlign(name);
		name->setVisible(true);
	}
	if (bartext) {
		scaleAlign(bartext);
		bartext->setVisible(true);
	}
	lazylerp = .02f;
}

FillBar::~FillBar()
{
	if (filler) filler->remove();
	if (name) name->remove();
	if (bartext) bartext->remove();
	if (lazy) lazy->remove();
	if (background) background->remove();
}
#include <iostream>

void FillBar::updateBar(const f32 newValue, const f32 max)
{
	if (!background->isVisible()) return;

	switch (fillDir)
	{
	case BARFILLDIR::LEFT:
		m_leftFill(newValue, max);
		break;
	case BARFILLDIR::RIGHT:
		m_rightFill(newValue, max);
		break;
	case BARFILLDIR::UP:
		m_upFill(newValue, max);
		break;
	case BARFILLDIR::DOWN:
		m_downFill(newValue, max);
		break;
	default:
		break;//whuh?
	}
}
void FillBar::m_txtUpdate(const f32& newVal, const f32& max)
{
	if (!name && !bartext) return;
	std::wstring value = wstr(fprecis(newVal, rounding));
	if(includeMax) value += L" / " + wstr(fprecis(max, rounding));
	std::wstring combined = nameStr + L" " + value;
	if (name) {
		if (nameAndValSameLine) {
			name->setText(combined.c_str());
			return;
		}
		else {
			name->setText(nameStr.c_str());
		}
	}
	if (bartext) {
		if (nameAndValSameLine) {
			bartext->setText(combined.c_str());
			return;
		}
		else {
			bartext->setText(value.c_str());
		}
	}
}
void FillBar::m_leftFill(const f32& newVal, const f32& max)
{
	f32 percent = (newVal / max);
	if (percent > 1.f) percent = 1.f;
	if (percent < 0.f) percent = 0.f;
	filler->setDrawBounds(rect<f32>(1.f - percent, 0.f, 1.f, 1.f));
	if (lazy) {
		oldPercent = lazy->getDrawBounds().UpperLeftCorner.X;
		lazy->setDrawBounds(rect<f32>(lerp(oldPercent, percent, lazylerp), 0.f, 1.f, 1.f));
	}
	m_txtUpdate(newVal, max);
}
void FillBar::m_rightFill(const f32& newVal, const f32& max)
{
	f32 percent = (newVal / max);
	if (percent > 1.f) percent = 1.f;
	if (percent < 0.f) percent = 0.f;
	if (lazy) {
		oldPercent = lazy->getDrawBounds().LowerRightCorner.X;
		lazy->setDrawBounds(rect<f32>(0.f, 0.f, lerp(oldPercent, percent, lazylerp), 1.f));
	}
	filler->setDrawBounds(rect<f32>(0.f, 0.f, percent, 1.f));
	m_txtUpdate(newVal, max);
}
void FillBar::m_upFill(const f32& newVal, const f32& max)
{
	f32 percent = (newVal / max);
	if (percent > 1.f) percent = 1.f;
	if (percent < 0.f) percent = 0.f;
	filler->setDrawBounds(rect<f32>(0.f, 1.f-percent, 1.f, 1.f));
	if (lazy) {
		oldPercent = lazy->getDrawBounds().UpperLeftCorner.Y;
		lazy->setDrawBounds(rect<f32>(0.f, lerp(oldPercent, percent, lazylerp), 1.f, 1.f));
	}
	m_txtUpdate(newVal, max);
}
void FillBar::m_downFill(const f32& newVal, const f32& max)
{
	f32 percent = (newVal / max);
	if (percent > 1.f) percent = 1.f;
	if (percent < 0.f) percent = 0.f;
	filler->setDrawBounds(rect<f32>(0.f, 0.f, 1.f, percent));
	if (lazy) {
		oldPercent = lazy->getDrawBounds().LowerRightCorner.Y;
		lazy->setDrawBounds(rect<f32>(0.f, 0.f, 1.f, lerp(oldPercent, percent, lazylerp)));
	}
	m_txtUpdate(newVal, max);
}