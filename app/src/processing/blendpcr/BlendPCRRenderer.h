// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

// Include OpenGL3.3 Core functions:
#include <glad/glad.h>
#include "src/gl/Shader.h"

#include "src/processing/blendpcr/CameraPasses.h"
#include "src/Data.h"

using namespace std::chrono;

// Uncomment if you want to print timings:
// #define PRINT_TIMINGS

#define CAMERA_COUNT 7
#define LOOKUP_IMAGE_SIZE 1024

class BlendPCRRenderer {
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

    /**
     * Define all the shaders for the screen passes:
     */
    Shader renderShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/separateRendering.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/separateRendering.frag");
    Shader majorCamShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/majorCam.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/majorCam.frag");
    Shader cameraWeightsShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/cameraWeights.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/cameraWeights.frag");
    Shader blendingShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/screen/blending.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/screen/blending.frag");


    void generateAndBind2DTexture(
        unsigned int& texture,
        unsigned int width,
        unsigned int height,
        unsigned int internalFormat, // eg. GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_R32F,...
        unsigned int format, // e.g. only GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA without Sizes!
        unsigned int type, // egl. GL_UNSIGNED_BYTE, GL_FLOAT
        unsigned int filter // GL_LINEAR or GL_NEAREST
        ){
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);

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

    BlendPCRRenderer(){
    }

    ~BlendPCRRenderer(){
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

    /**
     * Renders the point cloud
     */
    void render(Mat4f projection, Mat4f view) {
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

            renderShader.bind();
            renderShader.setUniform("model", pctextures.currentPointClouds[cameraID]->modelMatrix);
            renderShader.setUniform("view", view);
            renderShader.setUniform("projection", projection);
            renderShader.setUniform("useColorIndices", useColorIndices);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, pctextures.useReimplementedFilters ? pctextures.texture2D_pcf_holeFilledRGB[cameraID] : pctextures.texture2D_inputRGB[cameraID]);
            renderShader.setUniform("texture2D_colors", 2);

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

            /*
            glBindVertexArray(pctextures.VAO);
            glDrawElements(GL_TRIANGLES, pctextures.indicesSize,  GL_UNSIGNED_INT, NULL);
            glBindVertexArray(0);
            */

            glBindVertexArray(pctextures.dummyVAO);
            int gridW = pctextures.cameraWidth[cameraID];
            int gridH = pctextures.cameraHeight[cameraID];
            int cellsX = (gridW - 1) / Data::instance.rectificationMeshStride;
            int cellsY = (gridH - 1) / Data::instance.rectificationMeshStride;
            renderShader.setUniform("stride", Data::instance.rectificationMeshStride);

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

            majorCamShader.setUniform("view", view);
            majorCamShader.setUniform("cameraVector", view.inverse() * Vec4f(0.0, 0.0, 1.0, 0.0));

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
        {
            glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);
            glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);

            if(originalFramebuffer != 0){
                unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0};
                glDrawBuffers(1, attachments);
            }

            blendingShader.bind();

            unsigned int currentTexture = 1;
            for(unsigned int cameraIDOfTexture : pctextures.usedCameraIDs){
                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenColor[cameraIDOfTexture]);
                blendingShader.setUniform("color["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenVertices[cameraIDOfTexture]);
                blendingShader.setUniform("vertices["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenNormals[cameraIDOfTexture]);
                blendingShader.setUniform("normals["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;

                glActiveTexture(GL_TEXTURE0 + currentTexture);
                glBindTexture(GL_TEXTURE_2D, texture2D_screenDepth[cameraIDOfTexture]);
                blendingShader.setUniform("depth["+std::to_string(cameraIDOfTexture)+"]", int(currentTexture));
                ++currentTexture;
            }

            glActiveTexture(GL_TEXTURE0 + currentTexture);
            glBindTexture(GL_TEXTURE_2D, texture2D_cameraWeightsA);
            blendingShader.setUniform("miniWeightsA", int(currentTexture));
            ++currentTexture;

            glActiveTexture(GL_TEXTURE0 + currentTexture);
            glBindTexture(GL_TEXTURE_2D, texture2D_cameraWeightsB);
            blendingShader.setUniform("miniWeightsB", int(currentTexture));
            ++currentTexture;

            for(int i=0; i < CAMERA_COUNT; ++i){
                blendingShader.setUniform("isCameraActive["+std::to_string(i)+"]", isCameraActive[i]);
            }

            blendingShader.setUniform("view", view);

            // Lighting
            Vec4f lightPos[4] = {Vec4f(1.5f, 1.5f, 1.5f), Vec4f(-1.5f, 1.5f, 1.5f), Vec4f(-1.5f, 1.5f, -1.5f), Vec4f(1.5f, 1.5f, -1.5f)};
            Vec4f lightColors[4] = {Vec4f(0.5f, 0.75f, 1.0f), Vec4f(1.0f, 0.75f, 0.5f), Vec4f(0.5f, 0.75f, 1.0f), Vec4f(1.0f, 0.75f, 0.5f)};

            for(int i=0; i < 4; ++i){
                blendingShader.setUniform("lights["+std::to_string(i)+"].position", view * lightPos[i]);
                blendingShader.setUniform("lights["+std::to_string(i)+"].intensity", lightColors[i]);
                blendingShader.setUniform("lights["+std::to_string(i)+"].range", 10.f);
            }

            blendingShader.setUniform("cameraVector", view.inverse() * Vec4f(0.0, 0.0, 1.0, 0.0));

            glBindVertexArray(pctextures.VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glActiveTexture(GL_TEXTURE0);
    }
};
