#include "MotionTrailSceneNode.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include <iostream>
#define useExactCameraDir

namespace irr
{
	namespace scene
	{
		CMotionTrailSceneNode::CMotionTrailSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
			const core::vector3df& position, u32 segmentCount,
			bool timeBased, bool globalSpace, bool useCenterVertex,
			E_MOTION_TRAIL_FACING facing) : IMotionTrailSceneNode(parent, mgr, id, position, segmentCount, timeBased, globalSpace, useCenterVertex, facing) {
#ifdef _DEBUG
			setDebugName("CMotionTrailSceneNode");
#endif
			if (segmentCount < 2) segmentCount = 2;
			if (segmentCount > 10000) segmentCount = 10000;
			Box.reset(0, 0, 0);

			Material = video::SMaterial();
			Material.Lighting = false;
			Material.BackfaceCulling = false;
			this->MaximumSegmentCount = segmentCount;

			segmentCount += 2;
			this->RealSegmentCount = 0;
			this->TimeBased = timeBased;
			this->GlobalSpace = globalSpace;
			this->UseCenterVertex = useCenterVertex;
			this->FacingDirection = FacingDirection;

			TipIndex = 0;
			//EndIndex = 0;

			initMesh();
			SegmentPositions = new core::vector3df[segmentCount];
			SegmentDirections = 0;
			if (FacingDirection == EMTF_PER_SEGMENT) SegmentDirections = new core::vector3df[segmentCount];
			SegmentTimes = 0;
			if (timeBased) SegmentTimes = new u32[segmentCount];
			LockedAxis = core::vector3df(0);
			SegmentLength = (f32)10 / (f32)MaximumSegmentCount;
			Lifetime = 10000;

			InterpolationFunc = 0; //linear

			TipCenter = video::SColor(255, 255, 255, 255);
			TipEdge = video::SColor(255, 255, 255, 255);
			EndCenter = video::SColor(0, 255, 255, 255);
			EndEdge = video::SColor(0, 255, 255, 255);

			Width = 1.f;

			ShrinkTip = false;
			ShrinkEnd = false;
			ClipEnd = true;

			MeshDirty = true;
			BoxDirty = true;
		}

		CMotionTrailSceneNode::~CMotionTrailSceneNode()
		{
			if (Vertices) delete[] Vertices;
			if (Indices) delete[] Indices;
			if (SegmentPositions) delete[] SegmentPositions;
			if (SegmentDirections) delete[] SegmentDirections;
			if (SegmentTimes) delete[] SegmentTimes;
		}

