#include "HUDHealthBar.h"
#include "GameController.h"
#include "GameStateController.h"
#include "HUDComms.h"
#include "BulletRigidBodyComponent.h"
#include "GameAssets.h"
#include "WeaponInfoComponent.h"
#include "HealthComponent.h"
#include "LoadoutData.h"
#include "GameController.h"
#include "Scenario.h"
#include "GameFunctions.h"
#include "PlayerComponent.h"
#include <string>

HUDResources::HUDResources(IGUIElement* root, flecs::entity id) : HUDElement(root)
{
	sideWidthRatio = (168.f / 960.f);
	sideHeightRatio = (141.f / 540.f);
	centerWidthRatio = (668.f / 1920.f);
	centerHeightRatio = (178.f / 1080.f);

	rect<s32> screen = root->getRelativePosition();
	sideSize = dimension2du(screen.getWidth() * sideWidthRatio, screen.getHeight() * sideHeightRatio);
	dimension2du commSize(screen.getWidth() * (378.f / 1920.f), screen.getHeight() * (378.f / 1080.f));
	centerSize = dimension2du(screen.getWidth() * centerWidthRatio, screen.getHeight() * centerHeightRatio);

	dimension2du hitSize(screen.getHeight() / 2.25, screen.getHeight() / 2.25);
	position2di hitPos(screen.getWidth() / 2, screen.getHeight() / 2.25);
	hitPos.X -= hitSize.Width / 2;
	hitPos.Y -= hitSize.Height / 2;
	for (u32 i = 0; i < 4; ++i) {
		hpHit[i] = guienv->addImage(rect<s32>(hitPos, hitSize), root);
		scaleAlign(hpHit[i]);
		hpHit[i]->setUseAlphaChannel(true);
		hpHit[i]->setColor(SColor(0, 255, 255, 255));
	}
	hpHit[0]->setImage(assets->getTexture("assets/hud/healthsmack_top.png"));
	hpHit[1]->setImage(assets->getTexture("assets/hud/healthsmack_bottom.png"));
	hpHit[2]->setImage(assets->getTexture("assets/hud/healthsmack_left.png"));
	hpHit[3]->setImage(assets->getTexture("assets/hud/healthsmack_right.png"));


	lBg = guienv->addImage(rect<s32>(position2di(0, screen.getHeight() - commSize.Height), commSize), root);
	lBg->setImage(assets->getTexture("assets/hud/comm_panel.png"));
	scaleAlign(lBg);

	comms = new HUDComms(root, lBg);

	//rBg = guienv->addImage(rect<s32>(position2di(screen.getWidth()-sideSize.Width, screen.getHeight() - sideSize.Height), sideSize), root);
	//rBg->setImage(assets->getHUDAsset("hudSideRight"));
	//scaleAlign(rBg);

	centBg = guienv->addImage(rect<s32>(position2di((screen.getWidth() - centerSize.Width) / 2.f, screen.getHeight() - centerSize.Height), centerSize), root);
	centBg->setImage(assets->getTexture("assets/hud/hp_panel.png"));
	scaleAlign(centBg);

	dimension2du barBgSize(screen.getWidth() * (558.f / 1920.f), screen.getHeight() * (48.f / 1080.f));

	//shield
	position2di barPos(screen.getWidth() * (55.f/1920.f), screen.getHeight() * (39.f/1080.f));
	auto barBg = guienv->addImage(rect<s32>(barPos, barBgSize), centBg);
	barBg->setImage(assets->getTexture("assets/hud/bar_health-shields.png"));
	scaleAlign(barBg);
	auto lazy = guienv->addImage(rect<s32>(vector2di(0, 0), barBgSize), barBg);
	lazy->setImage(assets->getTexture("assets/hud/bar_flash.png"));
	scaleAlign(lazy);
	auto barFill = guienv->addImage(rect<s32>(vector2di(0, 0), barBgSize), barBg);
	barFill->setImage(assets->getTexture("assets/hud/bar_fill_shields.png"));
	auto nameTxt = guienv->addStaticText(L"", rect<s32>(vector2di(0, 0), barBgSize), false, true, barBg);
	setHUDText(nameTxt);
	shields = new FillBar(barBg, barFill, BARFILLDIR::RIGHT, L"Shields", lazy, false, nullptr, nameTxt);

	//health
	barPos = position2di(screen.getWidth() * (55.f / 1920.f), screen.getHeight() * (89.f / 1080.f));
	barBg = guienv->addImage(rect<s32>(barPos, barBgSize), centBg);
	barBg->setImage(assets->getTexture("assets/hud/bar_health-shields.png"));
	scaleAlign(barBg);
	lazy = guienv->addImage(rect<s32>(vector2di(0, 0), barBgSize), barBg);
	lazy->setImage(assets->getTexture("assets/hud/bar_flash.png"));
	scaleAlign(lazy);
	barFill = guienv->addImage(rect<s32>(vector2di(0, 0), barBgSize), barBg);
	barFill->setImage(assets->getTexture("assets/hud/bar_fill_health.png"));
	nameTxt = guienv->addStaticText(L"", rect<s32>(vector2di(0, 0), barBgSize), false, true, barBg);
	setHUDText(nameTxt);
	hp = new FillBar(barBg, barFill, BARFILLDIR::RIGHT, L"Hull", lazy, false, nullptr, nameTxt);

	barBgSize = dimension2du(screen.getWidth() * (188.f / 1920.f), screen.getHeight() * (28.f / 1080.f));

	//energy
	barPos= position2di(0, screen.getHeight() * -(32.f / 1080.f));
	barBg = guienv->addImage(rect<s32>(barPos, barBgSize), centBg);
	barBg->setImage(assets->getTexture("assets/hud/bar_speed-energy.png"));
	scaleAlign(barBg);
	barBg->setNotClipped(true);
	barFill = guienv->addImage(rect<s32>(vector2di(0, 0), barBgSize), barBg);
	barFill->setImage(assets->getTexture("assets/hud/bar_fill_energy.png"));
	nameTxt = guienv->addStaticText(L"", rect<s32>(vector2di(-(s32)(barBgSize.Width)/2.f, screen.getHeight() * -(32.f/1080.f)), barBgSize + dimension2du(barBgSize.Width, 0.f)), false, true, barBg);
	nameTxt->setNotClipped(true);
	setHUDTextSmall(nameTxt);
	energy = new FillBar(barBg, barFill, BARFILLDIR::RIGHT, L"Energy", nullptr, true, nameTxt);

	//speed
	barPos = position2di(screen.getWidth() * (480.f/1920.f), screen.getHeight() * -(30.f / 1080.f));
	barBg = guienv->addImage(rect<s32>(barPos, barBgSize), centBg);
	barBg->setImage(assets->getTexture("assets/hud/bar_speed-energy.png"));
	scaleAlign(barBg);
	barBg->setNotClipped(true);
	barFill = guienv->addImage(rect<s32>(vector2di(0, 0), barBgSize), barBg);
	barFill->setImage(assets->getTexture("assets/hud/bar_fill_speed.png"));
	nameTxt = guienv->addStaticText(L"", rect<s32>(vector2di(-(s32)(barBgSize.Width) / 2.f, screen.getHeight() * -(30.f / 1080.f)), barBgSize + dimension2du(barBgSize.Width, 0.f)), false, true, barBg);
	nameTxt->setNotClipped(true);
	setHUDTextSmall(nameTxt);
	velocity = new FillBar(barBg, barFill, BARFILLDIR::LEFT, L"Speed", nullptr, true, nameTxt);



	auto hards = id.get<HardpointComponent>();
	dimension2du wepBgSize(screen.getWidth() * (378.f / 1920.f), screen.getHeight() * (70.f / 1080.f));
	dimension2du ammoBarSize(screen.getWidth() * (23.f / 1920.f), screen.getHeight() * (54.f / 1080.f));
	//u32 txtHeight = sideSize.Height * .1f;
	//dimension2du wepTxtSize(sideSize.Width * .8f, sideSize.Height * .1f);

	u32 baseTxtHeight = (u32)(screen.getHeight() * (660.f / 1080.f));
	u32 txtWidth = (u32)(screen.getWidth() * (1542.f / 1920.f));
	u32 baseHeight = (u32)(screen.getHeight() * (70.f / 1080.f));

	for (u32 i = 0; i < hards->hardpointCount; ++i) {
		if (!hards->weapons[i].is_alive()) continue;

		auto wepInfo = hards->weapons[i].get<WeaponInfoComponent>();
		if (!wepInfo->usesAmmunition) continue;
		rect<s32> bgSize(vector2di(txtWidth, baseTxtHeight + baseHeight * i), wepBgSize);
		auto wepBg = guienv->addImage(bgSize, root);
		wepBg->setImage(assets->getTexture("assets/hud/gun_panel.png"));
		scaleAlign(wepBg);

		auto txt = guienv->addStaticText(wstr(weaponData[wepInfo->wepDataId]->name).c_str(), rect<s32>(vector2di(0,0), wepBgSize), false, true, wepBg);
		setHUDTextSmall(txt);
		auto wepAmmoBg = guienv->addImage(rect<s32>(vector2di(screen.getWidth() * -(23.f / 1920.f), screen.getHeight() * (8.f / 1080.f)), ammoBarSize), wepBg);
		scaleAlign(wepAmmoBg);
		wepAmmoBg->setNotClipped(true);
		wepAmmoBg->setImage(assets->getTexture("assets/hud/bar_gunammo.png"));
		auto wepAmmoFill = guienv->addImage(rect<s32>(vector2di(0,0), ammoBarSize), wepAmmoBg);
		scaleAlign(wepAmmoFill);
		wepAmmoFill->setImage(assets->getTexture("assets/hud/bar_fill_gunammo.png"));
		IGUIStaticText* ammoTxt = nullptr;
		ammoBars[i] = new FillBar(wepAmmoBg, wepAmmoFill, BARFILLDIR::UP, wstr(weaponData[wepInfo->wepDataId]->name), nullptr, true, txt);
		ammoBars[i]->setRounding(7);
	}
	if (hards->heavyWeapon.is_alive()) {
		auto wepInfo = hards->heavyWeapon.get<WeaponInfoComponent>();
		if (wepInfo->usesAmmunition) {
			rect<s32> bgSize(vector2di(txtWidth, baseTxtHeight - baseHeight), wepBgSize);
			auto wepBg = guienv->addImage(bgSize, root);
			wepBg->setImage(assets->getTexture("assets/hud/gun_panel.png"));
			scaleAlign(wepBg);

			auto txt = guienv->addStaticText(wstr(heavyWeaponData[wepInfo->wepDataId]->name).c_str(), rect<s32>(vector2di(0, 0), wepBgSize), false, true, wepBg);
			setHUDTextSmall(txt);
			auto wepAmmoBg = guienv->addImage(rect<s32>(vector2di(screen.getWidth() * -(23.f / 1920.f), screen.getHeight() * (8.f/1080.f)), ammoBarSize), wepBg);
			scaleAlign(wepAmmoBg);
			wepAmmoBg->setNotClipped(true);
			wepAmmoBg->setImage(assets->getTexture("assets/hud/bar_gunammo.png"));
			auto wepAmmoFill = guienv->addImage(rect<s32>(vector2di(0, 0), ammoBarSize), wepAmmoBg);
			scaleAlign(wepAmmoFill);
			wepAmmoFill->setImage(assets->getTexture("assets/hud/bar_fill_gunammo.png"));

			IGUIStaticText* ammoTxt = nullptr;
			hvyBar = new FillBar(wepAmmoBg, wepAmmoFill, BARFILLDIR::UP, wstr(heavyWeaponData[wepInfo->wepDataId]->name), nullptr, true, txt);
			hvyBar->setRounding(7);
		}
	}

}

