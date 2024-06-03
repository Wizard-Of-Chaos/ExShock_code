#pragma once
#ifndef MOTION_TRAIL_SCENE_NODE_H
#define MOTION_TRAIL_SCENE_NODE_H
#include "ISceneNode.h"
#include "ICameraSceneNode.h"
#include "S3DVertex.h"
/*
note: the original version of these files was from a user named Foaly on the Irrlicht forums,
who had a perverse habit of submitting things as .patch files rather than .h and .cpp files. .patch files
were not working properly with irrlicht 1.9 given the other changes involved (or at least, I couldn't get them to work)
so these have been re-engineered

thanks very much to that user, but also, please consider Github or something in the future instead of a fucking dropbox file
*/
namespace irr
{
	namespace scene
	{
		enum E_MOTION_TRAIL_FACING
		{
			//face toward camera
			EMTF_CAMERA = 0,
			//face on axis
			EMTF_AXIS = 1,
			//face direction per specific segment
			EMTF_PER_SEGMENT=2
		};

		enum E_MOTION_TRAIL_INTERPOLATION
		{
			//linear interpolation
			EMTI_LINEAR = 0,
			//square-root approximation
			EMTI_ROOT = 1,
			//square for interpolation
			EMTI_QUADRATIC = 2,
			//smooth-step
			EMTI_SMOOTH = 3,
			//customized function with setInterpolationType()
			EMTI_CUSTOM = 4
		};
		const c8* const MotionTrailFacingNames[] =
		{
			"camera",
			"customAxis",
			"customPerSegment",
			0
		};
		const c8* const MotionTrailInterpolationNames[] =
		{
			"linear",
			"root",
			"quadratic",
			"smooth",
			"custom",
			0
		};

		/*
		* Scene node for creating motion trails, either length-based or time-based.
		* Effectively a bunch of connected billboards. MUST be used with a scene node animator to get any actual "motion".
		* This should not be used with lighting; it's doable but it looks odd.
		*/
		class IMotionTrailSceneNode : public ISceneNode
		{
		public:

			//! constructor
			/*
			* \param segmentCount Amount of segments in the trail.
			* \param timeBased Whether or not the motion trail is length-based or time-based.
			* \param globalSpace Whether or not the coordinates are used in global or local coordinate space.
			* \param useCenterVertex If the center vertex is going to be used, this is enabled (3 verts instead of 2).
			* \param facingDirection What direction the verts face.
			*/
			IMotionTrailSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
				const core::vector3df& position = core::vector3df(0, 0, 0), u32 segmentCount = 32,
				bool timeBased = false, bool globalSpace = true, bool useCenterVertex = false,
				E_MOTION_TRAIL_FACING facing = EMTF_CAMERA)
				: ISceneNode(parent, mgr, id, position) {}

			//! Adds a new segment to the motion trail.
			/*
			* This version is for length-based motion trails without custom direction.
			* Using the wrong one logs a warning.
			*/
			virtual void addSegment(const core::vector3df& position) = 0;

			//! Adds a new segment to the motion trail.
			/*
			* This version is for time-based motion trails without custom direction.
			* Using the wrong one logs a warning.
			*/
			virtual void addSegment(const core::vector3df& position, u32 currentTime) = 0;

			//! Adds a new segment to the motion trail.
			/*
			* This version is for length-based motion trails with custom direction.
			* Using the wrong one logs a warning.
			*/
			virtual void addSegment(const core::vector3df& position, const core::vector3df& customDirection) = 0;

			//! Adds a new segment to the motion trail.
			/*
			* This version is for time-based motion trails with custom direction.
			* Using the wrong one logs a warning.
			*/
			virtual void addSegment(const core::vector3df& position, const core::vector3df& customDirection, u32 currentTime) = 0;


			//! Sets a segment directly.
			/*
			* If the trail doesn't have that many segments, it will add a new segment.
			* This version is for length-based motion trails without custom direction.
			* Using the wrong one logs a warning.
			*/
			virtual void setSegment(u32 index, const core::vector3df& position) = 0;

			//! Sets a segment directly.
			/*
			* If the trail doesn't have that many segments, it will add a new segment.
			* This version is for length-based motion trails with custom direction.
			* Using the wrong one logs a warning.
			*/
			virtual void setSegment(u32 index, const core::vector3df& position, const core::vector3df& customDirection) = 0;

