// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include "src/simulation/scene/SceneComposite.h"
#include "src/simulation/scene/components/CoordinateSystem.h"
#include "src/simulation/scene/components/PointLight.h"
#include "src/simulation/scene/components/Projector.h"
#include "src/simulation/scene/components/ProjectionSurface.h"
#include "src/simulation/scene/components/VirtualRGBDCamera.h"

/**
 * The complete scene.
 */
class SurgicalScene : public SceneComposite {
private:
    /**
    * Function to convert degrees to radians.
    */
    static float degToRad(const float degrees) {
        return degrees / 180.f * M_PI;
    }

public:
    SurgicalScene(){
        //
        add(std::make_shared<ProjectionPlane>());

        // Add a projector to the scene:
        {
            std::shared_ptr<Projector> projector = std::make_shared<Projector>(0);
            projector->transformation = Mat4f::translation(0.6f, 2.5f, 0.75f) * Mat4f::rotationY(degToRad(-10)) * Mat4f::rotationX(degToRad(26)) * Mat4f::rotationZ(degToRad(245.5f)) * Mat4f::rotationY(degToRad(90));
            Data::instance.projectors.push_back(projector);
            add(projector);

            {
                std::shared_ptr<VirtualRGBDCamera> rgbdCamera = std::make_shared<VirtualRGBDCamera>();
                rgbdCamera->transformation = Mat4f::translation(-0.05f, 0.1f, -0.065f) * projector->transformation;
                rgbdCamera->active = true;
                add(rgbdCamera);
                Data::instance.rgbdCameras.push_back(rgbdCamera);
            }
        }

        {
            std::shared_ptr<Projector> projector2 = std::make_shared<Projector>(1);
            projector2->transformation = Mat4f::translation(0.6f, 2.5f, -0.75f) * Mat4f::rotationY(degToRad(10)) * Mat4f::rotationX(degToRad(-26)) * Mat4f::rotationZ(degToRad(245.5f)) * Mat4f::rotationY(degToRad(90));
            Data::instance.projectors.push_back(projector2);
            add(projector2);

            {
                std::shared_ptr<VirtualRGBDCamera> rgbdCamera = std::make_shared<VirtualRGBDCamera>();
                rgbdCamera->transformation = Mat4f::translation(-0.05f, 0.1f, 0.065f) * projector2->transformation;
                rgbdCamera->active = false;
                add(rgbdCamera);
                Data::instance.rgbdCameras.push_back(rgbdCamera);
            }
        }

        // Add a third projector to the scene:
        {
            std::shared_ptr<Projector> projector3 = std::make_shared<Projector>(2);
            projector3->transformation = Mat4f::translation(-0.6f, 2.5f, 0.f) * Mat4f::rotationY(M_PI) * Mat4f::rotationZ(degToRad(65.f + 180.f)) * Mat4f::rotationY(degToRad(90));
            Data::instance.projectors.push_back(projector3);
            add(projector3);

            {
                std::shared_ptr<VirtualRGBDCamera> rgbdCamera = std::make_shared<VirtualRGBDCamera>();
                rgbdCamera->transformation = Mat4f::translation(0.05f, 0.1f, 0.f) * projector3->transformation;
                rgbdCamera->active = false;
                add(rgbdCamera);
                Data::instance.rgbdCameras.push_back(rgbdCamera);
            }
        }

        // A simple coordinate system:
        std::shared_ptr<CoordinateSystem> coordinateSystem = std::make_shared<CoordinateSystem>();
        add(coordinateSystem);

        // Add a light Source:
        std::shared_ptr<PointLight> pointLight = std::make_shared<PointLight>();
        pointLight->transformation = Mat4f::translation(1.f, 2.5f, 1.f);
        pointLight->setColor(Vec4f(0.2f, 0.2f, 0.2f, 1.0f));
        add(pointLight);

        std::shared_ptr<PointLight> pointLight2 = std::make_shared<PointLight>();
        pointLight2->transformation = Mat4f::translation(1.f, 2.5f, -1.f);
        pointLight2->setColor(Vec4f(0.2f, 0.2f, 0.2f, 1.0f));
        add(pointLight2);

        std::shared_ptr<PointLight> pointLight3 = std::make_shared<PointLight>();
        pointLight3->transformation = Mat4f::translation(-1.f, 2.5f, 1.f);
        pointLight3->setColor(Vec4f(0.2f, 0.2f, 0.2f, 1.0f));
        add(pointLight3);

        std::shared_ptr<PointLight> pointLight4 = std::make_shared<PointLight>();
        pointLight4->transformation = Mat4f::translation(-1.f, 2.5f, -1.f);
        pointLight4->setColor(Vec4f(0.2f, 0.2f, 0.2f, 1.0f));
        add(pointLight4);
    }

    void render(SceneData& data, const Mat4f parentModel) override {
        SceneComposite::render(data, parentModel);
    }
};
