// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#define CAMERA_RESPONSIBILITY_RECTIFICATION 32
#define CAMERA_RESPONSIBILITY_OCCLUSION 64
#define CAMERA_RESPONSIBILITY_GESTURES 128

#include <map>

#ifdef USE_CUDA
#include <cuda_runtime.h>
#else
struct float2 {float x, y;};
struct float3 {float x, y, z;};
struct float4 {float x, y, z, w;};
struct uchar4 {unsigned char x, y, z, w;};
#endif

#include "src/math/Mat4.h"
#include "src/gl/primitive/TexCoord.h"

class RGBDCamera;

/**
 * Represents an organized point cloud ordered by the original camera image
 * (Row major order).
 *
 * Note that this organized point cloud
 */
struct OrganizedPointCloud {
private:    
    /**
     * Stores all (statically) lookup3DToImage-pointers that were uploaded to gpu for use with CUDA.
     * Since they are static for each camera (pointer of lookup3DToImage doesn't change), it is not
     * wanted to upload them every frame for performance reasons.
     *
     * Note: Here, we have a memory leak (the GPU memory of these pointers are not deleted) - this
     * deletion could be implemented in the PCStreamer classes.
     */
    static std::map<float*, float2*> uploadedLookup3DToImageTables;
    static std::map<float*, float2*> uploadedLookupImageTo3DTables;

    /** Defines a piece of GPU memory */
    struct GPUMemory {
        size_t size;
        void* pointer;

        GPUMemory(size_t size, void* pointer)
            : size(size)
            , pointer(pointer)
        {}
    };

    /**
     * For performance reasons, we want to reuse gpu / shared memory instead recreating it all the
     * time. This is less a problem because the most time, we need a memory block of the same size
     * again (e.g. in the next frame for the next created organized point cloud.
     */
    void initializeMemory(void** pointer, size_t size){
        #ifdef USE_CUDA
        for(unsigned int i = 0; i < unusedInitializedGPUMemory.size(); ++i){
            if(unusedInitializedGPUMemory[i].size == size){
                *pointer = unusedInitializedGPUMemory[i].pointer;
                unusedInitializedGPUMemory.erase(unusedInitializedGPUMemory.begin() + i);
                return;
            }
        }

        //std::cout << "Allocate new space" << std::endl;
        cudaMalloc(pointer, size);
        #endif
    }

    /** Defines initialized GPU memory which is currently not in use */
    static std::vector<GPUMemory> unusedInitializedGPUMemory;

public:
    OrganizedPointCloud(unsigned int width, unsigned int height)
        : width(width)
        , height(height){};

    /** Stores the xyzw coordinates of all points */
    uint16_t* depth = nullptr;

    /** Stores the rgba coordinates of all points */
    Vec4b* colors = nullptr;

    /** High res Colors */
    uint8_t* highResColors = nullptr;

    /** High Res Width (-1, if image is nullptr) */
    int highResWidth = -1;

    /** High Res Height (-1, if image is nullptr) */
    int highResHeight = -1;

    /** Function that transforms a color pixel coordinate (e.g. 1920 x 1080) to a depth pixel coordinate (e.g. 640 x 576) */
    std::function<std::pair<float,float>(float x, float y)> highResColorToDepthTransformer;

    /** Function that transforms a depth pixel coordinate (e.g. 640 x 576) to a color pixel coordinate (e.g. 1920 x 1080) */
    std::function<std::pair<float,float>(float x, float y, float depth)> depthToHighResTransformer;

    std::function<Vec4f(float x, float y)> projectHighResIn3D;

    Mat4f colorToDepth;

    uint16_t* ir = nullptr;

    /** Stores the normal of all points */
    Vec4f* normals = nullptr;

    /** Texture coordinates */
    TexCoord* texCoords = nullptr;

    /** Markers found in the infrared image */
    std::vector<Vec4f> imageIRMarkers;

    /** Rays */
    std::vector<Vec4f> irMarkerRays;

    /** */
    int frameID = -1;

    /**
     * 3D-to-image Lookup Table (memory is managed by READER to avoid copying for every point cloud
     * since the values doesn't change between different images!)
     */
    float* lookup3DToImage = nullptr;

    /**
     * 3D-to-image Lookup Table (memory is managed by READER to avoid copying for every point cloud
     * since the values doesn't change between different images!)
     */
    float* lookupImageTo3D = nullptr;

    /** Stores the xyzw coordiantes of all points */
    float4* gpuDepth = nullptr;

    /** Stores the rgba coordinates of all points */
    uchar4* gpuColors = nullptr;

    /** Stores the normal of all points */
    float4* gpuNormals = nullptr;

    /** Texture coordinates */
    float2* gpuTexCoords = nullptr;

    float2* gpuLookup3DToImage = nullptr;

    float2* gpuLookupImageTo3D = nullptr;