		void CMotionTrailSceneNode::initMesh()
		{
			u32 verticesPerSegment = getVerticesPerSegment();
			u32 indicesPerSegment = getIndicesPerSegment();

			u32 segmentCount = MaximumSegmentCount + 2;

			Vertices = new video::S3DVertex[segmentCount * verticesPerSegment];	//3 verts per segment, 2 without center vertex
			Indices = new u16[(segmentCount - 1) * indicesPerSegment];	//between 2 segments: 2/4 faces => 6/12 tris	


			for (u32 i = 0; i < segmentCount - 1; i++) {
				u32 startIndex = indicesPerSegment * i;	//index index
				u16 startVertex = verticesPerSegment * i;	//vertex index
				if (!UseCenterVertex) {
					Indices[startIndex + 0] = startVertex + 0;	//	0---2--
					Indices[startIndex + 1] = startVertex + 1;	//  | / | /...
					Indices[startIndex + 2] = startVertex + 2;	//  1/--3/-

					Indices[startIndex + 3] = startVertex + 2;
					Indices[startIndex + 4] = startVertex + 1;
					Indices[startIndex + 5] = startVertex + 3;
				}
				else {
					Indices[startIndex + 0] = startVertex + 0;	//	0---3--
					Indices[startIndex + 1] = startVertex + 1;	//  | / | /...
					Indices[startIndex + 2] = startVertex + 3;	//  1/--4/-
																//  | / | /...
					Indices[startIndex + 3] = startVertex + 3;	//  2/--5/-
					Indices[startIndex + 4] = startVertex + 1;
					Indices[startIndex + 5] = startVertex + 4;

					Indices[startIndex + 6] = startVertex + 1;
					Indices[startIndex + 7] = startVertex + 2;
					Indices[startIndex + 8] = startVertex + 4;

					Indices[startIndex + 9] = startVertex + 4;
					Indices[startIndex + 10] = startVertex + 2;
					Indices[startIndex + 11] = startVertex + 5;
				}
			}
		}
		void CMotionTrailSceneNode::regenerateMesh(const ICameraSceneNode* camera)
		{
			u32 verticesPerSegment = getVerticesPerSegment();
			core::vector3df pos = getAbsolutePosition();	//trail object position
			core::vector3df camPos = camera->getAbsolutePosition();	//camera position
			core::vector3df camTarget = camera->getTarget();

			if (!GlobalSpace) {
				core::matrix4 invWorld;	//inverse world for non global
				AbsoluteTransformation.getInverse(invWorld);
				invWorld.transformVect(camPos);	//transform the view vector into local space
				invWorld.transformVect(camTarget);
			}

			core::vector3df view = camTarget - camPos;	//view direction

			f32 lengthSum = 0;	//the sum of all segment lengths 
			f32 maximumLength = SegmentLength * MaximumSegmentCount;

			u32 absoluteSegmentIndex = 0;	//0 based index
			u32 previousSegmentIndex = 0;	//index in actual array
			u32 currentSegmentIndex = TipIndex;
			u32 nextSegmentIndex;

			core::vector3df* previousSegmentPos = 0;
			core::vector3df* currentSegmentPos = &SegmentPositions[currentSegmentIndex];
			core::vector3df* nextSegmentPos;

			core::vector3df lastOutward = core::vector3df(0);
			core::vector3df fromTip = core::vector3df(0);	//points from the previous segment towards the current segment
			core::vector3df	toEnd = core::vector3df(0);	//points from this segment to the next segment (towards the end)

			f32 toEndLength = 0; //(previous) >-fromTip-> (current) >-toEnd-> (n)

			do	//count through every segment. absoluteSegmentIndex is the current iteration.
			{
				nextSegmentIndex = getSegmentIndex(absoluteSegmentIndex + 1);
				if (absoluteSegmentIndex < RealSegmentCount - 1) nextSegmentPos = &SegmentPositions[nextSegmentIndex];
				else nextSegmentPos = 0;


				core::vector3df forwardVector = core::vector3df();	//forward is towards tip. tip has interpolation value 0

				fromTip = toEnd;
				if (nextSegmentPos != 0) toEnd = *currentSegmentPos - *nextSegmentPos;
				else toEnd = core::vector3df(0);

				forwardVector = fromTip + toEnd;
				forwardVector.normalize();

#ifdef useExactCameraDir
				view = camPos - *currentSegmentPos;	//calculate view dir for every seg
#endif

				core::vector3df normal;	//normal is the facing direction of the current vector and will generally point towards the camera, unless it is locked
				core::vector3df outward; //points away from the segment center, perpendicular to normal

				if (FacingDirection == EMTF_CAMERA)	{ //the normal will point towards the camera
					normal = view;
					outward = forwardVector.crossProduct(normal);
					outward.normalize();
					if (outward.dotProduct(lastOutward) < 0) outward.invert(); //avoid twists
				}
				else if (FacingDirection == EMTF_AXIS) { //the normal points to the camera, but will not use the axis described by 'lockedAxis'
					normal = LockedAxis.crossProduct(forwardVector);
					outward = LockedAxis;
					outward.normalize();
				}
				else if (FacingDirection == EMTF_PER_SEGMENT) { //use a custom normal
					normal = SegmentDirections[currentSegmentIndex];
					outward = forwardVector.crossProduct(normal);
					outward.normalize();
				}
					//Adjust outward length, increase width of sharp angles. actually, this should be projected in camera space, but it isn't as the difference is too small
					//Could be removed completely, but sometimes looks better with low segment count
				f32 outwardMultiplier;

				f32 fromTipLength = toEndLength;
				toEndLength = toEnd.getLength();

				if (fromTipLength == 0 || toEndLength == 0) outwardMultiplier = 1;
				else {
					f32 dotProduct = toEnd.dotProduct(fromTip) / (fromTipLength * toEndLength);
					outwardMultiplier = core::clamp(core::squareroot(1 / ((dotProduct + 1) / 2)), (f32)0.001, (f32)3);
				}
				outwardMultiplier *= Width * (f32)0.5;
				outward *= outwardMultiplier;

				//Interpolation
				f32 linearInterpolation;	//tip has 0.0, end has 1.0, everything else is in between
				if (TimeBased) linearInterpolation = (f32)(CurrentTime - SegmentTimes[currentSegmentIndex]) / (f32)Lifetime;
				else linearInterpolation = lengthSum / maximumLength;

				f32 shrinkInterpolation = 1;	//this takes a value 
				if (ShrinkTip && ShrinkEnd) shrinkInterpolation = CMotionTrailSceneNode::shrinkInterpolation(linearInterpolation);
				else if (ShrinkTip) shrinkInterpolation = linearInterpolation;
				else if (ShrinkEnd) shrinkInterpolation = 1 - linearInterpolation;
				f32 interpolation;
				if (InterpolationFunc != 0)	{ //apply interpolationFunc to interpolation values
					shrinkInterpolation = InterpolationFunc(shrinkInterpolation);
					interpolation = InterpolationFunc(linearInterpolation);
				}
				else interpolation = linearInterpolation;

				core::vector3df segmentCenter = *currentSegmentPos;

				//Last Segment
				if (absoluteSegmentIndex == RealSegmentCount - 1) {	//is this the last segment?
					//make values exact to prevent rounding-error problems
					if (ShrinkEnd) shrinkInterpolation = 0;
					else shrinkInterpolation = 1;

					linearInterpolation = 1;
					interpolation = 1;

					if (ClipEnd) {	//clip last segment to maximum length/time
						f32 segmentFade;
						if (TimeBased) segmentFade = (f32)(CurrentTime - Lifetime - SegmentTimes[currentSegmentIndex]) / (f32)(SegmentTimes[previousSegmentIndex] - SegmentTimes[currentSegmentIndex]);
						else segmentFade = (lengthSum - SegmentLength * MaximumSegmentCount) / (fromTip.getLength());

						segmentFade = core::clamp(segmentFade, (f32)0, (f32)1);	//clamp is necessary: you get high negative values in the begin when using clampEnd + shrinkTip
						segmentCenter = previousSegmentPos->getInterpolated(*currentSegmentPos, segmentFade);	//interpolate between last and pre-last segment
						outward = lastOutward.getInterpolated(outward, segmentFade);	//don't care it's already multiplied...
					}
				}

				shrinkInterpolation = core::clamp(shrinkInterpolation, (f32)0, (f32)1);	//when the game was paused, this is sometimes necessary

				if (ShrinkTip || ShrinkEnd) outward *= shrinkInterpolation;

				u32 startVertex = absoluteSegmentIndex * verticesPerSegment;
				video::SColor centerColor = TipCenter.getInterpolated(EndCenter, 1 - interpolation);
				if (UseCenterVertex) {
					video::SColor edgeColor = TipEdge.getInterpolated(EndEdge, 1 - interpolation);
					Vertices[startVertex + 0] = video::S3DVertex(
						segmentCenter + outward, normal, edgeColor, core::vector2df(linearInterpolation, 1));
					Vertices[startVertex + 1] = video::S3DVertex(
						segmentCenter, normal, centerColor, core::vector2df(linearInterpolation, 0.5));
					Vertices[startVertex + 2] = video::S3DVertex(
						segmentCenter - outward, normal, edgeColor, core::vector2df(linearInterpolation, 0));
				}
				else {
					Vertices[startVertex + 0] = video::S3DVertex(
						segmentCenter + outward, normal, centerColor, core::vector2df(linearInterpolation, 1));
					Vertices[startVertex + 1] = video::S3DVertex(
						segmentCenter - outward, normal, centerColor, core::vector2df(linearInterpolation, 0));
				}

				//loop increment
				lengthSum += toEndLength;
				lastOutward = outward;

				previousSegmentIndex = currentSegmentIndex;
				currentSegmentIndex = nextSegmentIndex;
				previousSegmentPos = currentSegmentPos;
				currentSegmentPos = nextSegmentPos;

				absoluteSegmentIndex++;
			} while (absoluteSegmentIndex < RealSegmentCount);	//end of mesh generation loop
			MeshDirty = false;
		}
		void CMotionTrailSceneNode::addSegmentLengthBased(const core::vector3df& position) 
		{
			MeshDirty = true;
			BoxDirty = true;
			{
				bool replaceTip = false;
				if (RealSegmentCount > 3) {
					f32 tipDistance =
						SegmentPositions[TipIndex].getDistanceFrom(SegmentPositions[getSegmentIndex(1)]);
					if (tipDistance < SegmentLength) replaceTip = true;
					else replaceTip = false;
				}
				if (RealSegmentCount < (MaximumSegmentCount + 2) && !replaceTip) {
					TipIndex = getSegmentIndex(-1);
					RealSegmentCount++;
				}
				SegmentPositions[TipIndex] = position;
			}

			//ditch unnecessary segments
			{
				f32 lengthSum = 0;
				f32 maxLengthSum = SegmentLength * (f32)MaximumSegmentCount;
				core::vector3df* previousSegmentPosition = &SegmentPositions[TipIndex];
					
				for (u32 i = 1; i < RealSegmentCount; i++) {
					u32 segmentIndex = getSegmentIndex(i);
					core::vector3df * currentSegmentPosition = &SegmentPositions[segmentIndex];
					lengthSum += currentSegmentPosition->getDistanceFrom(*previousSegmentPosition);
					previousSegmentPosition = currentSegmentPosition;
					if (lengthSum > maxLengthSum) {
						if (RealSegmentCount - 1 > i) RealSegmentCount = i + 1;
						break;
					}
				}
			}

		}
		void CMotionTrailSceneNode::addSegmentTimeBased(const core::vector3df& position, u32 currentTime)
		{
			MeshDirty = true;
			BoxDirty = true;

			bool replaceTip = false;
			if (RealSegmentCount > 2) {
				u32 lifetimePerSegment = Lifetime / MaximumSegmentCount;
				if ((currentTime - SegmentTimes[getSegmentIndex(1)]) < lifetimePerSegment)
					replaceTip = true;
			}

			if (RealSegmentCount != (MaximumSegmentCount + 2) && !replaceTip) {
				TipIndex = getSegmentIndex(-1);
				RealSegmentCount++;
			}

			SegmentPositions[TipIndex] = position;
			SegmentTimes[TipIndex] = currentTime;
		}


