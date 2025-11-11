// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include "src/gl/TextureFBO.h"
#include "src/gl/Texture2D.h"
#include "src/gl/Shader.h"

#include "src/Data.h"

#include "src/processing/OrganizedPointCloud.h"
#include "src/Semaphore.h"

// Include OpenGL3.3 Core functions:
#include <glad/glad.h>

using namespace std::chrono;

#define CAMERA_COUNT 3
#define LOOKUP_IMAGE_SIZE 1024

#define SEGMENTATION_DOWNSCALE_WIDTH 256
#define SEGMENTATION_DOWNSCALE_HEIGHT 256

/**
 * This is the first part of the BlendPCR rendering method. For performance reasons in dual use
 * scenarios, it is separated from the BlendPCR class in the DeformableProjection project.
 *
 * BlendPCR is also used by the projectors to reconstruct the mesh, but the first pass is the
 * same, no matter which projector or screen is used.
 */
class CameraPasses {
public:
    static CameraPasses& getInstance(){
        if(globalCameraPasses == nullptr){
            globalCameraPasses = new CameraPasses();
        }

        return *globalCameraPasses;
    }

    CameraPasses(){
        // Ensure that camera are correctly recognizable as uninitialized:
        for(int i=0; i < CAMERA_COUNT; ++i){
            cameraWidth[i] = -1;
            cameraHeight[i] = -1;
            lookupInitialized[i] = false;
        }
    }

    // Current Point Clouds:
    std::mutex pointCloudsMutex;
    std::vector<std::shared_ptr<OrganizedPointCloud>> currentPointClouds;

    Mat4f pointCloudMatrix[CAMERA_COUNT];

    int cameraWidth[CAMERA_COUNT];
    int cameraHeight[CAMERA_COUNT];
    bool lookupInitialized[CAMERA_COUNT];
    bool cameraIsUpdatedThisFrame[CAMERA_COUNT];

    // Reimplemented point cloud filter (Hole Filling):
    unsigned int fbo_pcf_holeFilling[CAMERA_COUNT];
    unsigned int texture2D_pcf_holeFilledVertices[CAMERA_COUNT];
    unsigned int texture2D_pcf_holeFilledRGB[CAMERA_COUNT];

    unsigned int texture2D_pcf_temporalNoiseFilter[CAMERA_COUNT];

    // Reimplemented point cloud filter (Erosion):
    unsigned int fbo_pcf_erosion[CAMERA_COUNT];
    unsigned int texture2D_pcf_erosion[CAMERA_COUNT];

    // Using Flip Flop approach to insert old texture for
    // noise removal:
    unsigned int fbo_pcf_temporalFilterA[CAMERA_COUNT];
    unsigned int texture2D_pcf_temporalFilterA[CAMERA_COUNT];
    unsigned int fbo_pcf_temporalFilterB[CAMERA_COUNT];
    unsigned int texture2D_pcf_temporalFilterB[CAMERA_COUNT];
    bool temporalFilterFlipFlop[CAMERA_COUNT];

    // NOT A CREATED RESOURCE, ONLY FOR PASSING THROUGH THE CORRECT TEXTURES
    // TO BE ABLE TO DYNAMICALLY ACTIVATE AND DEACTIVATE FILTERS:
    unsigned int currentProcessedVertices[CAMERA_COUNT];

    // FBO for generating 3D vertices from depth image:
    unsigned int fbo_genVertices[CAMERA_COUNT];

    // The textures for the input point clouds:
    unsigned int texture2D_inputGenVertices[CAMERA_COUNT];
    unsigned int texture2D_inputDepth[CAMERA_COUNT];
    unsigned int texture2D_inputRGB[CAMERA_COUNT];
    unsigned int texture2D_inputLookupImageTo3D[CAMERA_COUNT];
    unsigned int texture2D_inputLookup3DToImage[CAMERA_COUNT];

    // The fbo and texture for the rejection pass:
    unsigned int fbo_rejection[CAMERA_COUNT];
    unsigned int texture2D_rejection[CAMERA_COUNT];

    // The fbo and texture for the edge proximity pass:
    unsigned int fbo_edgeProximity[CAMERA_COUNT];
    unsigned int texture2D_edgeProximity[CAMERA_COUNT];

    // The fbo and texture for the mls pass:
    unsigned int fbo_mls[CAMERA_COUNT];
    unsigned int texture2D_mlsVertices[CAMERA_COUNT];

    // The fbo and texture for the normal estimation pass:
    unsigned int fbo_normals[CAMERA_COUNT];
    unsigned int texture2D_normals[CAMERA_COUNT];

