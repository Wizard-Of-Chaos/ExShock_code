#pragma once
#ifndef SHADERS_H
#define SHADERS_H
#include "BaseHeader.h"

enum SHADER_TYPE
{
	SHADE_8LIGHT_NORM,
	SHADE_4LIGHT_NORM,
	SHADE_3LIGHT_NORM,
	SHADE_2LIGHT_NORM,
	SHADE_SOLID
};

class ShaderManager
{
	public:
		void compile();
		E_MATERIAL_TYPE getShaderMaterial(SHADER_TYPE which) { if (m_materials.find(which) != m_materials.end()) return (E_MATERIAL_TYPE)m_materials.at(which); return EMT_SOLID; }
	private:
		std::unordered_map<SHADER_TYPE, IShaderConstantSetCallBack*> m_callbacks;
		std::unordered_map<SHADER_TYPE, s32> m_materials;
};

class DefaultShaderCb : public IShaderConstantSetCallBack
{
	public:
		virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData);
	private:
		matrix4 world;
		matrix4 view;
		matrix4 proj;
		matrix4 invtranspose;
		vector3df pos;
		SLight light;

};

class ShieldShaderCb : public IShaderConstantSetCallBack
{
	public:
		ShieldShaderCb(flecs::entity ent, IMeshSceneNode* shieldNode) : self(ent), shieldNode(shieldNode) {}
		virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData);
	private:
		matrix4 world;
		matrix4 view;
		matrix4 proj;
		flecs::entity self;
		IMeshSceneNode* shieldNode;
};


class EightLightNormCb : public IShaderConstantSetCallBack
{
public:
	virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData);
	virtual void OnSetMaterial(const SMaterial& material) { m_material = &material; }
private:
	const video::SMaterial* m_material;

	matrix4 m_world;
	matrix4 m_view;
	matrix4 m_proj;
	matrix4 m_invtranspose;

	matrix4 lights[8];
};

class FourLightNormCb : public IShaderConstantSetCallBack
{
public:
	virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData);
	virtual void OnSetMaterial(const SMaterial& material) { m_material = &material; }
private:
	const video::SMaterial* m_material;

	matrix4 m_world;
	matrix4 m_view;
	matrix4 m_proj;
	matrix4 m_invtranspose;

	matrix4 lights[4];
};

class ThreeLightNormCb : public IShaderConstantSetCallBack
{
public:
	virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData);
	virtual void OnSetMaterial(const SMaterial& material) { m_material = &material; }
private:
	const video::SMaterial* m_material;

	matrix4 m_world;
	matrix4 m_view;
	matrix4 m_proj;
	matrix4 m_invtranspose;

	matrix4 lights[3];
};

class TwoLightNormCb : public IShaderConstantSetCallBack
{
public:
	virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData);
	virtual void OnSetMaterial(const SMaterial& material) { m_material = &material; }
private:
	const video::SMaterial* m_material;

	matrix4 m_world;
	matrix4 m_view;
	matrix4 m_proj;
	matrix4 m_invtranspose;

	matrix4 lights[2];
};

#endif 