		void CMotionTrailSceneNode::setSegment(u32 index, const core::vector3df& position)
		{
			if (TimeBased || FacingDirection == EMTF_PER_SEGMENT) {
				printf("CMotionTrailSceneNode::Wrong setSegment used, ignoring\n");
				return;
			}
			if (index > MaximumSegmentCount) {
				printf("CMotionTrailSceneNode::setSegment exceeds maximum count, ignoring\n");
				return;
			}
			MeshDirty = true;
			BoxDirty = true;
			if (index >= RealSegmentCount) RealSegmentCount = index + 1;
			SegmentPositions[getSegmentIndex(index)] = position;
		}
		void CMotionTrailSceneNode::setSegment(u32 index, const core::vector3df& position, u32 currentTime)
		{
			if (!TimeBased || FacingDirection == EMTF_PER_SEGMENT) {
				printf("CMotionTrailSceneNode::Wrong setSegment used, ignoring\n");
				return;
			}
			if (index > MaximumSegmentCount) {
				printf("CMotionTrailSceneNode::setSegment exceeds maximum count, ignoring\n");
				return;
			}
			MeshDirty = true;
			BoxDirty = true;
			if (index >= RealSegmentCount) RealSegmentCount = index + 1;
			SegmentPositions[getSegmentIndex(index)] = position;
			SegmentTimes[getSegmentIndex(index)] = currentTime;
		}
		void CMotionTrailSceneNode::setSegment(u32 index, const core::vector3df& position, const core::vector3df& customDirection)
		{
			if (TimeBased || FacingDirection != EMTF_PER_SEGMENT) {
				printf("CMotionTrailSceneNode::Wrong setSegment used, ignoring\n");
				return;
			}
			if (index > MaximumSegmentCount) {
				printf("CMotionTrailSceneNode::setSegment exceeds maximum count, ignoring\n");
				return;
			}
			MeshDirty = true;
			BoxDirty = true;
			if (index >= RealSegmentCount) RealSegmentCount = index + 1;
			SegmentPositions[getSegmentIndex(index)] = position;
			SegmentDirections[getSegmentIndex(index)] = position;
		}
		void CMotionTrailSceneNode::setSegment(u32 index, const core::vector3df& position, const core::vector3df& customDirection, u32 currentTime)
		{
			if (!TimeBased || FacingDirection != EMTF_PER_SEGMENT) {
				printf("CMotionTrailSceneNode::Wrong setSegment used, ignoring\n");
				return;
			}
			if (index > MaximumSegmentCount) {
				printf("CMotionTrailSceneNode::setSegment exceeds maximum count, ignoring\n");
				return;
			}
			MeshDirty = true;
			BoxDirty = true;
			if (index >= RealSegmentCount) RealSegmentCount = index + 1;
			SegmentPositions[getSegmentIndex(index)] = position;
			SegmentDirections[getSegmentIndex(index)] = position;
			SegmentTimes[getSegmentIndex(index)] = currentTime;
		}
		void CMotionTrailSceneNode::clearSegments()
		{
			MeshDirty = true;
			BoxDirty = true;

			RealSegmentCount = 0;
		}
		void CMotionTrailSceneNode::setLockedAxis(const core::vector3df& lockAxis)
		{
			MeshDirty = true;

			this->LockedAxis = lockAxis;
		}
		void CMotionTrailSceneNode::setLength(f32 length)
		{
			//Allow it, but like... cmon
			if (TimeBased) printf("CMotionTrailSceneNode::Please use setLifetime to set the lifetime of time-based trails.\n");

			MeshDirty = true;

			if (length <= 0) printf("CMotionTrailSceneNode::Length must be >0. Ignoring.\n");
			else SegmentLength = length / (f32)MaximumSegmentCount;
		}
		void CMotionTrailSceneNode::setLifetime(u32 lifetime)
		{
			//same as above, allow it if someone wants to be silly
			if (!TimeBased) printf("CMotionTrailSceneNode::Please use setLength to set the length of length-based trails.\n");

			MeshDirty = true;

			if (lifetime < 1) printf("CMotionTrailSceneNode::Lifetime too low. Ignoring.\n");
			else this->Lifetime = lifetime;
		}
		void CMotionTrailSceneNode::setWidth(f32 width)
		{
			MeshDirty = true;

			this->Width = width;
		}
		void CMotionTrailSceneNode::setShrinkDirection(bool shrinkTip, bool shrinkEnd, bool clipEnd)
		{
			MeshDirty = true;

			this->ShrinkTip = shrinkTip;
			this->ShrinkEnd = shrinkEnd;
			this->ClipEnd = clipEnd;
		}
		void CMotionTrailSceneNode::setInterpolationMode(E_MOTION_TRAIL_INTERPOLATION interpolationType)
		{
			MeshDirty = true;

			switch (interpolationType)
			{
			case EMTI_LINEAR:
				InterpolationFunc = 0;
				break;
			case EMTI_ROOT:
				InterpolationFunc = rootInterpolation;
				break;
			case EMTI_QUADRATIC:
				InterpolationFunc = quadraticInterpolation;
				break;
			case EMTI_SMOOTH:
				InterpolationFunc = smoothInterpolation;
				break;
			case EMTI_CUSTOM:
				if (InterpolationFunc == 0) printf("CMotionTrailSceneNode::No custom interpolation function set. Falling back to linear!\n");
				break;
			default:
				InterpolationFunc = 0;
				break;
			}
		}
		void CMotionTrailSceneNode::setInterpolationMode(f32(*interpolationFunc)(f32))
		{
			MeshDirty = true;

			this->InterpolationFunc = interpolationFunc;
		}
		void CMotionTrailSceneNode::setVertexColors(video::SColor tip, video::SColor end)
		{
			MeshDirty = true;

			TipCenter = tip;
			TipEdge = tip;
			EndCenter = end;
			EndEdge = end;
		}
		void CMotionTrailSceneNode::setVertexColors(video::SColor tipCenter, video::SColor tipEdge,
			video::SColor endCenter, video::SColor endEdge)
		{
			if (!UseCenterVertex) printf("CMotionTrailSceneNode::No center vertex used. Edge values will be ignored.\n");

			MeshDirty = true;

			this->TipCenter = tipCenter;
			this->TipEdge = tipEdge;
			this->EndCenter = endCenter;
			this->EndEdge = endEdge;
		}
		bool CMotionTrailSceneNode::isTimeBased() const
		{
			return TimeBased;
		}
		void CMotionTrailSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible) SceneManager->registerNodeForRendering(this);
			ISceneNode::OnRegisterSceneNode();
		}
		void CMotionTrailSceneNode::OnAnimate(u32 currentTime)
		{
			if (TimeBased) {
				this->CurrentTime = currentTime;

				if (RealSegmentCount >= 2) {//we can't remove segments if we haven't got enough...
					bool gotOne = false; //we only remove, if two in a row are too old.
					//remove all expired segments, starting from the end
					for (u32 absoluteSegmentIndex = RealSegmentCount - 1; absoluteSegmentIndex > 0; absoluteSegmentIndex--) {
						u32 segmentIndex = getSegmentIndex(absoluteSegmentIndex);
						if (SegmentTimes[segmentIndex] + Lifetime < currentTime) {
							if (gotOne) {
								RealSegmentCount = absoluteSegmentIndex + 1;
								BoxDirty = true;
								MeshDirty = true;
								break;
							}
							else gotOne = true;
						}
					}
				}
			}

			ISceneNode::OnAnimate(currentTime);

			//calculate bbox
			if (RealSegmentCount < 2) {//empty if too few segments
				Box.reset(0, 0, 0);
			}
			else {
				if (BoxDirty) {
					Box.reset(SegmentPositions[TipIndex]);
					for (u32 i = 1; i < RealSegmentCount; i++) {
						Box.addInternalPoint(SegmentPositions[getSegmentIndex(i)]);
					}
					if (GlobalSpace) {
						core::matrix4 invWorld;	//inverse world for global
						AbsoluteTransformation.getInverse(invWorld);
						invWorld.transformBoxEx(Box);
					}

					Box.MaxEdge += core::vector3df(Width / 2);	//grow by width
					Box.MinEdge -= core::vector3df(Width / 2);

					BoxDirty = false;
				}
			}
		}
		void CMotionTrailSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			ICameraSceneNode* camera = SceneManager->getActiveCamera();

			if (!camera || !driver) return;
			if (RealSegmentCount < 2) return;

			if (FacingDirection == EMTF_CAMERA || MeshDirty) regenerateMesh(camera);//regenerate if facing is camera-dependent or modified

			u32 vertexCount = getMeshVertexCount();
			u32 triangleCount = getMeshTriangleCount();

			//actual render
			driver->setMaterial(Material);
			if (GlobalSpace) driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
			else driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
			driver->drawIndexedTriangleList(Vertices, vertexCount, Indices, triangleCount);

			if (DebugDataVisible) {
				video::SMaterial m;
				m.Lighting = false;
				m.AntiAliasing = 0;
				driver->setMaterial(m);

				if (DebugDataVisible & scene::EDS_BBOX) {
					driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
					driver->draw3DBox(Box, video::SColor(255, 255, 255, 255));
				}

				if (DebugDataVisible & scene::EDS_MESH_WIRE_OVERLAY) {
					if (GlobalSpace) driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
					else driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);

					m.Wireframe = true;
					driver->setMaterial(m);

					driver->drawIndexedTriangleList(Vertices, vertexCount, Indices, triangleCount);
				}
			}
		}
		const core::aabbox3d<f32>& CMotionTrailSceneNode::getBoundingBox() const
		{
			return Box;
		}
		video::SMaterial& CMotionTrailSceneNode::getMaterial(u32 i)
		{
			return Material;
		}
		u32 CMotionTrailSceneNode::getMaterialCount() const
		{
			return 1;
		}
		void CMotionTrailSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
			ISceneNode::serializeAttributes(out, options);

			out->addBool("TimeBased", TimeBased);
			out->addBool("UseCenterVertex", UseCenterVertex);
			out->addBool("GlobalSpace", GlobalSpace);

			out->addInt("MaximumSegmentCount", MaximumSegmentCount);
			out->addInt("RealSegmentCount", RealSegmentCount);
			out->addInt("TipIndex", TipIndex);

			core::stringc name;
			for (u32 i = 0; i < MaximumSegmentCount + 2; i++)
			{
				name = "SegmentPositions";
				name += i;
				out->addVector3d(name.c_str(), SegmentPositions[i]);
			}
			if (SegmentDirections)
				for (u32 i = 0; i < MaximumSegmentCount + 2; i++)
				{
					name = "SegmentDirections";
					name += i;
					out->addVector3d(name.c_str(), SegmentDirections[i]);
				}
			if (SegmentTimes)
				for (u32 i = 0; i < MaximumSegmentCount + 2; i++)
				{
					name = "SegmentTimes";
					name += i;
					out->addInt(name.c_str(), SegmentTimes[i]);
				}

			out->addVector3d("LockedAxis", LockedAxis);

			if (TimeBased)
				out->addInt("Lifetime", Lifetime);
			else
				out->addFloat("SegmentLength", SegmentLength);


			E_MOTION_TRAIL_INTERPOLATION interpolationMode;	//convert the interpolationFunc to the enum value
			if (InterpolationFunc == 0)
				interpolationMode = EMTI_LINEAR;
			else if (InterpolationFunc == rootInterpolation)
				interpolationMode = EMTI_ROOT;
			else if (InterpolationFunc == quadraticInterpolation)
				interpolationMode = EMTI_QUADRATIC;
			else if (InterpolationFunc == smoothInterpolation)
				interpolationMode = EMTI_SMOOTH;
			else
				interpolationMode = EMTI_CUSTOM;

			out->addEnum("FacingDirection", FacingDirection, MotionTrailFacingNames);
			out->addEnum("InterpolationMode", interpolationMode, MotionTrailInterpolationNames);

			out->addColor("TipCenter", TipCenter);
			out->addColor("EndCenter", EndCenter);
			if (UseCenterVertex)
			{
				out->addColor("TipEdge", TipEdge);
				out->addColor("EndEdge", EndEdge);
			}

			out->addFloat("Width", Width);
			out->addBool("ShrinkTip", ShrinkTip);
			out->addBool("ShrinkEnd", ShrinkEnd);
			out->addBool("ClipEnd", ClipEnd);
		}
		void CMotionTrailSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
			ISceneNode::deserializeAttributes(in, options);

			//Constructor already ran... cleanup...
			if (Vertices)
				delete[] Vertices;
			if (Indices)
				delete[] Indices;
			if (SegmentPositions)
				delete[] SegmentPositions;
			if (SegmentDirections)
				delete[] SegmentDirections;
			if (SegmentTimes)
				delete[] SegmentTimes;
			Vertices = 0;
			Indices = 0;
			SegmentPositions = 0;
			SegmentDirections = 0;
			SegmentTimes = 0;

			TimeBased = in->getAttributeAsBool("TimeBased");
			UseCenterVertex = in->getAttributeAsBool("UseCenterVertex");
			GlobalSpace = in->getAttributeAsBool("GlobalSpace");

			MaximumSegmentCount = in->getAttributeAsInt("MaximumSegmentCount");
			u32 segmentCount = MaximumSegmentCount + 2;
			RealSegmentCount = in->getAttributeAsInt("RealSegmentCount");
			TipIndex = in->getAttributeAsInt("TipIndex");

			FacingDirection = (E_MOTION_TRAIL_FACING)in->getAttributeAsEnumeration("FacingDirection", MotionTrailFacingNames);

			//init mesh
			initMesh();

			SegmentPositions = new core::vector3df[segmentCount];
			SegmentDirections = 0;
			if (FacingDirection == EMTF_PER_SEGMENT)
				SegmentDirections = new core::vector3df[segmentCount];

			SegmentTimes = 0;
			if (TimeBased)
				SegmentTimes = new u32[segmentCount];



			core::stringc name;
			for (u32 i = 0; i < MaximumSegmentCount + 2; i++)
			{
				name = "SegmentPositions";
				name += i;
				SegmentPositions[i] = in->getAttributeAsVector3d(name.c_str());
			}
			if (FacingDirection == EMTF_PER_SEGMENT)
				for (u32 i = 0; i < MaximumSegmentCount + 2; i++)
				{
					name = "SegmentDirections";
					name += i;
					SegmentDirections[i] = in->getAttributeAsVector3d(name.c_str());
				}
			if (TimeBased)
				for (u32 i = 0; i < MaximumSegmentCount + 2; i++)
				{
					name = "SegmentTimes";
					name += i;
					SegmentTimes[i] = in->getAttributeAsInt(name.c_str());
				}

			LockedAxis = in->getAttributeAsVector3d("LockedAxis");
			if (TimeBased)
				Lifetime = in->getAttributeAsInt("Lifetime");
			else
				SegmentLength = in->getAttributeAsFloat("SegmentLength");


			E_MOTION_TRAIL_INTERPOLATION interpolationMode = (E_MOTION_TRAIL_INTERPOLATION)in->getAttributeAsEnumeration("InterpolationMode", MotionTrailInterpolationNames);
			setInterpolationMode(interpolationMode);

			TipCenter = TipEdge = in->getAttributeAsColor("TipCenter");
			EndCenter = EndEdge = in->getAttributeAsColor("EndCenter");
			if (UseCenterVertex)
			{
				TipEdge = in->getAttributeAsColor("TipEdge");
				EndEdge = in->getAttributeAsColor("EndEdge");
			}

			Width = in->getAttributeAsFloat("Width");
			ShrinkTip = in->getAttributeAsBool("ShrinkTip");
			ShrinkEnd = in->getAttributeAsBool("ShrinkEnd");
			ClipEnd = in->getAttributeAsBool("ClipEnd");
		}
		ISceneNode* CMotionTrailSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			CMotionTrailSceneNode* node = new CMotionTrailSceneNode(
				newParent,
				newManager,
				ID,
				getPosition(),
				MaximumSegmentCount,
				TimeBased,
				GlobalSpace,
				UseCenterVertex,
				FacingDirection);

			node->cloneMembers(this, newManager);

			for (u32 index = 0; index < MaximumSegmentCount + 2; index++)
			{
				if (node->SegmentPositions)
					node->SegmentPositions[index] = this->SegmentPositions[index];
				if (node->SegmentDirections)
					node->SegmentDirections[index] = this->SegmentDirections[index];
				if (node->SegmentTimes)
					node->SegmentTimes[index] = this->SegmentTimes[index];
			}

			node->CurrentTime = this->CurrentTime;
			node->Box = this->Box;
			node->Material = this->Material;
			node->LockedAxis = this->LockedAxis;
			node->RealSegmentCount = this->RealSegmentCount;
			node->TipIndex = this->TipIndex;
			node->SegmentLength = this->SegmentLength;
			node->Lifetime = this->Lifetime;
			node->InterpolationFunc = this->InterpolationFunc;
			node->TipCenter = this->TipCenter;
			node->TipEdge = this->TipEdge;
			node->EndCenter = this->EndCenter;
			node->EndEdge = this->EndEdge;
			node->Width = this->Width;
			node->ShrinkTip = this->ShrinkTip;
			node->ShrinkEnd = this->ShrinkEnd;
			node->ClipEnd = this->ClipEnd;

			node->BoxDirty = true;
			node->MeshDirty = true;


			if (newParent)
				node->drop();
			return node;
		}

		//--------------------------------------------------------------------------------------------------//

		CSceneNodeAnimatorMotionTrail::CSceneNodeAnimatorMotionTrail(const ISceneNode* attach)
			: ISceneNodeAnimator()
		{
#ifdef _DEBUG
			setDebugName("CSceneNodeAnimatorMotionTrail");
#endif

			this->Attach = attach;
		}

		//! animates a scene node
		void CSceneNodeAnimatorMotionTrail::animateNode(ISceneNode* node, u32 timeMs)
		{
			if (node && Attach) {
				CMotionTrailSceneNode* trail = static_cast<CMotionTrailSceneNode*>(node);
				if (trail->isTimeBased()) trail->addSegment(Attach->getAbsolutePosition(), timeMs);
				else trail->addSegment(Attach->getAbsolutePosition());
			}
		}

		//! Writes attributes of the scene node animator.
		void CSceneNodeAnimatorMotionTrail::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
			ISceneNodeAnimator::serializeAttributes(out, options);
		}

		//! Reads attributes of the scene node animator.
		void CSceneNodeAnimatorMotionTrail::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
			ISceneNodeAnimator::deserializeAttributes(in, options);
		}


		ISceneNodeAnimator* CSceneNodeAnimatorMotionTrail::createClone(ISceneNode* node, ISceneManager* newManager)
		{
			CSceneNodeAnimatorMotionTrail* newAnimator = new CSceneNodeAnimatorMotionTrail(Attach);
			newAnimator->cloneMembers(this);
			return newAnimator;
		}

	} // end namespace scene
} //end namespace irr