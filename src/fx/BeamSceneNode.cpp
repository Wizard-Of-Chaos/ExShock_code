#include "BeamSceneNode.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include <iostream>
namespace irr
{
    namespace scene
    {
        CBeamNode::CBeamNode(ISceneNode* parent, ISceneManager* mgr, s32 id, video::ITexture* szBeam, video::ITexture* szBeamFront) : ISceneNode(parent, mgr, id)
        {
            // Setup the beam material
            material.Wireframe = false;
            material.Lighting = false;
            material.ZWriteEnable = video::E_ZWRITE::EZW_OFF;

            material.MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;
            material.setTexture(0, szBeam);

            material2.Wireframe = false;
            material2.Lighting = false;
            material2.ZWriteEnable = video::E_ZWRITE::EZW_OFF;

            material2.MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;
            material2.setTexture(0, szBeamFront);
            material2.BackfaceCulling = false;

            m_thickness = 1.0f;

            AutomaticCullingState = EAC_FRUSTUM_SPHERE;
        }

        CBeamNode::~CBeamNode(void)
        {

        }

        void CBeamNode::OnRegisterSceneNode(void)
        {
            if (IsVisible)
            {
                SceneManager->registerNodeForRendering(this, irr::scene::ESNRP_TRANSPARENT);
                ISceneNode::OnRegisterSceneNode();
            }
        }

        void CBeamNode::OnAnimate(u32 timeMs)
        {
            ISceneNode::OnAnimate(timeMs);
        }

        void CBeamNode::setLine(core::vector3df start, core::vector3df end, f32 thickness)
        {
            setPosition(start);
            m_start = core::vector3df(0, 0, 0);
            m_end = end;
            m_thickness = thickness;
        }

        void CBeamNode::render(void)
        {
            video::IVideoDriver* driver = SceneManager->getVideoDriver();
            driver->setTransform(irr::video::ETS_WORLD, AbsoluteTransformation);

            core::vector3df direction = m_end - m_start;
            direction.normalize();
            core::vector3df savesatrt = m_start;
            AbsoluteTransformation.transformVect(savesatrt);
            core::vector3df lookdir = savesatrt - SceneManager->getActiveCamera()->getAbsolutePosition();
            if (lookdir.getLength() < 20)
                return;
            lookdir.normalize();

            core::vector3df view(SceneManager->getActiveCamera()->getTarget() - SceneManager->getActiveCamera()->getAbsolutePosition());
            view.normalize();

            f32 angle = lookdir.dotProduct(direction)/*/(lookdir.getLength()*direction.getLength())*/;

            if (angle < 0.9999f && angle > -0.9999f)
            {
                core::vector3df updown = direction.crossProduct(lookdir);
                updown.normalize();
                core::vector3df normal = direction.crossProduct(updown);
                video::S3DVertex vertices[4];
                u16 indices[] = { 0,1,2,1,3,2 };

                vertices[0] = video::S3DVertex(m_end - updown * m_thickness * 0.5f, normal, video::SColor(255, 255, 255, 255), core::vector2d<f32>(0, 0));
                vertices[1] = video::S3DVertex(m_end + updown * m_thickness * 0.5f, normal, video::SColor(255, 255, 255, 255), core::vector2d<f32>(0, 1));
                vertices[2] = video::S3DVertex(m_start - updown * m_thickness * 0.5f, normal, video::SColor(255, 255, 255, 255), core::vector2d<f32>(1, 0));
                vertices[3] = video::S3DVertex(m_start + updown * m_thickness * 0.5f, normal, video::SColor(255, 255, 255, 255), core::vector2d<f32>(1, 1));

                driver->setMaterial(material);
                driver->drawIndexedTriangleList(&vertices[0], 4, &indices[0], 2);
                //driver->draw3DLine(m_start, m_end, video::SColor(255,255,0,0));
            }
            else
            {
                //printf("daw dot\n");

                core::vector3df horizontal = SceneManager->getActiveCamera()->getUpVector().crossProduct(view);
                horizontal.normalize();
                horizontal *= 0.5f * m_thickness;

                core::vector3df vertical = horizontal.crossProduct(view);
                vertical.normalize();
                vertical *= 0.5f * m_thickness;
                view *= -1.0f;

                video::S3DVertex vertices[4];
                u16 indices[] = { 0,1,2,1,3,2 };

                //decide to use end or start
                core::vector3df point;

                point = m_end;

                vertices[0] = video::S3DVertex(point - horizontal - vertical, view, video::SColor(255, 255, 255, 255), core::vector2d<f32>(0, 0));
                vertices[1] = video::S3DVertex(point + horizontal - vertical, view, video::SColor(255, 255, 255, 255), core::vector2d<f32>(0, 1));
                vertices[2] = video::S3DVertex(point - horizontal + vertical, view, video::SColor(255, 255, 255, 255), core::vector2d<f32>(1, 0));
                vertices[3] = video::S3DVertex(point + horizontal + vertical, view, video::SColor(255, 255, 255, 255), core::vector2d<f32>(1, 1));

                driver->setMaterial(material2);
                driver->drawIndexedTriangleList(&vertices[0], 4, &indices[0], 2);

                point = m_start;

                vertices[0] = video::S3DVertex(point - horizontal - vertical, view, video::SColor(255, 255, 255, 255), core::vector2d<f32>(0, 0));
                vertices[1] = video::S3DVertex(point + horizontal - vertical, view, video::SColor(255, 255, 255, 255), core::vector2d<f32>(0, 1));
                vertices[2] = video::S3DVertex(point - horizontal + vertical, view, video::SColor(255, 255, 255, 255), core::vector2d<f32>(1, 0));
                vertices[3] = video::S3DVertex(point + horizontal + vertical, view, video::SColor(255, 255, 255, 255), core::vector2d<f32>(1, 1));

                driver->setMaterial(material2);
                driver->drawIndexedTriangleList(&vertices[0], 4, &indices[0], 2);
            }
        }

        const core::aabbox3d<f32>& CBeamNode::getBoundingBox() const
        {
            return Box;
        }

        u32 CBeamNode::getMaterialCount() const
        {
            return 2;
        }

        video::SMaterial& CBeamNode::getMaterial(u32 i)
        {
            if (i == 0)
                return material;
            else if (i == 1)
                return material2;

            return material;
        }
    }
}