    // The fbo and texture for the quality estimation pass:
    unsigned int fbo_qualityEstimate[CAMERA_COUNT];
    unsigned int texture2D_qualityEstimate[CAMERA_COUNT];

    // Vertex shadow map (2. Pass):
    unsigned int fbo_vertexShadowMap[CAMERA_COUNT];
    unsigned int texture2D_vertexShadowMap[CAMERA_COUNT];

    // Ping Pong maps using the Jump Flooding algorithm:
    unsigned int fbo_jumpFloodingPing;
    unsigned int texture2D_jumpFloodingPing;

    unsigned int fbo_jumpFloodingPong;
    unsigned int texture2D_jumpFloodingPong;

    // Vertex distance map (3. Pass):
    unsigned int fbo_vertexDistanceMap[CAMERA_COUNT];
    unsigned int texture2D_vertexDistanceMap[CAMERA_COUNT];

    // Global distance map (4. Pass):
    unsigned int fbo_globalDistanceMap[CAMERA_COUNT];
    unsigned int texture2D_globalDistanceMap[CAMERA_COUNT];

    // Temporal distance map (5. Pass), flip flop approach:
    unsigned int fbo_temporalDistanceMapA[CAMERA_COUNT];
    unsigned int texture2D_temporalDistanceMapA[CAMERA_COUNT];

    unsigned int fbo_temporalDistanceMapB[CAMERA_COUNT];
    unsigned int texture2D_temporalDistanceMapB[CAMERA_COUNT];

    bool temporalDistanceFlipFlop = false;

    // Segmentation Downscaled pass:
    unsigned int fbo_segmentationDownscale[CAMERA_COUNT];
    unsigned int texture2D_segmentationID[CAMERA_COUNT];
    unsigned int texture2D_segmentationShadowDistance[CAMERA_COUNT];

    // Vertex Projector Assignment (6. Pass):
    unsigned int fbo_vertexProjectorAssignment[CAMERA_COUNT];
    unsigned int texture2D_vertexProjectorAssignment[CAMERA_COUNT];

    /**
     * Defines the mesh
     */
    unsigned int indicesSize;
    unsigned int* indices;

    float* gridData;

    unsigned int dummyVAO;

    /**
     * Defines the quad which is used for rendering in every pass.
     */

    unsigned int VBO_quad;
    unsigned int VAO_quad;
    float* quadData = new float[12]{1.f,-1.f, -1.f,-1.f, -1.f,1.f, -1.f,1.f, 1.f,1.f, 1.f,-1.f};

    std::vector<unsigned int> usedCameraIDs;

    bool useReimplementedFilters = true;
    bool shouldClip = false;

    Vec4f clipMin = Vec4f(-1.0f, 0.05f, -1.0, 0.0);
    Vec4f clipMax = Vec4f(1.0f, 2.0f, 1.0, 0.0);


private:
    // Global PCTextureProcessor:
    static CameraPasses* globalCameraPasses;

    // Semaphore which checks if a switch of the current PBO is
    // allowed (this is the case as long as PBOs are not getting
    // copied into the textures):
    Semaphore switchCurrentPBOSemaphore = Semaphore(1);

    float implicitH = 0.01f;
    float kernelRadius = 10.f;
    float kernelSpread = 1.f;

    float uploadTime = 0;

    /**
     * Define all the shaders for the reimplemented point cloud filters
     * (originally CUDA implemented):
     */
    Shader pcfHoleFillingShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/filter/holeFilling.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/filter/holeFilling.frag");
    Shader pcfErosionShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/filter/erosion.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/filter/erosion.frag");
    Shader noiseRemovalShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/filter/noiseRemoval.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/filter/noiseRemoval.frag");

    /** Generates 3D vertices (in m) from depth image (in mm) */
    Shader vertexGenShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/vertexGenerator.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/vertexGenerator.frag");

    /**
     * Define all the shaders for the point cloud processing passes:
     */
    Shader rejectionShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/rejection.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/rejection.frag");
    Shader edgeProximityShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/edgeProximity.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/edgeProximity.frag");
    Shader mlsShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/mls.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/mls.frag");
    Shader normalsShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/normals.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/normals.frag");
    Shader qualityEstimateShader = Shader(CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/qualityEstimate.vert", CMAKE_SOURCE_DIR "/shader/blendpcr/pointcloud/qualityEstimate.frag");

    bool isInitialized = false;

    void initQuadBuffer(){
        glGenVertexArrays(1, &VAO_quad);
        glBindVertexArray(VAO_quad);

        glGenBuffers(1, &VBO_quad);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_quad);
        glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(GLfloat), quadData, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

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


    void initMesh(){
        glGenVertexArrays(1, &dummyVAO);
        glBindVertexArray(dummyVAO);
    }

    inline void checkFramebufferComplete(const char* label) {
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer incomplete at " << label
                      << ": 0x" << std::hex << status << std::dec << std::endl;
        }
    }

