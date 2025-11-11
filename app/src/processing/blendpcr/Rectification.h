// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include <functional>

#include "src/processing/uirenderer/UIRenderer.h"
#include "src/processing/uirenderer/ShadowTile.h"
#include <glad/glad.h>

#include "src/Data.h"

#include "src/simulation/scene/SceneData.h"

#include "src/processing/OrganizedPointCloud.h"
#include "src/processing/blendpcr/CameraPasses.h"

#define PROJECTOR_COUNT 3

/**
 * Represents a projection rectifier based on a simple height map
 * reconstruction.
 *
 * The transformation matrix defines where the rectification plane is
 * which is by default of a size (1x1) and goes from -0.5 to 0.5.
 */
class Rectification {
    std::shared_ptr<UIRenderer>& uiRenderer;

    bool isInitialized = false;

    // The fbo and textures for the separate screen rendering passes:
    unsigned int fbo_screen[CAMERA_COUNT];
    unsigned int texture2D_screenColor[CAMERA_COUNT];
    unsigned int texture2D_screenVertices[CAMERA_COUNT];
    unsigned int texture2D_screenNormals[CAMERA_COUNT];
    unsigned int texture2D_screenDepth[CAMERA_COUNT];

    // The fbo and texture for the major cam pass:
    unsigned int fbo_majorCam;
    unsigned int texture2D_majorCam;

    // The fbo and texture for the camera weights:
    unsigned int fbo_cameraWeights;
    unsigned int texture2D_cameraWeightsA; // Camera 0-3
    unsigned int texture2D_cameraWeightsB; // Camera 4-7

    int fbo_screen_width = -1;
    int fbo_screen_height = -1;

    int fbo_mini_screen_width = -1;
    int fbo_mini_screen_height = -1;

    Mat4f sAMatrix;

    std::shared_ptr<Shader> customRenderShader = nullptr;

    std::function<void(std::shared_ptr<Shader>, int, int)> customShaderBindingsCallback;

    /**
     * Define all the shaders for the screen passes:
     */
    Shader renderShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr_rect/screen/separateRendering.vert", CMAKE_SOURCE_DIR "/shader/blendpcr_rect/screen/separateRendering.frag");

    Shader majorCamShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/majorCam.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/majorCam.frag");
    Shader cameraWeightsShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/cameraWeights.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/cameraWeights.frag");
    Shader blendingShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/blending.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/blending.frag");

    Shader vertexBlendingShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr_rect/screen/blending.vert", CMAKE_SOURCE_DIR "/shader/blendpcr_rect/screen/blending.frag");

    void generateAndBind2DTexture(
        unsigned int& texture,
        unsigned int width,
        unsigned int height,
        unsigned int internalFormat, // eg. GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_R32F,...
        unsigned int format, // e.g. only GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA without Sizes!
        unsigned int type, // egl. GL_UNSIGNED_BYTE, GL_FLOAT
        unsigned int filter, // GL_LINEAR or GL_NEAREST
        void* data = NULL
        ){
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);

        if(filter != GL_NONE){
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        }

