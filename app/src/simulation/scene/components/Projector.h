// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Yaroslav Purgin
#pragma once

#include "src/stb_image/stb_image.h"

#include "src/simulation/scene/SceneComponent.h"

#include "src/gl/Mesh.h"
#include "src/gl/Shader.h"
#include "src/gl/TextureFBO.h"

#include "src/simulation/scene/components/RectifiedProjection.h"


class Projector : public SceneComponent {
    static std::shared_ptr<Mesh> projectorMesh;
    static std::shared_ptr<Texture2D> textureActive;
    static std::shared_ptr<Texture2D> textureInactive;
    static std::shared_ptr<Shader> shader;
    static int instanceCounter;

public:
    int projectorID;

    /** Horizontal and vertical fields of view of the projector */
    float h_fov = 25.166f;
    float v_fov = h_fov / 16.0f * 9.0f;
    float h_offset = 0.f;
    float v_offset = 0.f;

    Mat4f projectionMatrix;

    /** The FBO where the projecting texture is rendered to */
    TextureFBO imageFBO;

    /**
     * Should the rectified image rendered to the fbo? This should
     * be deactivated when calibrating, since the calibration process
     * will render to the FBO.
     */
    bool shouldRenderRectifiedImage = true;

    /** Whether the projector is turned on */
    bool active = true;
    /**
     * Construct the projector and load meshes and shader for it.
     */
    Projector(int projectorID)
        : projectorID(projectorID)
        , imageFBO({
            TextureType(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE),
            TextureType(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE)
        }){
        ++instanceCounter;

        // Load projector mesh if not already loaded:
        if(projectorMesh == nullptr)
            projectorMesh = std::make_shared<Mesh>(CMAKE_SOURCE_DIR "/models/projector/Projector.obj");


        stbi_set_flip_vertically_on_load(true);
        // Load projector texture if not already loaded:
        if (textureActive == nullptr)
            textureInactive = textureActive = std::make_shared<Texture2D>(CMAKE_SOURCE_DIR "/models/projector/simplifiedProjectorTexture.jpg");

        stbi_set_flip_vertically_on_load(false);

        // Load shader if not already loaded:
        if(shader == nullptr)
            shader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/simpleLighting.vert", CMAKE_SOURCE_DIR "/shader/simpleLighting.frag");

        calculateProjection();

        int imageWidth = Data::instance.projectorImageWidth;
        int imageHeight = Data::instance.projectorImageHeight;

        imageFBO.bind(imageWidth, imageHeight);
    }

    /**
     * Clean memory
     */
    ~Projector(){
        // If no instance exists anymore, free memory:
        if (--instanceCounter == 0) {}
    }

    void renderRectifiedImage(Mat4f model){
        GLint storedFBO;
        GLint storedViewport[4];
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&storedFBO);
        glGetIntegerv(GL_VIEWPORT, storedViewport);

        int imageWidth = Data::instance.projectorImageWidth;
        int imageHeight = Data::instance.projectorImageHeight;

        imageFBO.bind(imageWidth, imageHeight);
        glViewport(0, 0, imageWidth, imageHeight);
        float minBrightness = Data::instance.projectorMinBrightness;
        glClearColor(minBrightness, minBrightness, minBrightness, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        SceneData data(SD_NONE);
        data.view = (model).inverse();
        data.projection = projectionMatrix;
        RectifiedProjection::instance->render(data, Mat4f(), false, projectorID);

        glBindFramebuffer(GL_FRAMEBUFFER, storedFBO);
        glViewport(storedViewport[0], storedViewport[1], storedViewport[2], storedViewport[3]);
    }

    /**
     * Prepare the rectified image.
     */
    void prepare(SceneData& sceneData, const Mat4f parentModel) override{
        Mat4f model = parentModel * transformation;

        glDisable(GL_CULL_FACE);
        if(sceneData.type != SD_SIMULATED_CAMERA && shouldRenderRectifiedImage)
            renderRectifiedImage(model);
    }

    /** Returns the rectified texture. */
    Texture2D& getTexture(){
        return imageFBO.getTexture2D(0);
    }

    /**
     * Is called every time this component should be rendered.
     */
    void render(SceneData& sceneData, const Mat4f parentModel) override{
        if(sceneData.type == SD_SIMULATED_CAMERA)
            return;

        const Mat4f model = parentModel * transformation;

        shader->bind();
        shader->setUniform("projection", sceneData.projection);
        shader->setUniform("view", sceneData.view);
        shader->setUniform("model", model);

        shader->setUniform("useMaterialColorTexture", true);
        (active ? textureActive : textureInactive)->bind(0);
        shader->setUniform("materialColorTexture", 0);

        constexpr float shininess = 100.0f;
        shader->setUniform("shininess", shininess);
        shader->setUniform("ambient_color", sceneData.ambientColor);

        // Set data for the lights:
        sceneData.setLightUniforms(shader);
        projectorMesh->render();
    }

    /** Calculates the projection matrix for this projector. */
    void calculateProjection() {
        projectionMatrix = Mat4f::customPerspectiveTransformation(h_fov, v_fov, h_offset, v_offset);
    };
};
