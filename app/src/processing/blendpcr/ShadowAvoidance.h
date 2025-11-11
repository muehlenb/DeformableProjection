// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include <glad/glad.h>

#include "src/processing/blendpcr/Rectification.h"

#include "src/processing/uirenderer/ShadowTile.h"

#define SEGMENTATION_DOWNSCALE_WIDTH 256
#define SEGMENTATION_DOWNSCALE_HEIGHT 256

#define PROJECTOR_COUNT 3

class ShadowAvoidance {
public:
    /** BlendPCR instance for rendering the first pass */
    std::shared_ptr<Rectification> blendPCRForFirstPass = nullptr;
    std::shared_ptr<Shader> blendPCRFirstPassRenderShader = nullptr;

    // Projector distances (1. Pass):
    unsigned int fbo_projectorDistance[PROJECTOR_COUNT];
    unsigned int texture2D_projectorDistanceMap[PROJECTOR_COUNT];

    std::shared_ptr<UIRenderer> uiRenderer;

    bool isInitialized = false;

    int fbo_screen_width = -1;
    int fbo_screen_height = -1;

    /** Creates a shadow map where the 4D-vector stores a float value for each projector whether it can be reached by it */
    Shader vertexShadowMapShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/shadowMap.vert", CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/shadowMap.frag");

    /** Jump Flooding shader*/
    Shader jumpFloodingShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/jumpFlooding.vert", CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/jumpFlooding.frag");

    /** Creates a distance map from the jump flooding shader */
    Shader vertexDistanceMapShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/distanceMap.vert", CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/distanceMap.frag");

    /** Assigns each vertex a projector using the distance map and other attributes */
    Shader globalDistanceMapShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/globalDistanceMap.vert", CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/globalDistanceMap.frag");

    /** Temporally interpolates the distance map so that points wh */
    Shader temporalDistanceMapShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/temporalDistanceMap.vert", CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/temporalDistanceMap.frag");

    /** Segmentation Downscale shader */
    Shader segmentationDownscaleShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/segmentationDownscale.vert", CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/segmentationDownscale.frag");

    /** Assigns each vertex a projector using the distance map and other attributes */
    Shader vertexProjectorAssignmentShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/projectorAssignment.vert", CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/pointcloud/projectorAssignment.frag");

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

    std::shared_ptr<UIRenderer> dummyUIRenderer = nullptr;

public:
    Vec4f estimatedMarkerPositions[CAMERA_COUNT * 4];

    /** I2DShadowAvoidance */
    ShadowAvoidance(std::shared_ptr<UIRenderer> uiRenderer)
        : uiRenderer(uiRenderer){
        /**
         * Instanciate blendpcr for the first pass where the object to projector distance
         * is calculated.
         */
        blendPCRFirstPassRenderShader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/screen/separateRendering1Pass.vert", CMAKE_SOURCE_DIR "/shader/blendpcr_occlusionremoval/screen/separateRendering1Pass.frag");
        blendPCRForFirstPass = std::make_shared<Rectification>(dummyUIRenderer, blendPCRFirstPassRenderShader);
    }

    void init(){
        int mainViewport[4];
        glGetIntegerv(GL_VIEWPORT, mainViewport);

        int originalFramebuffer;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &originalFramebuffer);

        int screenSizeX = 1280 / 2;
        int screenSizeY = 720 / 2;

        // If screen size changed:
        if(screenSizeX != fbo_screen_width || screenSizeY != fbo_screen_height){
            for(unsigned int projectorID = 0; projectorID < PROJECTOR_COUNT; ++projectorID){
                // Delete old frame buffer + texture:
                if(fbo_screen_width != -1){
                    glDeleteFramebuffers(1, &fbo_projectorDistance[projectorID]);
                    glDeleteTextures(1, &texture2D_projectorDistanceMap[projectorID]);
                }

                // Generate resources for SCREEN SPACE PASS:
                {
                    glGenFramebuffers(1, &fbo_projectorDistance[projectorID]);
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_projectorDistance[projectorID]);

                    generateAndBind2DTexture(texture2D_projectorDistanceMap[projectorID], screenSizeX, screenSizeY, GL_RED, GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_projectorDistanceMap[projectorID], 0);
                }
            }

            fbo_screen_width = screenSizeX;
            fbo_screen_height = screenSizeY;
        }

        if(!isInitialized){
            isInitialized = true;
        }


        glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);
    }

    /**
     * Updates the shadow maps and projector assignments during the render pass
     */
    virtual void glTick();
};