    /**
     * 3D-to-image Lookup Table Size for one dimension (which results in a square
     * resolution)
     */
    unsigned int lookup3DToImageSize = 0;

    /** Transforms the point cloud positions from camera space into world space */
    Mat4f modelMatrix;

    /** Width of the original depth image */
    unsigned int width = -1;

    /** Height of the original depth image */
    unsigned int height = -1;

    /**
     *  Acceleration of the camera; PCStreamer often only set the initial acceleration
     *  on startup.
     */
    Vec4f camAcceleration;

    /**
     * Usually not used / needed for processing, implemented for writing synchronized
     * training data
     */
    float timestamp = -1;

    /**
     * Indicates whether the last change was performed on GPU (or CPU) and whether
     * the (positions, colors, ...) pointer are up to date or whether the (gpuPositions,
     * gpuColors, ...) pointer are up to date.
     */
    bool gpu = false;

    /**
     * 1: Processed by CameraPasses of BlendPCR
     */
    unsigned int usageFlags = 0;

    /**
     * The origin of this point cloud:
     */
    RGBDCamera* camera;

    /**
     * Initializes the gpu... variables (if necessary) and copies the content from the
     * corresponding position, colors, ... arrays.
     */
    void toGPU(){
        #ifdef USE_CUDA
        if(gpu)
            return;

        if(colors != nullptr){
            if(gpuColors == nullptr)
                initializeMemory((void**)&gpuColors, width * height * sizeof(uchar4));
            cudaMemcpy(gpuColors, colors, width * height * sizeof(uchar4), cudaMemcpyHostToDevice);
        }

        if(depth != nullptr){
            if(gpuDepth == nullptr)
                initializeMemory((void**)&gpuDepth, width * height * sizeof(uint16_t));
            cudaMemcpy(gpuDepth, depth, width * height * sizeof(uint16_t), cudaMemcpyHostToDevice);
        }

        if(normals != nullptr){
            if(gpuNormals == nullptr)
                initializeMemory((void**)&gpuNormals, width * height * sizeof(float4));
            cudaMemcpy(gpuNormals, normals, width * height * sizeof(float4), cudaMemcpyHostToDevice);
        }

        if(texCoords != nullptr){
            if(gpuTexCoords == nullptr)
                initializeMemory((void**)&gpuTexCoords, width * height * sizeof(float2));
            cudaMemcpy(gpuTexCoords, gpuTexCoords, width * height * sizeof(float2), cudaMemcpyHostToDevice);
        }

        if(lookup3DToImage != nullptr){
            // If there is a key in the map:
            if (uploadedLookup3DToImageTables.find(lookup3DToImage) != uploadedLookup3DToImageTables.end()) {
                gpuLookup3DToImage = uploadedLookup3DToImageTables[lookup3DToImage];
            } else {
                cudaMalloc(&gpuLookup3DToImage, lookup3DToImageSize * lookup3DToImageSize * sizeof(float) * 2);
                cudaMemcpy(gpuLookup3DToImage, lookup3DToImage, lookup3DToImageSize * lookup3DToImageSize * sizeof(float) * 2, cudaMemcpyHostToDevice);
                uploadedLookup3DToImageTables[lookup3DToImage] = gpuLookup3DToImage;
            }
        }

        if(lookupImageTo3D != nullptr){
            // If there is a key in the map:
            if (uploadedLookupImageTo3DTables.find(lookupImageTo3D) != uploadedLookupImageTo3DTables.end()) {
                gpuLookupImageTo3D = uploadedLookupImageTo3DTables[lookupImageTo3D];
            } else {
                cudaMalloc(&gpuLookupImageTo3D, width * height * sizeof(float) * 2);
                cudaMemcpy(gpuLookupImageTo3D, lookupImageTo3D, width * height * sizeof(float) * 2, cudaMemcpyHostToDevice);
                uploadedLookupImageTo3DTables[lookupImageTo3D] = gpuLookupImageTo3D;
            }
        }

        gpu = true;
        #endif
    }

    /**
     * Copies the data back to the cpu (gpuPositions -> positions, gpuColors -> colors, ...).
     */

    void toCPU(){
        #ifdef USE_CUDA
        if(!gpu)
            return;

        if(colors != nullptr){
            cudaError_t err = cudaMemcpy(colors, gpuColors, width * height * sizeof(uchar4), cudaMemcpyDeviceToHost);
            if (err != cudaSuccess) {
                fprintf(stderr, "GPU->CPU copy error in pc->colors: %s\n", cudaGetErrorString(err));
            }
        }

        if(depth != nullptr){
            cudaError_t err = cudaMemcpy(depth, gpuDepth, width * height * sizeof(uint16_t), cudaMemcpyDeviceToHost);
            if (err != cudaSuccess) {
                fprintf(stderr, "GPU->CPU copy error in pc->positions:  %s\n", cudaGetErrorString(err));
            }
        }

        if(normals != nullptr)
            cudaMemcpy(normals, gpuNormals, width * height * sizeof(float4), cudaMemcpyDeviceToHost);

        if(texCoords != nullptr)
            cudaMemcpy(texCoords, gpuTexCoords, width * height * sizeof(float2), cudaMemcpyDeviceToHost);

        gpu = false;
        #endif
    }

