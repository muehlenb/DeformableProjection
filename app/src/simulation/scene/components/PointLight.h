#pragma once

#include "src/simulation/scene/SceneComponent.h"

#include "src/gl/Mesh.h"
#include "src/gl/Shader.h"

class PointLight : public SceneComponent {
    static std::shared_ptr<Mesh> lightMesh;
    static std::shared_ptr<Shader> unlitShader;
    static int instanceCounter;

    Vec4f intensity;

public:
    /**
     * Construct the point light and load meshes and shader for it.
     */
    PointLight(){
        ++instanceCounter;

        // Load point light mesh if not already loaded:
        if(lightMesh == nullptr)
            lightMesh = std::make_shared<Mesh>(CMAKE_SOURCE_DIR "/models/light/PointLight.obj");

        // Load shader if not already loaded:
        if(unlitShader == nullptr)
            unlitShader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/unshaded.vert", CMAKE_SOURCE_DIR "/shader/unshaded.frag");
    }

    /**
     * Clean memory
     */
    ~PointLight(){
        // If no instance exists anymore, free memory:
        if (--instanceCounter == 0) {
            lightMesh = nullptr;
            unlitShader = nullptr;
        }
    }

    /**
     * Prepare sceneData with point light position.
     */
    void prepare(SceneData& sceneData, const Mat4f parentModel) override{
        const Mat4f model = parentModel * transformation;
        sceneData.lights.emplace_back(model.getPosition(), intensity);
    }

    /**
     * Is called every time this component should be rendered.
     */
    void render(SceneData& sceneData, const Mat4f parentModel) override {
        const Mat4f model = parentModel * transformation;

        unlitShader->bind();
        unlitShader->setUniform("projection", sceneData.projection);
        unlitShader->setUniform("view", sceneData.view);
        unlitShader->setUniform("model", model);
        unlitShader->setUniform("color", Vec4f(1.f, 1.f, 1.f));
        lightMesh->render();
    }

    void setColor(const Vec4f color) {
        intensity = color;
    }
};

std::shared_ptr<Mesh> PointLight::lightMesh = nullptr;
std::shared_ptr<Shader> PointLight::unlitShader = nullptr;
int PointLight::instanceCounter = 0;