			//! Sets a segment directly.
			/*
			* If the trail doesn't have that many segments, it will add a new segment.
			* This version is for time-based motion trails without custom direction.
			* Using the wrong one logs a warning.
			*/
			virtual void setSegment(u32 index, const core::vector3df& position, u32 currentTime) = 0;

			//! Sets a segment directly.
			/*
			* If the trail doesn't have that many segments, it will add a new segment.
			* This version is for time-based motion trails with custom direction.
			* Using the wrong one logs a warning.
			*/
			virtual void setSegment(u32 index, const core::vector3df& position, const core::vector3df& customDirection, u32 currentTime) = 0;

			//! Removes all segments.
			/*
			* Useful for situations where the object in question might teleport.
			*/
			virtual void clearSegments() = 0;

			//! Sets the axis to lock the segments on.
			/*
			* MUST be used with EMTF_AXIS, otherwise this can't be used.
			*/
			virtual void setLockedAxis(const core::vector3df& lockAxis) = 0;

			//! Sets the maximum length of the motion trail.
			/*
			* Only usable with length-based trails.
			*/
			virtual void setLength(f32 length = 10.0) = 0;

			//! Sets the lifetime of a segment.
			/*
			* Only usable with time-based trails.
			*/
			virtual void setLifetime(u32 lifetime = 10000) = 0;

			//! Sets the width.
			virtual void setWidth(f32 width = 1.0) = 0;

			//! Sets whether and where the motion trail should shrink.
			/* 
			* \param clipEnd Clips segment to the maximum length (if the last segment is longer than the max length allowed).
			*/
			virtual void setShrinkDirection(bool shrinkTip = false, bool shrinkEnd = false, bool clipEnd = true) = 0;

			//! Set the interpolation mode for vertex colors and shrinking.
			virtual void setInterpolationMode(E_MOTION_TRAIL_INTERPOLATION interpolationType = EMTI_LINEAR) = 0;

			//! Set the interpolation mode for vertex colors and shrinking.
			/*
			* \param interpolationFunc Method that takes a value from tip to end (0 to 1) and returns a value from 0 to 1.
			*/
			virtual void setInterpolationMode(f32(*interpolationFunc)(f32)) = 0;

			//! Sets the vertex colors.
			virtual void setVertexColors(
				video::SColor tip = video::SColor(255, 255, 255, 255),
				video::SColor end = video::SColor(0, 255, 255, 255)) = 0;

			//! Sets the vertex colors.
			/*
			* This version used with the center vertex enabled.
			*/
			virtual void setVertexColors(
				video::SColor tipCenter = video::SColor(255, 255, 255, 255),
				video::SColor tipEdge = video::SColor(255, 255, 255, 255),
				video::SColor endCenter = video::SColor(0, 255, 255, 255),
				video::SColor endEdge = video::SColor(0, 255, 255, 255)) = 0;

			//! Returns whether the trail is time-based (if false, it's length-based).
			virtual bool isTimeBased() const = 0;
		};

		//implementation
		class CMotionTrailSceneNode : public IMotionTrailSceneNode
		{
		public:
			/*
			* \param timeBased True for time-based trails.
			* \param globalSpace True for using coordinates in global coordinate space.
			* \param useCenterVertex Enabled for making segments have 3 verts instead of 2; useful for vertex colors.
			*/
			CMotionTrailSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
				const core::vector3df& position = core::vector3df(0), u32 segmentCount = 32,
				bool timeBased = false, bool globalSpace = true, bool useCenterVertex = false,
				E_MOTION_TRAIL_FACING = EMTF_CAMERA);
			virtual ~CMotionTrailSceneNode();

