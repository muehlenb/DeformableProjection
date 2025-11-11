#pragma once

#include <algorithm>
#include "src/processing/OrganizedPointCloud.h"
#include <glad/glad.h>

#include "src/processing/blendpcr/Rectification.h"

#include "src/processing/blendpcr/ShadowAvoidance.h"
#include "src/processing/uirenderer/ShadowTile.h"

#include "src/processing/uirenderer/UIRenderer.h"
#include "src/processing/uirenderer/StaticImageUIRenderer.h"

#include "src/Data.h"

#include "src/simulation/scene/SceneComponent.h"

#include <opencv2/objdetect/aruco_detector.hpp>

/**
 * Represents a projection rectifier based on a simple height map
 * reconstruction.
 *
 * The transformation matrix defines where the rectification plane is
 * which is by default of a size (1x1) and goes from -0.5 to 0.5.
 */
class RectifiedProjection : public SceneComponent {
    std::shared_ptr<UIRenderer>& uiRenderer;

public:
    static RectifiedProjection* instance;

    std::shared_ptr<Rectification> rectification;
    std::shared_ptr<ShadowAvoidance> shadowAvoidance;

    int cntr = 0;

    bool updateShadowAvoidanceMap = false;

    RectifiedProjection(std::shared_ptr<UIRenderer>& uiRenderer)
        : uiRenderer(uiRenderer)
        , shadowAvoidance(std::make_shared<ShadowAvoidance>(uiRenderer)){
        instance = this;

        rectification = std::make_shared<Rectification>(uiRenderer);

        Data::instance.virtualDisplayTransform = transformation = Mat4f::translation(0.4f, Data::instance.virtualDisplayHeight, 0.0f) * Mat4f::rotationZ(-5.f * M_PI / 180.f) * Mat4f::scale(0.16875f, 1.0f, 0.3f);
    }

    void render(SceneData& sceneData, Mat4f parentModel) override {
        render(sceneData, parentModel, false, -1);
    }

    void render(SceneData& sceneData, Mat4f parentModel, bool useWireframe, int projectorID){
        Vec4d toTargetPos = Data::instance.virtualDisplaySpectator.toVec4d() - getPosition().toVec4d();
        toTargetPos.z *= Data::instance.rectifyYRotationFactor;
        toTargetPos = toTargetPos.normalized();

        Mat4f rotationMatrix = Mat4f::switchAxesMatrix(-3, -1, 2);
        if(toTargetPos.dot(Vec4d(0,1,0,0)) < 0.99999999){
            rotationMatrix = Mat4d::getRotationMatrix(Vec4d(0,0,1,0), toTargetPos.normalized(), Vec4d(0,1,0,0), false, 0.99999999).toMat4f();
        }
        Data::instance.virtualDisplayTransform = transformation = Mat4f::translation(0.0f, Data::instance.virtualDisplayHeight, 0.0f) * rotationMatrix * Mat4f::scale(0.4f * 1.0f, 0.225f * 1.0f, 1.0f);

        rectification->render(sceneData, parentModel, useWireframe, projectorID);
    }
};
