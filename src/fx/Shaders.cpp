#include "Shaders.h"
#include "GameController.h"
#include "BaedsLightManager.h"
#include "GameStateController.h"
#include "HealthComponent.h"
#include "IrrlichtComponent.h"

void ShaderManager::compile()
{
	IGPUProgrammingServices* gpu = driver->getGPUProgrammingServices();
	if (!gpu) return;

	m_callbacks[SHADE_8LIGHT_NORM] = new EightLightNormCb();
	m_callbacks[SHADE_4LIGHT_NORM] = new FourLightNormCb();
	m_callbacks[SHADE_3LIGHT_NORM] = new ThreeLightNormCb();
	m_callbacks[SHADE_2LIGHT_NORM] = new TwoLightNormCb();

	m_materials[SHADE_8LIGHT_NORM] = gpu->addHighLevelShaderMaterialFromFiles(
		"assets/effects/shaders/8light_norm.hlsl", "vertexMain", EVST_VS_3_0,
		"assets/effects/shaders/8light_norm.hlsl", "pixelMain", EPST_PS_3_0,
		m_callbacks.at(SHADE_8LIGHT_NORM), EMT_SOLID);
	m_materials[SHADE_4LIGHT_NORM] = gpu->addHighLevelShaderMaterialFromFiles(
		"assets/effects/shaders/4light_norm.hlsl", "vertexMain", EVST_VS_3_0,
		"assets/effects/shaders/4light_norm.hlsl", "pixelMain", EPST_PS_3_0,
		m_callbacks.at(SHADE_4LIGHT_NORM), EMT_SOLID);
	m_materials[SHADE_3LIGHT_NORM] = gpu->addHighLevelShaderMaterialFromFiles(
		"assets/effects/shaders/3light_norm.hlsl", "vertexMain", EVST_VS_3_0,
		"assets/effects/shaders/3light_norm.hlsl", "pixelMain", EPST_PS_3_0,
		m_callbacks.at(SHADE_3LIGHT_NORM), EMT_SOLID);
	m_materials[SHADE_2LIGHT_NORM] = gpu->addHighLevelShaderMaterialFromFiles(
		"assets/effects/shaders/2light_norm.hlsl", "vertexMain", EVST_VS_3_0,
		"assets/effects/shaders/2light_norm.hlsl", "pixelMain", EPST_PS_3_0,
		m_callbacks.at(SHADE_2LIGHT_NORM), EMT_SOLID);
	
	m_materials[SHADE_SOLID] = EMT_SOLID;
}

void DefaultShaderCb::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	proj = driver->getTransform(ETS_PROJECTION);
	world = driver->getTransform(ETS_WORLD);
	view = driver->getTransform(ETS_VIEW);
	invtranspose = world;
	invtranspose.makeInverse();
	invtranspose = invtranspose.getTransposed();
	services->setVertexShaderConstant("worldInverseTranspose", invtranspose.pointer(), 16);
	services->setVertexShaderConstant("world", world.pointer(), 16);
	services->setVertexShaderConstant("view", view.pointer(), 16);
	services->setVertexShaderConstant("pos", proj.pointer(), 16);

	light = driver->getDynamicLight(0);
	light.DiffuseColor.a = 1.f / (light.Radius * light.Radius); //set attenuation
	pos = device->getSceneManager()->getActiveCamera()->getAbsolutePosition();

	services->setVertexShaderConstant("diffuseLightPosition", reinterpret_cast<const f32*>(&light.Position), 3);
	services->setVertexShaderConstant("diffuseColor", reinterpret_cast<const f32*>(&light.DiffuseColor), 4);
	services->setVertexShaderConstant("viewPosition", reinterpret_cast<const f32*>(&pos), 3);

}