			/*
			* Adds a new segment to the motion trail. Used for length-based motion trails without custom direction.
			*/
			virtual void addSegment(const core::vector3df& position)
			{
				if (TimeBased || FacingDirection == EMTF_PER_SEGMENT) {
					printf("CMotionTrailSceneNode::Wrong addSegment used, ignoring.\n");
					return;
				}
				addSegmentLengthBased(position);
			}
			/*
			* Adds a new segment to the motion trail. Used for time-based motion trails without custom direction.
			*/
			virtual void addSegment(const core::vector3df& position, u32 currentTime) {
				if (!TimeBased || FacingDirection == EMTF_PER_SEGMENT) {
					printf("CMotionTrailSceneNode::Wrong addSegment used, ignoring.\n");
					return;
				}
				addSegmentTimeBased(position, currentTime);
			}
			/*
			* Adds a new segment to the motion trail. Used for length-based motion trails with custom direction.
			*/
			virtual void addSegment(const core::vector3df& position, const core::vector3df& customDirection) {
				if (TimeBased || FacingDirection != EMTF_PER_SEGMENT) {
					printf("CMotionTrailSceneNode::Wrong addSegment used, ignoring.\n");
					return;
				}
				addSegmentLengthBased(position);
				SegmentDirections[TipIndex] = customDirection;
				SegmentDirections[TipIndex].normalize();
			}
			/*
			* Adds a new segment to the motion trail. Used for time-based motion trails with custom direction.
			*/
			virtual void addSegment(const core::vector3df& position, const core::vector3df& customDirection, u32 currentTime) {
				if (!TimeBased || FacingDirection != EMTF_PER_SEGMENT) {
					printf("CMotionTrailSceneNode::Wrong addSegment used, ignoring.\n");
					return;
				}
				addSegmentTimeBased(position, currentTime);
				SegmentDirections[TipIndex] = customDirection;
				SegmentDirections[TipIndex].normalize();
			}

			//Sets a segment directly. Used for length-based motion trails without custom direction.
			virtual void setSegment(u32 index, const core::vector3df& position);
			//Sets a segment directly. Used for length-based motion trails with custom direction.
			virtual void setSegment(u32 index, const core::vector3df& position, const core::vector3df& customDirection);
			//Sets a segment directly. Used for time-based motion trails without custom direction.
			virtual void setSegment(u32 index, const core::vector3df& position, u32 currentTime);
			//Sets a segment directly. Used for length-based motion trails with custom direction.
			virtual void setSegment(u32 index, const core::vector3df& position, const core::vector3df& customDirection, u32 currentTime);

			//Clears all segments.
			virtual void clearSegments();
			//Sets the axis to lock segments on. Must be called when using EMTF_AXIS, otherwise not usable.
			virtual void setLockedAxis(const core::vector3df& lockAxis);
			//Sets the maximum length of the motion trail. Only used with length-based nodes.
			virtual void setLength(f32 length = 10.f);
			//Sets the maximum lifetime of a segment. Only used with time-based nodes.
			virtual void setLifetime(u32 lifetime = 10000);
			//Sets the width.
			virtual void setWidth(f32 width = 1.f);
			//Sets whether and where the motion trail should shrink.
			/* \param clipEnd Whether or not to clip the last segment to the maximum length.*/
			virtual void setShrinkDirection(bool shrinkTip = false, bool shrinkEnd = false, bool clipEnd = true);
			//Sets the interpolation mode for vertex colors and shrinking.
			virtual void setInterpolationMode(E_MOTION_TRAIL_INTERPOLATION type = EMTI_LINEAR);
			//Sets the interpolation mode for vertex colors and shrinking.
			/* \param interpolationFunc Function that takes a value from 1 to 0 and returns a value from 0 to 1. */
			virtual void setInterpolationMode(f32(*interpolationFunc)(f32));
			//Sets vertex colors.
			virtual void setVertexColors(
				video::SColor tip = video::SColor(255, 255, 255, 255),
				video::SColor end = video::SColor(0, 255, 255, 255)
			);
			//Sets vertex colors. Only usable if the center vertex is enabled.
			virtual void setVertexColors(
				video::SColor tipCenter = video::SColor(255, 255, 255, 255),
				video::SColor endCenter = video::SColor(0, 255, 255, 255),
				video::SColor tipEdge = video::SColor(255, 255, 255, 255),
				video::SColor endEdge = video::SColor(0, 255, 255, 255)
			);
			//Returns whether or not the trail is time based.
			virtual bool isTimeBased() const;