    /**
     * Initializes the memory of a camera if necessary (meant to be called in every frame where the camera is used).
     */
    void ensureCameraInitialized(int deviceIndex, unsigned int imageWidth, unsigned int imageHeight){

        // If already correctly initialized, just return:
        if(cameraWidth[deviceIndex] == imageWidth && cameraHeight[deviceIndex] == imageHeight)
            return;

        // If camera is initialized (but with wrong resolution), delete first:
        if(cameraWidth[deviceIndex] > 0 || cameraHeight[deviceIndex] > 0)
            deinitializeCamera(deviceIndex);

        // Now initialize again:
        std::cout << "Initialize Camera " << deviceIndex << " at " << imageWidth << " x " << imageHeight << std::endl;

        // Generate resources for INPUT
        {
            // Input point cloud texture
            generateAndBind2DTexture(texture2D_inputDepth[deviceIndex], imageWidth, imageHeight, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT, GL_NEAREST);

            // Input color texture
            generateAndBind2DTexture(texture2D_inputRGB[deviceIndex], imageWidth, imageHeight, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR);

            // Input lookup table
            generateAndBind2DTexture(texture2D_inputLookupImageTo3D[deviceIndex], imageWidth, imageHeight, GL_RG32F, GL_RG, GL_FLOAT, GL_LINEAR);
            generateAndBind2DTexture(texture2D_inputLookup3DToImage[deviceIndex], LOOKUP_IMAGE_SIZE, LOOKUP_IMAGE_SIZE, GL_RG32F, GL_RG, GL_FLOAT, GL_LINEAR);
        }

        /**
             * Generate resources for reimplemented EROSION FILTER (orig. CUDA implemented).
             */
        {
            glGenFramebuffers(1, &fbo_pcf_erosion[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_pcf_erosion[deviceIndex]);

            generateAndBind2DTexture(texture2D_pcf_erosion[deviceIndex], imageWidth, imageHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_pcf_erosion[deviceIndex], 0);
        }

        /**
             * Generate resources for TEMPORAL NOISE FILTER.
             */
        {
            glGenFramebuffers(1, &fbo_pcf_temporalFilterA[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_pcf_temporalFilterA[deviceIndex]);

            generateAndBind2DTexture(texture2D_pcf_temporalFilterA[deviceIndex], imageWidth, imageHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_pcf_temporalFilterA[deviceIndex], 0);

            glGenFramebuffers(1, &fbo_pcf_temporalFilterB[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_pcf_temporalFilterB[deviceIndex]);

            generateAndBind2DTexture(texture2D_pcf_temporalFilterB[deviceIndex], imageWidth, imageHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_pcf_temporalFilterB[deviceIndex], 0);
        }

        /**
             * Generate resources for vertex generation fbo:
             */
        {
            glGenFramebuffers(1, &fbo_genVertices[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_genVertices[deviceIndex]);

            generateAndBind2DTexture(texture2D_inputGenVertices[deviceIndex], imageWidth, imageHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_inputGenVertices[deviceIndex], 0);
        }

        /**
         * Generate resources for reimplemented HOLE FILLING FILTER (orig. CUDA implemented).
         */
        {
            glGenFramebuffers(1, &fbo_pcf_holeFilling[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_pcf_holeFilling[deviceIndex]);

            generateAndBind2DTexture(texture2D_pcf_holeFilledVertices[deviceIndex], imageWidth, imageHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_pcf_holeFilledVertices[deviceIndex], 0);

            generateAndBind2DTexture(texture2D_pcf_holeFilledRGB[deviceIndex], imageWidth, imageHeight, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture2D_pcf_holeFilledRGB[deviceIndex], 0);

            // Wichtig: mehrere Color Attachments aktivieren
            GLenum hfAttachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, hfAttachments);

            checkFramebufferComplete("HoleFilling");
        }

        /**
             * Generate resources for REJECTION PASS.
             *
             * This pass calculates whether a vertex is valid or not
             * (and considers the distance to neighbouring vertices).
             *
             * A red-value of 0 means valid, 1 means invalid.
             */
        {
            glGenFramebuffers(1, &fbo_rejection[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_rejection[deviceIndex]);

            generateAndBind2DTexture(texture2D_rejection[deviceIndex], imageWidth, imageHeight, GL_RED, GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_rejection[deviceIndex], 0);
        }

        /**
             * Generate resources for EDGE PROXIMITY PASS.
             *
             * This pass calculates the proximity to invalid pixels.
             *
             * A red-value of 0 means far away from edge, 1 means on edge.
             *
             * The kernelRadius (uniform var) defines the search radius.
             * Vertices which are more than [kernelRadius] units away from
             * invalid pixels get the value 0.
             */
        {
            glGenFramebuffers(1, &fbo_edgeProximity[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_edgeProximity[deviceIndex]);

            generateAndBind2DTexture(texture2D_edgeProximity[deviceIndex], imageWidth, imageHeight, GL_RED, GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_edgeProximity[deviceIndex], 0);
        }

        /**
             * Generate resources for QUALITY ESTIMATE PASS.
             *
             * This pass calcluates the estimated quality for each pixel of
             * this camera. The first output value is the quality estimate,
             * the second output is the edge proximity.
             */
        {
            glGenFramebuffers(1, &fbo_qualityEstimate[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_qualityEstimate[deviceIndex]);

            generateAndBind2DTexture(texture2D_qualityEstimate[deviceIndex], imageWidth, imageHeight, GL_RG32F, GL_RG, GL_FLOAT, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_qualityEstimate[deviceIndex], 0);
        }

        /**
             * MLS PASS: Generate frame buffer and render texture.
             *
             * This pass smoothes the vertices with a weighted moving least
             * squares kernel while being weighted with the edgeProximity
             * (to smooth the edges).
             */
        {
            glGenFramebuffers(1, &fbo_mls[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_mls[deviceIndex]);

            generateAndBind2DTexture(texture2D_mlsVertices[deviceIndex], imageWidth, imageHeight, GL_RGB32F, GL_RGB, GL_FLOAT, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_mlsVertices[deviceIndex], 0);
        }

        /**
             * NORMAL ESTIMATION PASS: Generate frame buffer and render texture.
             *
             * Calculates normals for the vertices using cholesky, eigenvalues,
             * and so on.
             */
        {
            glGenFramebuffers(1, &fbo_normals[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_normals[deviceIndex]);

            generateAndBind2DTexture(texture2D_normals[deviceIndex], imageWidth, imageHeight, GL_RGB32F, GL_RGB, GL_FLOAT, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_normals[deviceIndex], 0);
        }


        /*
         * SHADOW AVOIDANCE MAPS
         */
        {
            // Shadow Maps for Vertices (each vertex stores, whether each projector 1, 2 and 3 is visible):
            glGenFramebuffers(1, &fbo_vertexShadowMap[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_vertexShadowMap[deviceIndex]);

            generateAndBind2DTexture(texture2D_vertexShadowMap[deviceIndex], imageWidth, imageHeight, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_vertexShadowMap[deviceIndex], 0);

            checkFramebufferComplete("VertexShadowMap");
            // Vertex distance map to next shadow.
            glGenFramebuffers(1, &fbo_vertexDistanceMap[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_vertexDistanceMap[deviceIndex]);

            generateAndBind2DTexture(texture2D_vertexDistanceMap[deviceIndex], imageWidth, imageHeight, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_vertexDistanceMap[deviceIndex], 0);

            // Vertex distance map to next shadow.
            glGenFramebuffers(1, &fbo_globalDistanceMap[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_globalDistanceMap[deviceIndex]);

            generateAndBind2DTexture(texture2D_globalDistanceMap[deviceIndex], imageWidth, imageHeight, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_globalDistanceMap[deviceIndex], 0);

            // Temporal distance map A:
            glGenFramebuffers(1, &fbo_temporalDistanceMapA[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_temporalDistanceMapA[deviceIndex]);

            generateAndBind2DTexture(texture2D_temporalDistanceMapA[deviceIndex], imageWidth, imageHeight, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_temporalDistanceMapA[deviceIndex], 0);

            // Temporal distance map B:
            glGenFramebuffers(1, &fbo_temporalDistanceMapB[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_temporalDistanceMapB[deviceIndex]);

            generateAndBind2DTexture(texture2D_temporalDistanceMapB[deviceIndex], imageWidth, imageHeight, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_temporalDistanceMapB[deviceIndex], 0);

            // Generate segmentation downscaled fbos:
            glGenFramebuffers(1, &fbo_segmentationDownscale[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_segmentationDownscale[deviceIndex]);

            // ID-Textur (Integer, im Shader als usampler2D benutzen!)
            generateAndBind2DTexture(texture2D_segmentationID[deviceIndex], SEGMENTATION_DOWNSCALE_WIDTH, SEGMENTATION_DOWNSCALE_HEIGHT, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_segmentationID[deviceIndex], 0);

            // Shadow Distance Textur
            generateAndBind2DTexture(texture2D_segmentationShadowDistance[deviceIndex], SEGMENTATION_DOWNSCALE_WIDTH, SEGMENTATION_DOWNSCALE_HEIGHT, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture2D_segmentationShadowDistance[deviceIndex], 0);

            // Mehrere Attachments aktivieren
            GLenum segAttachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, segAttachments);

            checkFramebufferComplete("SegmentationDownscale");


            // An assignment of a projector to each vertex:
            glGenFramebuffers(1, &fbo_vertexProjectorAssignment[deviceIndex]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_vertexProjectorAssignment[deviceIndex]);

            generateAndBind2DTexture(texture2D_vertexProjectorAssignment[deviceIndex], imageWidth, imageHeight, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_vertexProjectorAssignment[deviceIndex], 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        cameraWidth[deviceIndex] = imageWidth;
        cameraHeight[deviceIndex] = imageHeight;
    }

    void deinitializeCamera(int deviceIndex){
        std::cout << "Deinitialize Camera " << deviceIndex << std::endl;

        cameraWidth[deviceIndex] = -1;
        cameraHeight[deviceIndex] = -1;

        glDeleteFramebuffers(1, &fbo_pcf_holeFilling[deviceIndex]);
        glDeleteTextures(1, &texture2D_pcf_holeFilledVertices[deviceIndex]);
        glDeleteTextures(1, &texture2D_pcf_holeFilledRGB[deviceIndex]);

        glDeleteFramebuffers(1, &fbo_pcf_erosion[deviceIndex]);
        glDeleteTextures(1, &texture2D_pcf_erosion[deviceIndex]);

        glDeleteFramebuffers(1, &fbo_pcf_temporalFilterA[deviceIndex]);
        glDeleteTextures(1, &texture2D_pcf_temporalFilterA[deviceIndex]);
        glDeleteFramebuffers(1, &fbo_pcf_temporalFilterB[deviceIndex]);
        glDeleteTextures(1, &texture2D_pcf_temporalFilterB[deviceIndex]);

        glDeleteTextures(1, &texture2D_inputGenVertices[deviceIndex]);
        glDeleteTextures(1, &texture2D_inputDepth[deviceIndex]);
        glDeleteTextures(1, &texture2D_inputRGB[deviceIndex]);
        glDeleteTextures(1, &texture2D_inputLookupImageTo3D[deviceIndex]);

        glDeleteFramebuffers(1, &fbo_rejection[deviceIndex]);
        glDeleteTextures(1, &texture2D_rejection[deviceIndex]);

        glDeleteFramebuffers(1, &fbo_edgeProximity[deviceIndex]);
        glDeleteTextures(1, &texture2D_edgeProximity[deviceIndex]);

        glDeleteFramebuffers(1, &fbo_mls[deviceIndex]);
        glDeleteTextures(1, &texture2D_mlsVertices[deviceIndex]);

        glDeleteFramebuffers(1, &fbo_normals[deviceIndex]);
        glDeleteTextures(1, &texture2D_normals[deviceIndex]);

        glDeleteFramebuffers(1, &fbo_qualityEstimate[deviceIndex]);
        glDeleteTextures(1, &texture2D_qualityEstimate[deviceIndex]);
    }

    void init(){
        // If already initialized, don't to it again and simply return:
        if(isInitialized)
            return;

        // Init the quadbuffer:
        initQuadBuffer();

        // Init mesh (grid):
        initMesh();

        glGenFramebuffers(1, &fbo_jumpFloodingPing);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_jumpFloodingPing);
        generateAndBind2DTexture(texture2D_jumpFloodingPing, 512, 512, GL_RGB32UI, GL_RGB_INTEGER, GL_UNSIGNED_INT, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_jumpFloodingPing, 0);

        // Jump Flooding Pong:
        glGenFramebuffers(1, &fbo_jumpFloodingPong);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_jumpFloodingPong);
        generateAndBind2DTexture(texture2D_jumpFloodingPong, 512, 512, GL_RGB32UI, GL_RGB_INTEGER, GL_UNSIGNED_INT, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D_jumpFloodingPong, 0);


        isInitialized = true;
    }

public:
    ~CameraPasses(){
        if(!isInitialized)
            return;

        for(int cameraID = 0; cameraID < CAMERA_COUNT; ++cameraID){
            if(cameraWidth[cameraID] > 0){
                deinitializeCamera(cameraID);
            }
        }

        delete[] indices;

        delete[] gridData;

        glDeleteVertexArrays(1, &dummyVAO);

        glDeleteVertexArrays(1, &VAO_quad);
        glDeleteBuffers(1, &VBO_quad);
    }

    /**
     * Integrate new RGB XYZ images.
     */
    virtual void insertNewPointCloud(int deviceIndex, std::shared_ptr<OrganizedPointCloud> pointCloud) {
        std::unique_lock lock(pointCloudsMutex);

        if(currentPointClouds.size() <= deviceIndex){
            // Fill point clouds:
            for(int i=currentPointClouds.size(); i < deviceIndex; ++i){
                currentPointClouds.push_back(nullptr);
            }

            // Insert point clouds:
            currentPointClouds.push_back(pointCloud);
        } else {
            // Just update point cloud:
            currentPointClouds[deviceIndex] = pointCloud;
        }
    };

    std::chrono::time_point<std::chrono::steady_clock> lastTime;

    virtual void glTick(){
        // Ensure that currentPointClouds is a temporal copy to ensure that during upload it's not actualized
        // to ensure setting the right flags:
        std::vector<std::shared_ptr<OrganizedPointCloud>> syncCurrentPointClouds;
        {
            std::unique_lock lock(pointCloudsMutex);
            syncCurrentPointClouds = currentPointClouds;
        }
        auto& currentPointClouds = syncCurrentPointClouds;

        glDisable(GL_BLEND);
        // If opengl resources are not initialized yet, do it:
        init();

        // Stores camera ids of cameras which should be rendered:
        std::vector<unsigned int> cameraIDsThatCanBeRendered;

        // Stores if the camera with the respective ID is active:
        bool isCameraActive[CAMERA_COUNT];
        std::fill_n(isCameraActive, CAMERA_COUNT, false);

        // Iterate over all cameras:
        for(unsigned int i = 0; i < currentPointClouds.size(); ++i){
            cameraIsUpdatedThisFrame[i] = false;
            // Check if point cloud is zero. If that's the case,
            if(currentPointClouds[i] != nullptr){
                unsigned int width = currentPointClouds[i]->width;
                unsigned int height = currentPointClouds[i]->height;

                if(!((width == 640 && height == 576) || (width == 968 && height == 608))){
                    std::cout << "WARNING: GL Camera Passes PC Size is: " << width << " x " << height << std::endl;
                }

                cameraIDsThatCanBeRendered.push_back(i);
                isCameraActive[i] = true;

                ensureCameraInitialized(i, currentPointClouds[i]->width, currentPointClouds[i]->height);

                if((currentPointClouds[i]->usageFlags & 1) == 0){
                    currentPointClouds[i]->usageFlags |= 1;
                    cameraIsUpdatedThisFrame[i] = true;
                }
            }
        }

        // Used camera ids:
        usedCameraIDs = cameraIDsThatCanBeRendered;

        // Check if cameras for rendering are available:
        if(cameraIDsThatCanBeRendered.size() == 0){
            //std::cout << "ExperimentalPCPreprocessor: NO CAMERAS FOR RENDERING AVAILABLE!" << std::endl;
            return;
        }

        // Cache main viewport and initialize viewport for depth camera image passes:
        int originalViewport[4];
        glGetIntegerv(GL_VIEWPORT, originalViewport);

        int originalFramebuffer;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &originalFramebuffer);

        // Initialize viewport for depth camera passes:
        glDisable(GL_CULL_FACE);

        {
            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                if(!cameraIsUpdatedThisFrame[cameraID])
                    continue;

                std::shared_ptr<OrganizedPointCloud> currentPC = currentPointClouds[cameraID];

                if(currentPC->depth != nullptr){
                    glBindTexture(GL_TEXTURE_2D, texture2D_inputDepth[cameraID]);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cameraWidth[cameraID], cameraHeight[cameraID], GL_RED_INTEGER, GL_UNSIGNED_SHORT, currentPC->depth);
                }
            }

            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                if(!cameraIsUpdatedThisFrame[cameraID])
                    continue;

                std::shared_ptr<OrganizedPointCloud> currentPC = currentPointClouds[cameraID];
                if(currentPC->colors != nullptr){
                    glBindTexture(GL_TEXTURE_2D, texture2D_inputRGB[cameraID]);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cameraWidth[cameraID], cameraHeight[cameraID], GL_RGBA, GL_UNSIGNED_BYTE, currentPC->colors);
                }
            }
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); // Unbind PBO

            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                if(!cameraIsUpdatedThisFrame[cameraID])
                    continue;

                std::shared_ptr<OrganizedPointCloud> currentPC = currentPointClouds[cameraID];

                if(!lookupInitialized[cameraID]){
                    if(currentPC->lookupImageTo3D != nullptr){
                        glBindTexture(GL_TEXTURE_2D, texture2D_inputLookupImageTo3D[cameraID]);
                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, currentPointClouds[cameraID]->width, currentPointClouds[cameraID]->height, GL_RG, GL_FLOAT, currentPC->lookupImageTo3D);
                    }
                    if(currentPC->lookup3DToImage != nullptr){
                        glBindTexture(GL_TEXTURE_2D, texture2D_inputLookup3DToImage[cameraID]);
                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, LOOKUP_IMAGE_SIZE, LOOKUP_IMAGE_SIZE, GL_RG, GL_FLOAT, currentPC->lookup3DToImage);
                    }
                    lookupInitialized[cameraID] = true;
                    std::cout << "Initialized Lookup! " << cameraID << std::endl;
                }
            }


            // Generate vertices from depth images:
            {
                for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                    if(!cameraIsUpdatedThisFrame[cameraID])
                        continue;

                    glViewport(0, 0, cameraWidth[cameraID], cameraHeight[cameraID]);

                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_genVertices[cameraID]);
                    vertexGenShader.bind();

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, texture2D_inputDepth[cameraID]);
                    vertexGenShader.setUniform("depthTexture", 1);

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, texture2D_inputLookupImageTo3D[cameraID]);
                    vertexGenShader.setUniform("lookupTexture", 2);

                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);

                    currentProcessedVertices[cameraID] = texture2D_inputGenVertices[cameraID];
                }
            }

            if(useReimplementedFilters){
                for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                    if(!cameraIsUpdatedThisFrame[cameraID])
                        continue;

                    glViewport(0, 0, cameraWidth[cameraID], cameraHeight[cameraID]);
                    // Hole Filling Pass:
                    {
                        glBindFramebuffer(GL_FRAMEBUFFER, fbo_pcf_holeFilling[cameraID]);
                        pcfHoleFillingShader.bind();

                        unsigned int hfattachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
                        glDrawBuffers(2, hfattachments);

                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, currentProcessedVertices[cameraID]);
                        pcfHoleFillingShader.setUniform("inputVertices", 1);

                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_2D, texture2D_inputRGB[cameraID]);
                        pcfHoleFillingShader.setUniform("inputColors", 2);

                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_2D, texture2D_inputLookupImageTo3D[cameraID]);
                        pcfHoleFillingShader.setUniform("lookupImageTo3D", 3);

                        glBindVertexArray(VAO_quad);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                    }

