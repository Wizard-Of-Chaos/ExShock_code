#pragma once
#ifndef GEOMETRYCREATORS_H
#define GEOMETRYCREATORS_H
#include <irrlicht.h>

namespace irr
{
	namespace scene
	{
		IMesh* createPointVolumeLightMesh(const u32 layers = 64, const u32 pointsPerSphere = 8,
			const video::SColor centerColor = video::SColor(255, 255, 255, 255), const video::SColor outerColor = video::SColor(0, 255, 255, 255));

		IMesh* createCloudVolumeMesh(video::IVideoDriver* driver, const IMesh* cloudMesh, const u32 layers = 64,
			const video::SColor centerColor = video::SColor(255, 255, 255, 255), const video::SColor outerColor = video::SColor(0, 255, 255, 255));
	}
}

#endif