			virtual void OnRegisterSceneNode() IRR_OVERRIDE;
			virtual void OnAnimate(u32 currentTime) IRR_OVERRIDE;
			virtual void render() IRR_OVERRIDE;
			virtual const core::aabbox3d<f32>& getBoundingBox() const IRR_OVERRIDE;
			virtual video::SMaterial& getMaterial(u32 i) IRR_OVERRIDE;
			virtual u32 getMaterialCount() const IRR_OVERRIDE;
			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options = 0) const IRR_OVERRIDE;
			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options = 0) IRR_OVERRIDE;
			//virtual ESCENE_NODE_TYPE getType() const IRR_OVERRIDE;
			virtual ISceneNode* clone(ISceneNode* newParent = 0, ISceneManager* newManager = 0) IRR_OVERRIDE;
			u32 segCount() const { return RealSegmentCount; }
		private:
			REALINLINE u32 getSegmentIndex(s32 absoluteIndex)
			{
				return (absoluteIndex + (s32)TipIndex + (s32)(MaximumSegmentCount + 2)) % (s32)(MaximumSegmentCount + 2);
			}
			void initMesh();
			void regenerateMesh(const ICameraSceneNode* camera); //regenerate with a different camera
			//new segment index here is tipIndex, too old ones are removed
			REALINLINE void addSegmentLengthBased(const core::vector3df& position);
			//new segment index here is tipIndex
			REALINLINE void addSegmentTimeBased(const core::vector3df& position, u32 currentTime);
			//defs here to force inlines
#define quickRootConst 1.618033988749895 // core::squareroot((5) + 1) / 2;
#define quickRootConstInv	0.618033988749894	// 1 / quickRootConst
			static f32 rootInterpolation(f32 value) { return -1 / (value + (f32)quickRootConstInv) + (f32)quickRootConst; } //return core::squareroot(value);
#undef quickRootConst
#undef quickRootConstInv
			static f32 quadraticInterpolation(f32 value) { return value * value; }
			static f32 smoothInterpolation(f32 value) { return value * value * (3 - 2 * value); }
			REALINLINE static f32 shrinkInterpolation(f32 value) {
				if (value < .5f) return value * 2;
				return (1 - value) * 2;
			}
			REALINLINE u32 getVerticesPerSegment() { return UseCenterVertex ? 3 : 2; }
			REALINLINE u32 getIndicesPerSegment() { return UseCenterVertex ? 12 : 6; }
			REALINLINE u32 getTrianglesPerSegment() { return UseCenterVertex ? 4 : 2; }
			REALINLINE u32 getMeshVertexCount() { return RealSegmentCount * getVerticesPerSegment(); }
			REALINLINE u32 getMeshTriangleCount() { return (RealSegmentCount - 1) * getTrianglesPerSegment(); }

			u32 CurrentTime;
			bool TimeBased;
			bool UseCenterVertex;
			bool GlobalSpace;	//whether the data is stored in local or global space; coordinates given by add/set need to be in the given space

			//mesh vals
			core::aabbox3d<f32>	Box;
			video::SMaterial	Material;
			video::S3DVertex*   Vertices;
			u16*                Indices;

			bool BoxDirty;
			bool MeshDirty;

			//segment vals
			core::vector3df*    SegmentPositions;
			core::vector3df*    SegmentDirections;
			u32*                SegmentTimes;
			core::vector3df		LockedAxis;

			u32 MaximumSegmentCount;
			u32 RealSegmentCount;
			u32 TipIndex;

			//looks
			f32 SegmentLength;
			u32 Lifetime;
			E_MOTION_TRAIL_FACING FacingDirection;
			f32(*InterpolationFunc)(f32);
			video::SColor TipCenter;
			video::SColor TipEdge;
			video::SColor EndCenter;
			video::SColor EndEdge;
			f32 Width;
			bool ShrinkTip;
			bool ShrinkEnd;
			bool ClipEnd;
		};

		//animators for the motion trail in question
		class CSceneNodeAnimatorMotionTrail : public ISceneNodeAnimator
		{
		public:

			//! constructor
			CSceneNodeAnimatorMotionTrail(const ISceneNode* attach);

			//! animates a scene node
			virtual void animateNode(ISceneNode* node, u32 timeMs) IRR_OVERRIDE;

			//! Writes attributes of the scene node animator.
			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options = 0) const IRR_OVERRIDE;

			//! Reads attributes of the scene node animator.
			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options = 0) IRR_OVERRIDE;

			//! Creates a clone of this animator.
			/** Please note that you will have to drop
			(IReferenceCounted::drop()) the returned pointer after calling this. */
			virtual ISceneNodeAnimator* createClone(ISceneNode* node, ISceneManager* newManager = 0) IRR_OVERRIDE;

		private:

			const ISceneNode* Attach;
		};

	} //namespace scene
} //namespace irr
#endif