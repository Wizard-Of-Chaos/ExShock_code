#include "GeometryCreators.h"

namespace irr
{
	namespace scene
	{
		IMesh* createPointVolumeLightMesh(
			const u32 layers, u32 pointsPerSphere,
			const video::SColor centerColor, const video::SColor outerColor)
		{
			if (pointsPerSphere < 4)
				pointsPerSphere = 4;

			while (layers * pointsPerSphere*2 > 32767) // prevent u16 overflow
			{
				pointsPerSphere /= 2;
			}
			f32 totalRadius = 5.f;
			f32 radSlice = totalRadius / layers;
			const u32 polyCountXPitch = pointsPerSphere + 1; // get to same vertex on next level

			SMesh* mesh = new SMesh();
			//mesh->setMaterialFlag(video::EMF_LIGHTING, false);
			for (u32 lyr = 0; lyr < layers; ++lyr) {
				SMeshBuffer* buffer = new SMeshBuffer();
				f32 radius = radSlice + (radSlice * lyr);
				buffer->Vertices.clear();
				buffer->Indices.clear();
				buffer->setHardwareMappingHint(EHM_STATIC);
				buffer->Indices.reallocate((pointsPerSphere * pointsPerSphere) * 6);

				const video::SColor clr = outerColor.getInterpolated(centerColor, ((f32)lyr / (f32)layers));
				if (clr.getAlpha() == 0) break;
				//const video::SColor clr(255, 255, 255, 255);

				u32 level = 0;

				for (u32 p1 = 0; p1 < pointsPerSphere - 1; ++p1)
				{
					//main quads, top to bottom
					for (u32 p2 = 0; p2 < pointsPerSphere - 1; ++p2)
					{
						const u32 curr = level + p2;
						buffer->Indices.push_back(curr + polyCountXPitch);
						buffer->Indices.push_back(curr);
						buffer->Indices.push_back(curr + 1);
						buffer->Indices.push_back(curr + polyCountXPitch);
						buffer->Indices.push_back(curr + 1);
						buffer->Indices.push_back(curr + 1 + polyCountXPitch);
					}

					// the connectors from front to end
					buffer->Indices.push_back(level + pointsPerSphere - 1 + polyCountXPitch);
					buffer->Indices.push_back(level + pointsPerSphere - 1);
					buffer->Indices.push_back(level + pointsPerSphere);

					buffer->Indices.push_back(level + pointsPerSphere - 1 + polyCountXPitch);
					buffer->Indices.push_back(level + pointsPerSphere);
					buffer->Indices.push_back(level + pointsPerSphere + polyCountXPitch);
					level += polyCountXPitch;
				}

				const u32 polyCountSq = polyCountXPitch * pointsPerSphere; // top point
				const u32 polyCountSq1 = polyCountSq + 1; // bottom point
				const u32 polyCountSqM1 = (pointsPerSphere - 1) * polyCountXPitch; // last row's first vertex

				for (u32 p2 = 0; p2 < pointsPerSphere - 1; ++p2)
				{
					// create triangles which are at the top of the sphere

					buffer->Indices.push_back(polyCountSq);
					buffer->Indices.push_back(p2 + 1);
					buffer->Indices.push_back(p2);

					// create triangles which are at the bottom of the sphere

					buffer->Indices.push_back(polyCountSqM1 + p2);
					buffer->Indices.push_back(polyCountSqM1 + p2 + 1);
					buffer->Indices.push_back(polyCountSq1);
				}

				// create final triangle which is at the top of the sphere

				buffer->Indices.push_back(polyCountSq);
				buffer->Indices.push_back(pointsPerSphere);
				buffer->Indices.push_back(pointsPerSphere - 1);

				// create final triangle which is at the bottom of the sphere

				buffer->Indices.push_back(polyCountSqM1 + pointsPerSphere - 1);
				buffer->Indices.push_back(polyCountSqM1);
				buffer->Indices.push_back(polyCountSq1);

				// calculate the angle which separates all points in a circle
				const f64 AngleX = 2 * core::PI / pointsPerSphere;
				const f64 AngleY = core::PI / pointsPerSphere;

				u32 i = 0;
				f64 axz;

				// we don't start at 0.

				f64 ay = 0;//AngleY / 2;

				buffer->Vertices.set_used((polyCountXPitch * pointsPerSphere) + 2);
				for (u32 y = 0; y < pointsPerSphere; ++y)
				{
					ay += AngleY;
					const f64 sinay = sin(ay);
					axz = 0;

					// calculate the necessary vertices without the doubled one
					for (u32 xz = 0; xz < pointsPerSphere; ++xz)
					{
						// calculate points position

						const core::vector3df pos(static_cast<f32>(radius * cos(axz) * sinay),
							static_cast<f32>(radius * cos(ay)),
							static_cast<f32>(radius * sin(axz) * sinay));
						// for spheres the normal is the position
						core::vector3df normal(pos);
						normal.normalize();

						// calculate texture coordinates via sphere mapping
						// tu is the same on each level, so only calculate once
						f32 tu = 0.5f;
						if (y == 0)
						{
							if (normal.Y != -1.0f && normal.Y != 1.0f)
								tu = static_cast<f32>(acos(core::clamp(normal.X / sinay, -1.0, 1.0)) * 0.5 * core::RECIPROCAL_PI64);
							if (normal.Z < 0.0f)
								tu = 1 - tu;
						}
						else
							tu = buffer->Vertices[i - polyCountXPitch].TCoords.X;
						buffer->Vertices[i] = video::S3DVertex(pos.X, pos.Y, pos.Z,
							normal.X, normal.Y, normal.Z,
							clr, tu,
							static_cast<f32>(ay * core::RECIPROCAL_PI64));
						++i;
						axz += AngleX;
					}
					// This is the doubled vertex on the initial position
					buffer->Vertices[i] = video::S3DVertex(buffer->Vertices[i - pointsPerSphere]);
					buffer->Vertices[i].TCoords.X = 1.0f;
					++i;
				}

				// the vertex at the top of the sphere
				buffer->Vertices[i] = video::S3DVertex(0.0f, radius, 0.0f, 0.0f, 1.0f, 0.0f, clr, 0.5f, 0.0f);

				// the vertex at the bottom of the sphere
				++i;
				buffer->Vertices[i] = video::S3DVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, clr, 0.5f, 1.0f);

				// recalculate bounding box

				buffer->BoundingBox.reset(buffer->Vertices[i].Pos);
				buffer->BoundingBox.addInternalPoint(buffer->Vertices[i - 1].Pos);
				buffer->BoundingBox.addInternalPoint(radius, 0.0f, 0.0f);
				buffer->BoundingBox.addInternalPoint(-radius, 0.0f, 0.0f);
				buffer->BoundingBox.addInternalPoint(0.0f, 0.0f, radius);
				buffer->BoundingBox.addInternalPoint(0.0f, 0.0f, -radius);
				mesh->addMeshBuffer(buffer);
				buffer->drop();
			}
			mesh->setHardwareMappingHint(EHM_STATIC);
			mesh->recalculateBoundingBox();
			return mesh;
		}
		IMesh* createCloudVolumeMesh(video::IVideoDriver* driver, const IMesh* cloudMesh, const u32 layers, const video::SColor centerColor, const video::SColor outerColor)
		{
			if (!driver) return nullptr;
			if (!cloudMesh) return nullptr;

			SMesh* mesh = new SMesh();
			const IMeshBuffer* cloudBuf = cloudMesh->getMeshBuffer(0);
			IMeshManipulator* manip = driver->getMeshManipulator();

			for (u32 lyr = 0; lyr < layers; ++lyr) {
				IMeshBuffer* newbuf = cloudBuf->createClone();
				f32 scale = 1 + (.05f * (f32)lyr);
				manip->scale(newbuf, core::vector3df(scale, scale, scale));
				const video::SColor clr = outerColor.getInterpolated(centerColor, ((f32)lyr / (f32)layers));
				manip->setVertexColors(newbuf, clr);
				mesh->addMeshBuffer(newbuf);
			}

			mesh->setHardwareMappingHint(EHM_STATIC);
			mesh->recalculateBoundingBox();
			return mesh;
		}
	} // end namespace scene
}// end namespace irr