        if(format == GL_DEPTH_COMPONENT){
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        }
    };

    void init(){
        int mainViewport[4];
        glGetIntegerv(GL_VIEWPORT, mainViewport);

        int originalFramebuffer;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &originalFramebuffer);

        // If screen size changed:
        if(mainViewport[2] != fbo_screen_width || mainViewport[3] != fbo_screen_height){
            for(unsigned int cameraID = 0; cameraID < CAMERA_COUNT; ++cameraID){
                // Delete old frame buffer + texture:
                if(fbo_screen_width != -1){
                    glDeleteFramebuffers(1, &fbo_screen[cameraID]);
                    glDeleteTextures(1, &texture2D_screenColor[cameraID]);
                    glDeleteTextures(1, &texture2D_screenVertices[cameraID]);
                    glDeleteTextures(1, &texture2D_screenNormals[cameraID]);
                    glDeleteTextures(1, &texture2D_screenDepth[cameraID]);
                }

                // Generate resources for SCREEN SPACE PASS:
                {
                    std::cout << "Generate FBO Screen" << std::endl;

                    glGenFramebuffers(1, &fbo_screen[cameraID]);
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_screen[cameraID]);

                    generateAndBind2DTexture(texture2D_screenColor[cameraID], mainViewport[2], mainViewport[3], GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_screenColor[cameraID], 0);

                    generateAndBind2DTexture(texture2D_screenVertices[cameraID], mainViewport[2], mainViewport[3], GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture2D_screenVertices[cameraID], 0);

                    generateAndBind2DTexture(texture2D_screenNormals[cameraID], mainViewport[2], mainViewport[3], GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texture2D_screenNormals[cameraID], 0);

                    generateAndBind2DTexture(texture2D_screenDepth[cameraID], mainViewport[2], mainViewport[3], GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, GL_NEAREST);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture2D_screenDepth[cameraID], 0);

                }
            }


            fbo_screen_width = mainViewport[2];
            fbo_screen_height = mainViewport[3];

            // Delete old frame buffer + texture:
            if(fbo_mini_screen_width != -1){
                glDeleteFramebuffers(1, &fbo_majorCam);
                glDeleteTextures(1, &texture2D_majorCam);

                glDeleteFramebuffers(1, &fbo_cameraWeights);
                glDeleteTextures(1, &texture2D_cameraWeightsA);
                glDeleteTextures(1, &texture2D_cameraWeightsB);
            }

            int requestedMiniScreenWidth = mainViewport[2] / 4;
            int requestedMiniScreenHeight = mainViewport[3] / 4;


            glGenFramebuffers(1, &fbo_majorCam);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_majorCam);

            generateAndBind2DTexture(texture2D_majorCam, requestedMiniScreenWidth, requestedMiniScreenHeight, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_majorCam, 0);

            glGenFramebuffers(1, &fbo_cameraWeights);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_cameraWeights);

            generateAndBind2DTexture(texture2D_cameraWeightsA, requestedMiniScreenWidth, requestedMiniScreenHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_cameraWeightsA, 0);

            generateAndBind2DTexture(texture2D_cameraWeightsB, requestedMiniScreenWidth, requestedMiniScreenHeight, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture2D_cameraWeightsB, 0);

            fbo_mini_screen_width = requestedMiniScreenWidth;
            fbo_mini_screen_height = requestedMiniScreenHeight;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);
        isInitialized = true;
    }

