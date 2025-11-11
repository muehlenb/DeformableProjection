// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Yaroslav Purgin
#pragma once

#include "src/simulation/scene/SceneComponent.h"

#include "src/processing/OrganizedPointCloud.h"

#include "src/gl/TextureFBO.h"
#include "src/gl/Mesh.h"
#include "src/gl/Shader.h"
#include "src/math/Vec4.h"
#include "src/math/Mat4.h"

#include "src/Data.h"

// Include white noise texture 2D:
#include "src/simulation/util/NoiseTexture2D.h"

// Scene:
#include "src/simulation/scene/SceneComposite.h"

/**
 * Represents a virtual RGBD camera in the scene.
 * Simulates Azure Kinekt DK, specifically the NFOV unbinned mode.
 */
class VirtualRGBDCamera : public SceneComponent {
    static std::shared_ptr<Mesh> mesh;
    static std::shared_ptr<Texture2D> texture;
    static std::shared_ptr<Shader> shader;
    static std::shared_ptr<Shader> sndPassShader;
    static int instanceCounter;

    // Quad mesh to render on:
    Mesh quadMesh;

    // Model matrix in world space:
    Mat4f modelWS = Mat4f();

    // FBOs:
    TextureFBO fstPassFBO;
    TextureFBO dataFBO;

    NoiseTexture2D noiseTexture;

    // Vector for 3d data:
    std::vector<uint16_t> dataDepth;

    // Vector for color data:
    std::vector<Vec4b> dataBGRA;

    float* lookupImageTo3D = nullptr;
    float* lookup3DToImage = nullptr;
    int lookup3DToImageSize = 1024;

    float fovX = 75.0;
    float fovY = 65.0;

    float halfFovXRad = fovX / 2.0 / 180 * M_PI;
    float halfFovYRad = fovY / 2.0 / 180 * M_PI;

public:
    /** Whether the camera data should be used */
    bool active = true;

    // Camera resolution:
    const int width = 640;
    const int height = 576;

    const float horizontalFOV = 75.0f;
    const float verticalFOV = 65.0f;

    /**
     * Construct the virtual RGBD camera and load meshes and shader for it.
     */
    VirtualRGBDCamera()
        // Create our FBO:
        : quadMesh(std::vector<Triangle>({
              Triangle(Vertex(Vec4f(-1.0f, -1.0f, -1.0f)), Vertex(Vec4f(1.0f, -1.0f, -1.0f)), Vertex(Vec4f(1.0f, 1.0f, -1.0f))),
              Triangle(Vertex(Vec4f(-1.0f, -1.0f, -1.0f)), Vertex(Vec4f(1.0f, 1.0f, -1.0f)), Vertex(Vec4f(-1.0f, 1.0f, -1.0f)))
          }))
        , fstPassFBO(std::vector<TextureType>({
            TextureType(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE),                            // Color texture type
            TextureType(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE),    // Depth texture type
          }))
        , dataFBO(std::vector<TextureType>({
            TextureType(GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT),                                  // 3D Data texture type
          }))
        , noiseTexture(1000, 1000)
    {
        ++instanceCounter;

        // Load camera mesh if not already loaded:
        if (mesh == nullptr)
            mesh = std::make_shared<Mesh>(CMAKE_SOURCE_DIR "/models/depth-sensor/DepthSensor.obj");

        // Load camera texture if not already loaded:
        if (texture == nullptr)
            texture = std::make_shared<Texture2D>(CMAKE_SOURCE_DIR "/models/depth-sensor/simplifiedDepthSensorTexture.jpg");
        
        // Load shader if not already loaded:
        if (shader == nullptr)
            shader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/simpleLighting.vert", CMAKE_SOURCE_DIR "/shader/simpleLighting.frag");

        // Load shader if not already loaded:
        if (sndPassShader == nullptr)
            sndPassShader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/rgbdSndPass.vert", CMAKE_SOURCE_DIR "/shader/rgbdSndPass.frag");

        // Allocate memory for the texture data:
        GLsizei dataSize = width * height;
        dataDepth.resize(dataSize);
        dataBGRA.resize(dataSize);

        lookupImageTo3D = new float[dataSize * 2];
        lookup3DToImage = new float[lookup3DToImageSize * lookup3DToImageSize * 2];

        for(int y = 0; y < height; ++y){
            for(int x = 0; x < width; ++x){
                float lookupX = std::tan(halfFovXRad) * ((x / float(width-1)) * 2.0 - 1.0);
                float lookupY = std::tan(halfFovYRad) * ((y / float(height-1)) * 2.0 - 1.0);

                int i = y * width + x;

                lookupImageTo3D[i*2] = lookupX;
                lookupImageTo3D[i*2+1] = lookupY;
            }
        }

        for(int y = 0; y < lookup3DToImageSize; ++y){
            for(int x = 0; x < lookup3DToImageSize; ++x){
                float relX = x / float(lookup3DToImageSize - 1) * 2 - 1;
                float relY = y / float(lookup3DToImageSize - 1) * 2 - 1;

                float imageX = ((relX / tan(halfFovXRad)) + 1) * 0.5f * (width - 1);
                float imageY = ((relY / tan(halfFovYRad)) + 1) * 0.5f * (height - 1);

                int idx = (y * lookup3DToImageSize + x) * 2;
                lookup3DToImage[idx] = imageX;
                lookup3DToImage[idx+1] = imageY;
            }
        }
    }