void ShieldShaderCb::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	proj = driver->getTransform(ETS_PROJECTION);
	world = driver->getTransform(ETS_WORLD);
	view = driver->getTransform(ETS_VIEW);

	s32 id = services->getVertexShaderConstantID("WORLD");
	services->setVertexShaderConstant(id, world.pointer(), 16);
	id = services->getVertexShaderConstantID("VIEW");
	services->setVertexShaderConstant(id, world.pointer(), 16);
	id = services->getVertexShaderConstantID("PROJ");
	services->setVertexShaderConstant(id, world.pointer(), 16);
	
	auto hp = self.get<HealthComponent>();
	auto transform = shieldNode->getAbsoluteTransformation();
	transform.makeInverse();
	for (s32 i = 0; i < MAX_TRACKED_DAMAGE_INSTANCES; ++i) {
		std::string hitstr = "HIT_POS_" + std::to_string(i);
		std::string timestr = "HIT_TIME_" + std::to_string(i);
		auto pos = services->getVertexShaderConstantID(hitstr.c_str());
		auto time = services->getVertexShaderConstantID(timestr.c_str());
		vector3df position(0, 10, 0);
		u32 timeMs = 5000;
		f32 timeF = 0.f;

		if (hp) {
			if (i < hp->instances.size()) {
				position = hp->instances.at(i).hitPos;
				timeMs = hp->instances.at(i).time;
				u32 elapsedTime = device->getTimer()->getTime() - timeMs;
				timeF = (f32)(elapsedTime / 1000) / 2.5f;
				timeF = std::clamp(timeF, 0.f, 1.f);
				std::cout << elapsedTime << std::endl;
			}
		}
		
		transform.transformVect(position);
		services->setVertexShaderConstant(pos, reinterpret_cast<const f32*>(&position), 3);
		services->setVertexShaderConstant(time, &timeF, 1);

	}
}

static void _getGlobalMatrices(matrix4& world, matrix4& view, matrix4& proj, matrix4& invTranspose)
{
	auto driver = device->getVideoDriver();
	world = driver->getTransform(ETS_WORLD);
	view = driver->getTransform(ETS_VIEW);
	proj = driver->getTransform(ETS_PROJECTION);
	invTranspose = world;
	invTranspose.makeInverse();
	invTranspose = invTranspose.getTransposed();
}

static void _setGlobalShaderConstants(IMaterialRendererServices* services, matrix4& world, matrix4& view, matrix4& proj, matrix4& invTranspose)
{
	auto id = services->getVertexShaderConstantID("WORLD");
	services->setVertexShaderConstant(id, world.pointer(), 16);
	id = services->getVertexShaderConstantID("VIEW");
	services->setVertexShaderConstant(id, view.pointer(), 16);
	id = services->getVertexShaderConstantID("PROJ");
	services->setVertexShaderConstant(id, proj.pointer(), 16);
	id = services->getVertexShaderConstantID("INV_TRANSPOSE");
	services->setVertexShaderConstant(id, invTranspose.pointer(), 16);
	id = services->getPixelShaderConstantID("CAMERA_VIEW");
	auto cam = device->getSceneManager()->getActiveCamera();
	vector3df camPos = cam->getAbsolutePosition();
	vector3df camVec = cam->getAbsoluteTransformation().getRotationDegrees().rotationToDirection(vector3df(0, 0, 1));
	f32 dist = cam->getFarValue();
	services->setPixelShaderConstant(id, reinterpret_cast<f32*>(&camVec), 3);

	vector3df nodePos;
	nodePos = lmgr->currentNode()->getAbsolutePosition();
	id = services->getPixelShaderConstantID("PLAYER_POS");
	services->setPixelShaderConstant(id, reinterpret_cast<f32*>(&camPos), 3);
	id = services->getPixelShaderConstantID("NODE_POS");
	services->setPixelShaderConstant(id, reinterpret_cast<f32*>(&nodePos), 3);
}

static void _setFogConstantsFromMaterial(IMaterialRendererServices* services, const SMaterial* material)
{
	auto id = services->getPixelShaderConstantID("HAS_FOG");
	f32 isFog = material->FogEnable;
	services->setPixelShaderConstant(id, &isFog, 1);
	if (material->FogEnable) {
		SColor col;
		f32 min, max, density;
		bool range, pix;
		E_FOG_TYPE type;
		driver->getFog(col, type, min, max, density, pix, range);
		SColorf colf(col);
		id = services->getPixelShaderConstantID("FOG_COLOR");
		services->setPixelShaderConstant(id, reinterpret_cast<f32*>(&colf), 4);
		id = services->getPixelShaderConstantID("FOG_MIN");
		services->setPixelShaderConstant(id, &min, 4);
		id = services->getPixelShaderConstantID("FOG_MAX");
		services->setPixelShaderConstant(id, &max, 4);
	}
}
static char numc[2];
static char which[8] = "LIGHT_X";
static char dirc[11] = "LIGHTDIR_X";
static char posc[11] = "LIGHTPOS_X";