HUDResources::~HUDResources()
{
	delete hp;
	delete shields;
	delete comms;
	delete energy;
	delete velocity;

	lBg->remove();
	//rBg->remove();
	centBg->remove();
	for (u32 i = 0; i < 4; ++i) {
		if (hpHit[i]) hpHit[i]->remove();
	}
	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		if (ammoBars[i]) delete ammoBars[i];
	}
	if(hvyAmmoBar)delete hvyAmmoBar;

}

void HUDResources::updateElement(flecs::entity playerId)
{
	auto hpcomp = playerId.get<HealthComponent>();
	auto player = playerId.get<PlayerComponent>();
	auto ship = playerId.get<ShipComponent>();
	auto hards = playerId.get<HardpointComponent>();
	auto rbc = playerId.get<BulletRigidBodyComponent>();
	auto thrst = playerId.get<ThrustComponent>();
	auto pow = playerId.get<PowerComponent>();

	for (u32 i = 0; i < 4; ++i) {
		auto clrhp = hpHit[i]->getColor();
		if (clrhp.getAlpha() > 0) clrhp.setAlpha((clrhp.getAlpha() >= 2) ? clrhp.getAlpha() - 2 : 0);
		hpHit[i]->setColor(clrhp);
	}

	root->bringToFront(lBg);
	//root->bringToFront(rBg);
	root->bringToFront(centBg);

	hp->updateBar(hpcomp->health, hpcomp->maxHealth);
	shields->updateBar(hpcomp->shields, hpcomp->maxShields);
	energy->updateBar(pow->power, pow->maxPower);
	f32 max = thrst->getLinMax();
	if (gameController->currentScenario->environment() == SECTOR_FINALE) max = 10000;
	velocity->updateBar(rbc->rigidBody->getLinearVelocity().length(), max);

	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		if (!ammoBars[i]) continue;
		auto wepInfo = hards->weapons[i].get<WeaponInfoComponent>();
		auto wepFire = hards->weapons[i].get<WeaponFiringComponent>();
		if (!wepInfo) continue;
		if (!wepInfo->usesAmmunition) continue;
		ammoBars[i]->updateBar(wepFire->clip, wepInfo->maxClip);
		std::wstring clipstr = L" (" + std::to_wstring(wepFire->ammunition) + L")";
		ammoBars[i]->name->setText((ammoBars[i]->name->getText() + clipstr).c_str());
	}
	if (hvyBar) {
		auto wepInfo = hards->heavyWeapon.get<WeaponInfoComponent>();
		auto wepFire = hards->heavyWeapon.get<WeaponFiringComponent>();

		if (!wepInfo) return;
		if (!wepInfo->usesAmmunition) return;
		hvyBar->updateBar(wepFire->clip, wepInfo->maxClip);
		std::wstring clipstr = L" (" + std::to_wstring(wepFire->ammunition) + L")";
		hvyBar->name->setText((hvyBar->name->getText() + clipstr).c_str());

	}

	comms->updateElement(playerId);
}