    /**
     * Clean memory
     */
    ~VirtualRGBDCamera() {
        // If no instance exists anymore, free memory:
        if (--instanceCounter == 0) {}

        delete[] lookupImageTo3D;
        lookupImageTo3D = nullptr;
    }

    /**
     * Is called every time this component should be rendered.
     */
    void render(SceneData& sceneData, const Mat4f parentModel) override {
        modelWS = parentModel * transformation;
        
        shader->bind();
        shader->setUniform("projection", sceneData.projection);
        shader->setUniform("view", sceneData.view);
        shader->setUniform("model", modelWS);
        //shader->setUniform("color", Vec4f(0.7f, 0.7f, 0.7f));
        shader->setUniform("useMaterialColorTexture", true);
        texture->bind(0);
        shader->setUniform("materialColorTexture", 0);
        // TODO: fetch correct values
        constexpr float shininess = 100.0f;
        shader->setUniform("shininess", shininess);
        shader->setUniform("ambient_color", sceneData.ambientColor);

        // Set data for the lights:
        sceneData.setLightUniforms(shader);

        mesh->render();
    }

    /** Renders the rgbd image */
    void renderRGBD(SceneComposite& scene) {
        // First pass:
        {
            SceneData rgbdSceneData(SD_SIMULATED_CAMERA);

            // FIXME: renders gizmos (which may end up in the point cloud)
            // Projection matrix:
            rgbdSceneData.projection = getProjection();

            // View matrix (transform all objects manually here, instead of using the inverse of a model matrix):
            rgbdSceneData.view = getView();

            // Activate the FBO to render to its textures instead
            // of rendering directly to the screen:
            fstPassFBO.bind(width, height);

            // Setup viewport and clear color & depth image:
            glViewport(0, 0, width, height);
            glClearColor(0.6f, 0.725f, 0.8f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Prepare scene data:
            scene.prepare(rgbdSceneData, Mat4f());

            // Render the scene:
            scene.render(rgbdSceneData, Mat4f());
        }

        // Second pass:
        {
            dataFBO.bind(width, height);

            sndPassShader->bind();

            // Bind depth texture from pass 1:
            fstPassFBO.getTexture2D(1).bind(0);
            sndPassShader->setUniform("renderedDepthTexture", 0);

            // Bind depth texture from pass 1:
            noiseTexture.bind(1);
            sndPassShader->setUniform("noiseTexture", 1);
            sndPassShader->setUniform("time", Data::instance.runtime);
            sndPassShader->setUniform("simulateNoise", Data::instance.simulateRGBDNoise);

            // Render quad mesh:
            quadMesh.render();
        }

        // Read Point Cloud data from GPU to RAM:
        {
            dataFBO.getTexture2D(0).bind();
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, dataDepth.data());
        }

        // Read Color data from GPU to RAM:
        {
            fstPassFBO.getTexture2D(0).bind();
            glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, dataBGRA.data());
        }

    }

    std::shared_ptr<OrganizedPointCloud> getOrganizedPointCloud(){
        const size_t size = dataDepth.size();
        auto pc = std::make_shared<OrganizedPointCloud>(width, height);
        // TODO: maybe only initialize multiple arrays at the beginning
        pc->depth = new uint16_t[size];
        pc->colors = new Vec4b[size];
        pc->lookupImageTo3D = lookupImageTo3D;
        pc->lookup3DToImage = lookup3DToImage;
        pc->lookup3DToImageSize = lookup3DToImageSize;
        pc->modelMatrix = transformation;
        pc->usageFlags = CAMERA_RESPONSIBILITY_GESTURES | CAMERA_RESPONSIBILITY_OCCLUSION | CAMERA_RESPONSIBILITY_RECTIFICATION;

        // Invalid pixels are drawn but placed inside the RGBD Camera:
        std::memcpy(pc->depth, dataDepth.data(), size * sizeof(uint16_t));
        std::memcpy(pc->colors, dataBGRA.data(), size * sizeof(Vec4b));

        return pc;
    }

    /** Returns the projection matrix for this camera. */
    Mat4f getProjection() const {
        return Mat4f::customPerspectiveTransformation(horizontalFOV, verticalFOV);
    }

    /** Returns the view matrix for this camera. */
    Mat4f getView() {
        return modelWS.inverse(); 
    }

    /** Returns the model matrix in world space. */
    Mat4f getPositionWS() const {
        return modelWS;
    }

    /** Provides access to the distance texture as a Texture2D object. */
    Texture2D& getColorTexture2D() {
        return fstPassFBO.getTexture2D(0);
    }

    /** Provides access to the data texture as a Texture2D object. */
    Texture2D& getDataTexture2D() {
        return dataFBO.getTexture2D(0);
    }

    TextureFBO& getFBO(){
        return dataFBO;
    }
};

std::shared_ptr<Mesh> VirtualRGBDCamera::mesh = nullptr;
std::shared_ptr<Texture2D> VirtualRGBDCamera::texture = nullptr;
std::shared_ptr<Shader> VirtualRGBDCamera::shader = nullptr;
std::shared_ptr<Shader> VirtualRGBDCamera::sndPassShader = nullptr;
int VirtualRGBDCamera::instanceCounter = 0;