static void _setLightShaderConstantsFromMatrix(IMaterialRendererServices* services, const u32& num, matrix4& light, vector3df& dir, vector3df& pos)
{
	itoa(num, numc, 10);
	which[6] = numc[0];
	dirc[9] = numc[0];
	posc[9] = numc[0];

	s32 matId = services->getPixelShaderConstantID(which);
	services->setPixelShaderConstant(matId, light.pointer(), 16);
	matId = services->getPixelShaderConstantID(dirc);
	services->setPixelShaderConstant(matId, reinterpret_cast<f32*>(&dir), 3);
	matId = services->getPixelShaderConstantID(posc);
	services->setPixelShaderConstant(matId, reinterpret_cast<f32*>(&pos), 3);
}

static void _loadLightDataToMatrices(IMaterialRendererServices* services, const u32& count, matrix4* matArray)
{
	auto driver = device->getVideoDriver();
	vector3df nodepos(0, 0, 0);
	if (lmgr->currentNode()) nodepos = lmgr->currentNode()->getAbsolutePosition();
	s32 lightCount = driver->getDynamicLightCount();

	for (s32 i = 0; i < count; ++i) {
		f32 radius = 0, inner = 0, outer = 0;
		vector3df direction(0, 0, 0);
		vector3df position(0, 0, 0);
		SColorf diff, amb, spec;
		bool valid = false;
		if (i < lightCount) {
			valid = true;
			const SLight& light = driver->getDynamicLight(i);
			if (i == 0 && lmgr->global()) lmgr->global()->getLightData();

			if (light.Type == ELT_POINT) {
				direction = (light.Position - nodepos);
				direction = direction.normalize();
			}
			else {
				direction = light.Direction;
			}
			position = light.Position;

			diff = light.DiffuseColor;
			amb = light.AmbientColor;
			spec = light.SpecularColor;

			radius = light.Radius;
			inner = light.InnerCone;
			outer = light.OuterCone;
		}
		else {
			valid = false;
		}
		f32 cast = valid;

		matArray[i][0] = cast;
		matArray[i][1] = radius;
		matArray[i][2] = inner;
		matArray[i][3] = outer;

		matArray[i][4] = diff.r;
		matArray[i][5] = diff.g;
		matArray[i][6] = diff.b;
		matArray[i][7] = diff.a;

		matArray[i][8] = spec.r;
		matArray[i][9] = spec.g;
		matArray[i][10] = spec.b;
		matArray[i][11] = spec.a;

		matArray[i][12] = amb.r;
		matArray[i][13] = amb.g;
		matArray[i][14] = amb.b;
		matArray[i][15] = amb.a;

		_setLightShaderConstantsFromMatrix(services, i, matArray[i], direction, position);
	}
}

void EightLightNormCb::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	const u32 num = 8;
	_getGlobalMatrices(m_world, m_view, m_proj, m_invtranspose);
	_setGlobalShaderConstants(services, m_world, m_view, m_proj, m_invtranspose);
	_loadLightDataToMatrices(services, num, lights);
	_setFogConstantsFromMaterial(services, m_material);
}
void FourLightNormCb::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	const u32 num = 4;
	_getGlobalMatrices(m_world, m_view, m_proj, m_invtranspose);
	_setGlobalShaderConstants(services, m_world, m_view, m_proj, m_invtranspose);
	_loadLightDataToMatrices(services, num, lights);
	_setFogConstantsFromMaterial(services, m_material);
}
void ThreeLightNormCb::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	const u32 num = 3;
	_getGlobalMatrices(m_world, m_view, m_proj, m_invtranspose);
	_setGlobalShaderConstants(services, m_world, m_view, m_proj, m_invtranspose);
	_loadLightDataToMatrices(services, num, lights);
	_setFogConstantsFromMaterial(services, m_material);
}
void TwoLightNormCb::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	const u32 num = 2;
	_getGlobalMatrices(m_world, m_view, m_proj, m_invtranspose);
	_setGlobalShaderConstants(services, m_world, m_view, m_proj, m_invtranspose);
	_loadLightDataToMatrices(services, num, lights);
	_setFogConstantsFromMaterial(services, m_material);
}