public:
    float implicitH = 0.01f;
    float kernelRadius = 10.f;
    float kernelSpread = 1.f;

    bool useColorIndices = false;

    float uploadTime = 0;

    /**
     * When no customRenderShader is given, the transformation and the texture is used for rectification the given texture.
     * Otherwise, the customRenderShader is used for shadow avoidance.
     */
    Rectification(std::shared_ptr<UIRenderer>& uiRenderer, std::shared_ptr<Shader> customRenderShader = nullptr, std::function<void(std::shared_ptr<Shader>, int, int)> shaderCallback = {})
        : uiRenderer(uiRenderer)
        , customRenderShader(customRenderShader)
        , customShaderBindingsCallback(shaderCallback)
    {}

    /**
     *
     */
    ~Rectification(){
        if(fbo_mini_screen_width != -1){
            glDeleteFramebuffers(1, &fbo_majorCam);
            glDeleteTextures(1, &texture2D_majorCam);

            glDeleteFramebuffers(1, &fbo_cameraWeights);
            glDeleteTextures(1, &texture2D_cameraWeightsA);
            glDeleteTextures(1, &texture2D_cameraWeightsB);
        }

        for(int cameraID = 0; cameraID < CAMERA_COUNT; ++cameraID){
            if(fbo_screen_width != -1){
                glDeleteFramebuffers(1, &fbo_screen[cameraID]);
                glDeleteTextures(1, &texture2D_screenColor[cameraID]);
                glDeleteTextures(1, &texture2D_screenVertices[cameraID]);
                glDeleteTextures(1, &texture2D_screenDepth[cameraID]);
            }
        }
    }

    virtual void render(SceneData& sceneData, Mat4f parentModel, bool useWireframe, int projectorID) {
        CameraPasses& pctextures = CameraPasses::getInstance();

        glDisable(GL_BLEND);

        // If opengl resources are not initialized yet, do it:
        init();

        // Check if cameras for rendering are available:
        if(pctextures.usedCameraIDs.size() == 0){
            //std::cout << "ExperimentalPCPreprocessor: NO CAMERAS FOR RENDERING AVAILABLE!" << std::endl;
            return;
        }

        bool isCameraActive[CAMERA_COUNT];
        std::fill_n(isCameraActive, CAMERA_COUNT, false);

        // Iterate over all cameras:
        for(unsigned int i = 0; i < pctextures.currentPointClouds.size(); ++i){
            isCameraActive[i] = true;
        }

        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        int originalFramebuffer;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &originalFramebuffer);

        // Now we render all meshes of each depth camera to a framebuffer:
        for(unsigned int cameraID : pctextures.usedCameraIDs){
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_screen[cameraID]);

            // Draw out point cloud and color texture:
            unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
            glDrawBuffers(3, attachments);

            float clearColor[4] = {0.0, 0.0, 0.0, 0.0};
            glClear(GL_DEPTH_BUFFER_BIT);
            glClearBufferfv(GL_COLOR, 0, clearColor);
            glClearBufferfv(GL_COLOR, 1, clearColor);

            if(customRenderShader == nullptr){
                renderShader.bind();
                renderShader.setUniform("model", pctextures.currentPointClouds[cameraID]->modelMatrix);
                renderShader.setUniform("view", sceneData.view);
                renderShader.setUniform("projection", sceneData.projection);
                renderShader.setUniform("useColorIndices", useColorIndices);

                renderShader.setUniform("projectorID", projectorID);

                renderShader.setUniform("inverseUIMatrix", Data::instance.virtualDisplayTransform.inverse());

                renderShader.setUniform("spectatorPosWS", Data::instance.virtualDisplaySpectator);
                renderShader.setUniform("projectOnlyProjectorIDColor", Data::instance.projectOnlyProjectorIDColor);

                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_mlsVertices[cameraID]);
                renderShader.setUniform("texture2D_vertices", 3);

                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_edgeProximity[cameraID]);
                renderShader.setUniform("texture2D_edgeProximity", 4);

                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_normals[cameraID]);
                renderShader.setUniform("texture2D_normals", 5);

                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_qualityEstimate[cameraID]);
                renderShader.setUniform("texture2D_qualityEstimate", 6);

                unsigned int* vertexProjectorAssignent = CameraPasses::getInstance().texture2D_vertexProjectorAssignment;

                if(vertexProjectorAssignent != nullptr){
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_2D, vertexProjectorAssignent[cameraID]);
                    renderShader.setUniform("texture2D_vertexProjectorAssignment", 7);
                }

                renderShader.setUniform("stride", Data::instance.rectificationMeshStride);
            } else {
                customRenderShader->bind();
                customRenderShader->setUniform("model", pctextures.currentPointClouds[cameraID]->modelMatrix);
                customRenderShader->setUniform("view", sceneData.view);
                customRenderShader->setUniform("projection", sceneData.projection);

                customRenderShader->setUniform("projectorID", projectorID);

                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_mlsVertices[cameraID]);
                customRenderShader->setUniform("texture2D_vertices", 3);

                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_edgeProximity[cameraID]);
                customRenderShader->setUniform("texture2D_edgeProximity", 4);

                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_normals[cameraID]);
                customRenderShader->setUniform("texture2D_normals", 5);

                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_qualityEstimate[cameraID]);
                customRenderShader->setUniform("texture2D_qualityEstimate", 6);

                customRenderShader->setUniform("stride", Data::instance.rectificationMeshStride);
                // Bind further textures:
                if(customShaderBindingsCallback){
                    customShaderBindingsCallback(customRenderShader, cameraID, 7);
                }
            }

            glBindVertexArray(pctextures.dummyVAO);
            int gridW = pctextures.cameraWidth[cameraID];
            int gridH = pctextures.cameraHeight[cameraID];
            int cellsX = (gridW - 1) / Data::instance.rectificationMeshStride;
            int cellsY = (gridH - 1) / Data::instance.rectificationMeshStride;
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, cellsX * cellsY);
            glBindVertexArray(0);

            glDrawBuffers(1, attachments);
        }

        glDisable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        int originalViewport[4];
        glGetIntegerv(GL_VIEWPORT, originalViewport);

        // MiniScreen:
        {
            glViewport(0, 0, fbo_mini_screen_width, fbo_mini_screen_height);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_majorCam);
            majorCamShader.bind();

            unsigned int currentTexture = 1;
            for(unsigned int cameraIDOfTexture : pctextures.usedCameraIDs){
                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenColor[cameraIDOfTexture]);
                majorCamShader.setUniform("color["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenVertices[cameraIDOfTexture]);
                majorCamShader.setUniform("vertices["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenNormals[cameraIDOfTexture]);
                majorCamShader.setUniform("normals["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenDepth[cameraIDOfTexture]);
                majorCamShader.setUniform("depth["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;
            }

            for(int i=0; i < CAMERA_COUNT; ++i){
                majorCamShader.setUniform("isCameraActive["+std::to_string(i)+"]", isCameraActive[i]);
            }

            majorCamShader.setUniform("view", sceneData.view);
            majorCamShader.setUniform("cameraVector", sceneData.view.inverse() * Vec4f(0.0, 0.0, 1.0, 0.0));

            glBindVertexArray(pctextures.VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        {
            glViewport(0, 0, fbo_mini_screen_width, fbo_mini_screen_height);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_cameraWeights);
            cameraWeightsShader.bind();

            unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
            glDrawBuffers(2, attachments);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture2D_majorCam);
            cameraWeightsShader.setUniform("dominanceTexture", 1);

            for(int i=0; i < CAMERA_COUNT; ++i){
                cameraWeightsShader.setUniform("isCameraActive["+std::to_string(i)+"]", isCameraActive[i]);
            }

            glBindVertexArray(pctextures.VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }


        glDisable(GL_CULL_FACE);
        glCullFace(GL_FRONT);

        // Screen Merging:

        Shader& usedBlendingShader = customRenderShader == nullptr ? vertexBlendingShader : blendingShader;
        {
            glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);
            glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);

            unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0};
            glDrawBuffers(1, attachments);

            usedBlendingShader.bind();

            unsigned int currentTexture = 1;
            for(unsigned int cameraIDOfTexture : pctextures.usedCameraIDs){
                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenColor[cameraIDOfTexture]);
                usedBlendingShader.setUniform("color["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenVertices[cameraIDOfTexture]);
                usedBlendingShader.setUniform("vertices["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenNormals[cameraIDOfTexture]);
                usedBlendingShader.setUniform("normals["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenDepth[cameraIDOfTexture]);
                usedBlendingShader.setUniform("depth["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;
            }

            glActiveTexture(GL_TEXTURE0 + currentTexture);
            glBindTexture(GL_TEXTURE_2D, texture2D_cameraWeightsA);
            usedBlendingShader.setUniform("miniWeightsA", int(currentTexture));
            ++currentTexture;

            for(int i=0; i < CAMERA_COUNT; ++i){
                usedBlendingShader.setUniform("isCameraActive["+std::to_string(i)+"]", isCameraActive[i]);
            }


            if(customRenderShader == nullptr){
                usedBlendingShader.setUniform("inverseView", sceneData.view.inverse());
                usedBlendingShader.setUniform("inverseUIMatrix", Data::instance.virtualDisplayTransform.inverse());

                if(uiRenderer != nullptr){
                    uiRenderer->bindTexture(currentTexture);
                	usedBlendingShader.setUniform("texture2D_ui", int(currentTexture));
                	++currentTexture;
				}

                usedBlendingShader.setUniform("spectatorPosWS", Data::instance.virtualDisplaySpectator);
                usedBlendingShader.setUniform("projectorID", projectorID);
                usedBlendingShader.setUniform("projectOnlyProjectorIDColor", Data::instance.projectOnlyProjectorIDColor);
                usedBlendingShader.setUniform("tileBasedShadowAvoidance", Data::instance.tileBasedShadowAvoidance);
                usedBlendingShader.setUniform("ignoreShadowAvoidance", Data::instance.ignoreShadowAvoidance);

                if(uiRenderer != nullptr){
                    uiRenderer->bindShadowTileTexture(currentTexture);
                    usedBlendingShader.setUniform("texture2D_segmentID", int(currentTexture));
                    ++currentTexture;
                }

                int i=0;

                if(uiRenderer != nullptr){
                    for(ShadowTile& tile : uiRenderer->getShadowTiles()){
                        usedBlendingShader.setUniform("shadowTiles["+std::to_string(i)+"].projectorWeight[0]", tile.projectorWeight[0]);
                        usedBlendingShader.setUniform("shadowTiles["+std::to_string(i)+"].projectorWeight[1]", tile.projectorWeight[1]);
                        usedBlendingShader.setUniform("shadowTiles["+std::to_string(i)+"].projectorWeight[2]", tile.projectorWeight[2]);
                        usedBlendingShader.setUniform("shadowTiles["+std::to_string(i)+"].useFallback", tile.useFallback);
                        if(tile.majorProjector >= 0 && tile.majorProjector < 3){ // TODO: DYNAMIC SIZE CHECK!
                            usedBlendingShader.setUniform("shadowTiles["+std::to_string(i)+"].minDistance", tile.minDistance[tile.majorProjector]);
                        }
                        ++i;
                    }
                    usedBlendingShader.setUniform("shadowTileCount", int(uiRenderer->getShadowTiles().size()));
                }
            }


            glBindVertexArray(pctextures.VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glActiveTexture(GL_TEXTURE0);
    }
};
