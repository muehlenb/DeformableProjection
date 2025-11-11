#pragma once

#include "src/simulation/scene/SceneComponent.h"

#include "src/gl/Mesh.h"
#include "src/gl/Shader.h"

class CoordinateSystem : public SceneComponent {
    static std::shared_ptr<Mesh> cubeMesh;
    static std::shared_ptr<Shader> unlitShader;
    static int instanceCounter;

public:
    /**
     * Construct the coordinate system and load meshes and shader for it.
     */
    CoordinateSystem(){
        ++instanceCounter;

        // Load cube mesh if not already loaded:
        if(cubeMesh == nullptr)
            cubeMesh = std::make_shared<Mesh>(CMAKE_SOURCE_DIR "/models/unit-cube.obj");

        // Load shader if not already loaded:
        if(unlitShader == nullptr)
            unlitShader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/unshaded.vert", CMAKE_SOURCE_DIR "/shader/unshaded.frag");
    }

    /**
     * Clean memory
     */
    ~CoordinateSystem(){
        // If no instance exists anymore, free memory:
        if(--instanceCounter == 0) {}
    }

    /**
     * Is called every time this component should be rendered.
     */
    void render(SceneData& sceneData, const Mat4f parentModel) override {
        const Mat4f model = parentModel * transformation;

        unlitShader->bind();
        unlitShader->setUniform("projection", sceneData.projection);
        unlitShader->setUniform("view", sceneData.view);

        unlitShader->setUniform("model", model * Mat4f::scale(0.1f, 0.01f, 0.01f) * Mat4f::translation(1,0,0));
        unlitShader->setUniform("color", Vec4f(1.0f, 0.0f, 0.0f));
        cubeMesh->render();

        unlitShader->setUniform("model", model * Mat4f::scale(0.0101f, 0.11f, 0.0101f) * Mat4f::translation(0,0.899f,0));
        unlitShader->setUniform("color", Vec4f(0.0f, 1.0f, 0.0f));
        cubeMesh->render();

        unlitShader->setUniform("model", model * Mat4f::scale(0.01f, 0.01f, 0.1f) * Mat4f::translation(0,0,1));
        unlitShader->setUniform("color", Vec4f(0.0f, 0.0f, 1.0f));
        cubeMesh->render();
    }
};

std::shared_ptr<Mesh> CoordinateSystem::cubeMesh = nullptr;
std::shared_ptr<Shader> CoordinateSystem::unlitShader = nullptr;
int CoordinateSystem::instanceCounter = 0;