                    currentProcessedVertices[cameraID] = texture2D_pcf_holeFilledVertices[cameraID];
                }

                for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                    if(!cameraIsUpdatedThisFrame[cameraID])
                        continue;

                    glViewport(0, 0, cameraWidth[cameraID], cameraHeight[cameraID]);
                    // Noise Removal Pass:
                    {
                        unsigned int currentfbo = temporalFilterFlipFlop[cameraID] ? fbo_pcf_temporalFilterB[cameraID] : fbo_pcf_temporalFilterA[cameraID];
                        unsigned int previousTexture = temporalFilterFlipFlop[cameraID] ? texture2D_pcf_temporalFilterA[cameraID] : texture2D_pcf_temporalFilterB[cameraID];
                        unsigned int writtenTexture = temporalFilterFlipFlop[cameraID] ? texture2D_pcf_temporalFilterB[cameraID] : texture2D_pcf_temporalFilterA[cameraID];

                        glBindFramebuffer(GL_FRAMEBUFFER, currentfbo);
                        noiseRemovalShader.bind();

                        unsigned int eattachments[1] = { GL_COLOR_ATTACHMENT0};
                        glDrawBuffers(1, eattachments);

                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, currentProcessedVertices[cameraID]);
                        noiseRemovalShader.setUniform("currentVertices", 1);

                        glActiveTexture(GL_TEXTURE2);
                        glBindTexture(GL_TEXTURE_2D, previousTexture);
                        noiseRemovalShader.setUniform("previousVertices", 2);

                        glBindVertexArray(VAO_quad);
                        glDrawArrays(GL_TRIANGLES, 0, 6);

                        currentProcessedVertices[cameraID] = writtenTexture;
                    }

                    temporalFilterFlipFlop[cameraID] = !temporalFilterFlipFlop[cameraID];
                }

                for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                    if(!cameraIsUpdatedThisFrame[cameraID])
                        continue;

                    glViewport(0, 0, cameraWidth[cameraID], cameraHeight[cameraID]);
                    // Erosion Pass:
                    {
                        glBindFramebuffer(GL_FRAMEBUFFER, fbo_pcf_erosion[cameraID]);
                        pcfErosionShader.bind();

                        unsigned int eattachments[1] = { GL_COLOR_ATTACHMENT0};
                        glDrawBuffers(1, eattachments);

                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, currentProcessedVertices[cameraID]);
                        pcfErosionShader.setUniform("inputVertices", 1);

                        glBindVertexArray(VAO_quad);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                    }

                    currentProcessedVertices[cameraID] = texture2D_pcf_erosion[cameraID];
                }
            }

            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                if(!cameraIsUpdatedThisFrame[cameraID])
                    continue;

                glViewport(0, 0, cameraWidth[cameraID], cameraHeight[cameraID]);
                // Rejected PASS:
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_rejection[cameraID]);
                    rejectionShader.bind();

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, currentProcessedVertices[cameraID]);
                    rejectionShader.setUniform("pointCloud", 1);

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, useReimplementedFilters ? texture2D_pcf_holeFilledRGB[cameraID] : texture2D_inputRGB[cameraID]);
                    rejectionShader.setUniform("colorTexture", 2);

                    rejectionShader.setUniform("model", currentPointClouds[cameraID]->modelMatrix);
                    rejectionShader.setUniform("shouldClip", shouldClip);
                    rejectionShader.setUniform("clipMin", clipMin);
                    rejectionShader.setUniform("clipMax", clipMax);

                    rejectionShader.setUniform("isRectification", (currentPointClouds[cameraID]->usageFlags & CAMERA_RESPONSIBILITY_RECTIFICATION) != 0);
                    rejectionShader.setUniform("virtualDisplayTransform", Data::instance.virtualDisplayTransform.inverse());

                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }

            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                if(!cameraIsUpdatedThisFrame[cameraID])
                    continue;

                glViewport(0, 0, cameraWidth[cameraID], cameraHeight[cameraID]);
                // Edge Distance PASS:
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_edgeProximity[cameraID]);
                    edgeProximityShader.bind();

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, texture2D_rejection[cameraID]);
                    edgeProximityShader.setUniform("rejectedTexture", 1);
                    edgeProximityShader.setUniform("kernelRadius", 10);

                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }

            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                if(!cameraIsUpdatedThisFrame[cameraID])
                    continue;

                glViewport(0, 0, cameraWidth[cameraID], cameraHeight[cameraID]);
                // Texture a(x) PASS:
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_mls[cameraID]);
                    mlsShader.bind();

                    mlsShader.setUniform("kernelRadius", kernelRadius);
                    mlsShader.setUniform("kernelSpread", kernelSpread);
                    mlsShader.setUniform("p_h", implicitH);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, currentProcessedVertices[cameraID]);
                    mlsShader.setUniform("pointCloud", 1);

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, texture2D_edgeProximity[cameraID]);
                    mlsShader.setUniform("edgeProximity", 2);

                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }

            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                if(!cameraIsUpdatedThisFrame[cameraID])
                    continue;

                glViewport(0, 0, cameraWidth[cameraID], cameraHeight[cameraID]);
                // Texture n(x) PASS:
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_normals[cameraID]);
                    //gl.glDisable(GL_DEPTH_TEST);
                    normalsShader.bind();

                    normalsShader.setUniform("kernelRadius", kernelRadius);
                    normalsShader.setUniform("kernelSpread", kernelSpread);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, texture2D_mlsVertices[cameraID]);
                    normalsShader.setUniform("texture2D_mlsVertices", 1);

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, texture2D_edgeProximity[cameraID]);
                    normalsShader.setUniform("texture2D_edgeProximity", 2);

                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_2D, currentProcessedVertices[cameraID]);
                    normalsShader.setUniform("texture2D_inputVertices", 3);

                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }

            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                if(!cameraIsUpdatedThisFrame[cameraID])
                    continue;

                glViewport(0, 0, cameraWidth[cameraID], cameraHeight[cameraID]);
                // OVERLAP PASS:
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo_qualityEstimate[cameraID]);
                    qualityEstimateShader.bind();

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texture2D_mlsVertices[cameraID]);
                    qualityEstimateShader.setUniform("vertices", 0);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, texture2D_normals[cameraID]);
                    qualityEstimateShader.setUniform("normals", 1);

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, texture2D_edgeProximity[cameraID]);
                    qualityEstimateShader.setUniform("edgeDistances", 2);

                    glBindVertexArray(VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }

            // Set point cloud matrices:
            for(unsigned int cameraID : cameraIDsThatCanBeRendered){
                pointCloudMatrix[cameraID] = currentPointClouds[cameraID]->modelMatrix;
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);
        glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);

        glActiveTexture(GL_TEXTURE0);
    }
};
