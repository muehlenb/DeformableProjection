#pragma once

#include "src/simulation/scene/SceneComponent.h"

#include "src/Data.h"

#include "src/simulation/scene/components/Projector.h"

#include "src/gl/Mesh.h"
#include "src/gl/Shader.h"

class ProjectionPlane : public SceneComponent {
    static std::shared_ptr<Mesh> surfaceMesh;

    static std::shared_ptr<Texture2D> surfaceTexture;
    static std::shared_ptr<Shader> shader;
    static int instanceCounter;

public:
    /**
     * Construct the operating table and load meshes and shader for it.
     */
    ProjectionPlane(){
        ++instanceCounter;

        if(surfaceMesh == nullptr)
            surfaceMesh = std::make_shared<Mesh>(CMAKE_SOURCE_DIR "/models/projection-plane/ProjectionPlane.obj");

        if(surfaceTexture == nullptr)
            surfaceTexture = std::make_shared<Texture2D>(CMAKE_SOURCE_DIR "/textures/projectionSurface.jpg");

        // Load shader if not already loaded:
        if(shader == nullptr)
            shader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/projectionSurface.vert", CMAKE_SOURCE_DIR "/shader/projectionSurface.frag");
    }

    /**
     * Clean memory
     */
    ~ProjectionPlane(){
        // If no instance exists anymore, free memory:
        if(--instanceCounter == 0) {}
    }

    /**
     * Is called every time this component should be rendered.
     */
    void render(SceneData& sceneData, const Mat4f parentModel) override {
        // If operating table should to be in scene, don't render at all:
        if(!Data::instance.isProjectionPlaneInScene)
            return;

        // If operating table should not be shown (but perceived in the cameras),
        // disable in main pass:
        if(sceneData.type == SD_MAIN && !Data::instance.showProjectionPlane)
            return;

        Vec4f posOffset = Data::instance.projectionPlanePositionOffset;
        transformation = Mat4f::translation(posOffset.x, posOffset.y, posOffset.z);

        const Mat4f model = parentModel * transformation;

        shader->bind();
        shader->setUniform("projection", sceneData.projection);
        shader->setUniform("view", sceneData.view);
        shader->setUniform("model", model);
        shader->setUniform("ambient_color", sceneData.ambientColor);

        // Set data for the projectors:
        std::vector<std::shared_ptr<Projector>> projectors = Data::instance.projectors;
        shader->setUniform("proj_num", static_cast<int>(projectors.size()));

        for (int i = 0; i < projectors.size(); i++) {
            std::shared_ptr<Projector> projector = projectors[i];
            std::string projIdStr = std::to_string(i);

            projector->getTexture().bind(i+1);
            shader->setUniform("images[" + projIdStr + "]", i+1);
            shader->setUniform("projectors[" + projIdStr + "].is_active", projector->active);

            // Assumes that projector parentModel is identity!
            shader->setUniform("projectors[" + projIdStr + "].model", projector->transformation);
            shader->setUniform("projectors[" + projIdStr + "].invModel", projector->transformation.inverse());
            shader->setUniform("projectors[" + projIdStr + "].projectionMatrix", projector->projectionMatrix);
        }

        // Set data for the lights:
        sceneData.setLightUniforms(shader);

        shader->setUniform("bumpIntensity", Data::instance.projectionPlaneBumpIntensity);

        shader->setUniform("projectingIntensity", Data::instance.projectorIntensity);
        shader->setUniform("simulateFolds", Data::instance.projectionPlaneSimulateFolds);
        shader->setUniform("simulateWind", Data::instance.projectionPlaneSimulateWind);
        shader->setUniform("useTexture", false);
        shader->setUniform("time", Data::instance.runtime);
        shader->setUniform("uvMultiplier", 4.f);
        shader->setUniform("shininess", 10.0f);
        surfaceTexture->bind();
        shader->setUniform("albedoTexture", 0);
        shader->setUniform("useTexture", true);
        shader->setUniform("color", Vec4f(0.8f, 0.8f, 0.8f));
        surfaceMesh->render();
    }
};