    ~OrganizedPointCloud(){
        if(gpuDepth != nullptr){
            unusedInitializedGPUMemory.emplace_back(sizeof(uint16_t) * width * height, gpuDepth);
            gpuDepth = nullptr;
        }

        if(gpuColors != nullptr){
            unusedInitializedGPUMemory.emplace_back(sizeof(Vec4b) * width * height, gpuColors);
            gpuColors = nullptr;
        }

        if(gpuNormals != nullptr){
            unusedInitializedGPUMemory.emplace_back(sizeof(Vec4f) * width * height, gpuNormals);
            gpuNormals = nullptr;
        }

        if(gpuTexCoords != nullptr){
            unusedInitializedGPUMemory.emplace_back(sizeof(TexCoord) * width * height, gpuTexCoords);
            gpuTexCoords = nullptr;
        }

        if(depth != nullptr){
            delete[] depth;
            depth = nullptr;
        }

        if(colors != nullptr){
            delete[] colors;
            colors = nullptr;
        }

        if(highResColors != nullptr){
            delete[] highResColors;
            highResColors = nullptr;
        }

        if(ir != nullptr){
            delete[] ir;
            ir = nullptr;
        }

        if(normals != nullptr){
            delete[] normals;
            normals = nullptr;
        }

        if(texCoords != nullptr){
            delete[] texCoords;
            texCoords = nullptr;
        }
    }

    Vec4f getPosition(int x, int y){
        if(x < 0 || y < 0 || x >= width || y >= height)
            return Vec4f();

        int pixelID = y * width + x;
        float z3D = depth[pixelID] / 1000.f;

        float x3D = lookupImageTo3D[pixelID * 2] * z3D;
        float y3D = lookupImageTo3D[pixelID * 2 + 1] * z3D;

        return Vec4f(x3D, y3D, z3D, 1.f);
    }

    Vec4f getDirection(int x, int y){
        if(x < 0 || y < 0 || x >= width || y >= height)
            return Vec4f();

        int pixelID = y * width + x;
        float z3D = 1;

        float x3D = lookupImageTo3D[pixelID * 2] * z3D;
        float y3D = lookupImageTo3D[pixelID * 2 + 1] * z3D;

        return Vec4f(x3D, y3D, z3D, 1.f);
    }

    std::pair<float, float> getImageCoord(Vec4f v){
        float x = v.x / v.z;
        float y = v.y / v.z;

        float pX = (x * 0.5f + 0.5f) * (lookup3DToImageSize - 1);
        float pY = (y * 0.5f + 0.5f) * (lookup3DToImageSize - 1);

        if(pX < 0.0f)
            pX = 0.0f;
        else if(pX > lookup3DToImageSize - 1)
            pX = float(lookup3DToImageSize - 1);

        if(pY < 0.0f)
            pY = 0.0f;
        else if(pY > lookup3DToImageSize - 1)
            pY = float(lookup3DToImageSize - 1);

        int ix0 = static_cast<int>(pX);
        int iy0 = static_cast<int>(pY);

        int ix1 = (ix0 < lookup3DToImageSize - 1) ? ix0 + 1 : ix0;
        int iy1 = (iy0 < lookup3DToImageSize - 1) ? iy0 + 1 : iy0;

        float dx = pX - ix0;
        float dy = pY - iy0;

        int index00 = (iy0 * lookup3DToImageSize + ix0) * 2;
        int index10 = (iy0 * lookup3DToImageSize + ix1) * 2;
        int index01 = (iy1 * lookup3DToImageSize + ix0) * 2;
        int index11 = (iy1 * lookup3DToImageSize + ix1) * 2;

        float v00x = lookup3DToImage[index00];
        float v00y = lookup3DToImage[index00 + 1];
        float v10x = lookup3DToImage[index10];
        float v10y = lookup3DToImage[index10 + 1];
        float v01x = lookup3DToImage[index01];
        float v01y = lookup3DToImage[index01 + 1];
        float v11x = lookup3DToImage[index11];
        float v11y = lookup3DToImage[index11 + 1];

        float interpX = v00x * (1 - dx) * (1 - dy) + v10x * dx * (1 - dy) + v01x * (1 - dx) * dy + v11x * dx * dy;
        float interpY = v00y * (1 - dx) * (1 - dy) + v10y * dx * (1 - dy) + v01y * (1 - dx) * dy + v11y * dx * dy;

        return std::make_pair(interpX, interpY);
    }

    static void cleanupStaticMemory();
};
