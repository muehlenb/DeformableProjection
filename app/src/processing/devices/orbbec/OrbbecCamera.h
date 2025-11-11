// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include <libobsensor/ObSensor.hpp>
#include <libobsensor/h/Utils.h>

#include "src/processing/devices/RGBDCamera.h"

/**
 * A streamer when using a single or multiple Orbbec Camera Devices.
 */

class OrbbecCamera : public RGBDCamera {
    std::shared_ptr<ob::PointCloudFilter> pointCloudFilter;
    std::shared_ptr<ob::Align> aligner;

    float* lookup2DTo3D = nullptr;
    float* lookup3DTo2D = nullptr;

    std::shared_ptr<ob::Pipeline> pipe;
    std::string cameraSerial = "Orbbec1";
    int deviceIndex;

    bool isPipeRunning = false;

    std::function<void(int, std::shared_ptr<OrganizedPointCloud>)> pointCloudCallback;

public:
    OrbbecCamera(std::shared_ptr<ob::Pipeline> pipe, int deviceIndex, std::function<void(int, std::shared_ptr<OrganizedPointCloud>)> pointCloudCallback)
        : pipe(pipe)
        , deviceIndex(deviceIndex)
        , pointCloudCallback(pointCloudCallback)
    {
        aligner = std::make_shared<ob::Align>(OB_STREAM_DEPTH);
        pointCloudFilter = std::make_shared<ob::PointCloudFilter>();

        cameraSerial = pipe->getDevice()->getDeviceInfo()->serialNumber();
    }

    void ensureLookupsInitialized(std::shared_ptr<ob::FrameSet>& frameSet, int width, int height){
        bool initLookup2DTo3D = false;
        if(lookup2DTo3D == nullptr){
            uint16_t *data   = reinterpret_cast<uint16_t *>(frameSet->depthFrame()->getData());
            for(int i=0; i < width * height; ++i)
                data[i] = 1000;

            initLookup2DTo3D = true;
        }


        if(initLookup2DTo3D){
            std::shared_ptr<ob::FrameSet> alignedFrameset = std::static_pointer_cast<ob::FrameSet>(aligner->process(frameSet));
            std::shared_ptr<ob::Frame> frame = pointCloudFilter->process(alignedFrameset);
            lookup2DTo3D = new float[width * height * 2];
            for(int y = 0; y < height; ++y){
                for(int x = 0; x < width; ++x){
                    int idx = (x + y * width);

                    lookup2DTo3D[idx*2] = ((float*)frame->data())[idx*3] / 1000.f;
                    lookup2DTo3D[idx*2+1] = ((float*)frame->data())[idx*3+1] / 1000.f;
                }
            }
            generate3DTo2D();
        }
    }

    void generate3DTo2D() {
        float* newLookup = new float[1024 * 1024 * 2];

        // Initialisierung mit -1
        for (int y = 0; y < 1024; ++y) {
            for (int x = 0; x < 1024; ++x) {
                int id = y * 1024 + x;
                newLookup[id * 2] = -1.0f;
                newLookup[id * 2 + 1] = -1.0f;
            }
        }

        for (int y = 0; y < 575; y++) {
            for (int x = 0; x < 639; x++) {
                int id = y * 640 + x;
                float luX00 = lookup2DTo3D[id * 2];
                float luY00 = lookup2DTo3D[id * 2 + 1];

                int id01 = (y + 1) * 640 + x;
                float luY01 = lookup2DTo3D[id01 * 2 + 1];

                int id10 = y * 640 + (x + 1);
                float luX10 = lookup2DTo3D[id10 * 2];

                float fStartX = (luX00 * 0.5f + 0.5f) * 1023.0f;
                float fEndX   = (luX10 * 0.5f + 0.5f) * 1023.0f;
                float fStartY = (luY00 * 0.5f + 0.5f) * 1023.0f;
                float fEndY   = (luY01 * 0.5f + 0.5f) * 1023.0f;

                // Falls vertauscht, Werte tauschen, um Interpolation korrekt zu halten
                if (fStartX > fEndX) {
                    std::swap(fStartX, fEndX);
                    std::swap(luX00, luX10);
                }
                if (fStartY > fEndY) {
                    std::swap(fStartY, fEndY);
                    std::swap(luY00, luY01);
                }

                for (int pY = std::max(0, int(fStartY)); pY <= std::min(1023, int(ceil(fEndY))); pY++) {
                    for (int pX = std::max(0, int(fStartX)); pX <= std::min(1023, int(ceil(fEndX))); pX++) {
                        float rangeX = fEndX - fStartX;
                        float rangeY = fEndY - fStartY;

                        float progressX = (rangeX != 0) ? (pX - fStartX) / rangeX : 0.0f;
                        float progressY = (rangeY != 0) ? (pY - fStartY) / rangeY : 0.0f;

                        int oid = pY * 1024 + pX;

                        // TODO: FOR SOMEWHAT MORE PRECISION, IMPLEMENT ALSO INTERPOLATION BETWEEN ...
                        newLookup[oid * 2]     = x + progressX;
                        newLookup[oid * 2 + 1] = y + progressY;
                    }
                }
            }
        }
        lookup3DTo2D = newLookup;
    }

    void startStream() {
        std::shared_ptr<ob::Config> config = std::make_shared<ob::Config>();
        config->enableVideoStream(OB_STREAM_COLOR, 1280, 720, 30, OB_FORMAT_RGB);
        config->enableVideoStream(OB_STREAM_DEPTH, 640, 576, 30, OB_FORMAT_Y16);
        config->setFrameAggregateOutputMode(OB_FRAME_AGGREGATE_OUTPUT_FULL_FRAME_REQUIRE);
        pipe->enableFrameSync();

        pointCloudFilter->setCreatePointFormat(OB_FORMAT_POINT);
        auto dev = pipe->getDevice();

        isPipeRunning = true;
        pipe->start(config, [this](std::shared_ptr<ob::FrameSet> frameSet) {
            try {
                uint32_t width = frameSet->depthFrame()->width();
                uint32_t height = frameSet->depthFrame()->height();

                ensureLookupsInitialized(frameSet, width, height);

                std::shared_ptr<OrganizedPointCloud> pc = std::make_shared<OrganizedPointCloud>(width, height);
                pc->depth = new uint16_t[width * height];
                pc->colors = new Vec4b[width * height];
                pc->lookupImageTo3D = lookup2DTo3D;
                pc->lookup3DToImage = lookup3DTo2D;
                pc->lookup3DToImageSize = 1024;
                pc->modelMatrix = transformation;
                pc->camera = this;
                pc->usageFlags = CAMERA_RESPONSIBILITY_GESTURES | CAMERA_RESPONSIBILITY_OCCLUSION | CAMERA_RESPONSIBILITY_RECTIFICATION;

                pc->highResWidth = 1280;
                pc->highResHeight = 720;
                pc->highResColors = new uint8_t[pc->highResWidth * pc->highResHeight * 3];
                std::memcpy(pc->highResColors, frameSet->colorFrame()->data(), pc->highResWidth * pc->highResHeight * 3 * sizeof(uint8_t));

                auto colorProfile =  frameSet->colorFrame()->getStreamProfile();
                auto depthProfile = frameSet->depthFrame()->getStreamProfile();
                OBD2CTransform transDepthToColor = depthProfile->getExtrinsicTo(colorProfile);
                OBD2CTransform transColorToDepth = colorProfile->getExtrinsicTo(depthProfile);


                // Get the intrinsic and distortion parameters for the color and depth streams
                OBCameraIntrinsic colorIntrinsic = colorProfile->as<ob::VideoStreamProfile>()->getIntrinsic();
                OBCameraDistortion colorDistortion = colorProfile->as<ob::VideoStreamProfile>()->getDistortion();
                OBCameraIntrinsic depthIntrinsic = depthProfile->as<ob::VideoStreamProfile>()->getIntrinsic();
                OBCameraDistortion depthDistortion = depthProfile->as<ob::VideoStreamProfile>()->getDistortion();

                OBD2CTransform transIdentity = colorProfile->getExtrinsicTo(colorProfile);

                std::weak_ptr<OrganizedPointCloud> weakPc = pc;
                pc->highResColorToDepthTransformer = [this, colorIntrinsic, colorDistortion, depthIntrinsic, depthDistortion, transDepthToColor, transColorToDepth, weakPc](float x, float y){
                    if (auto pcLocked = weakPc.lock()) {
                        OBPoint2f result;
                        transformationColor2dToDepth2d(colorIntrinsic, colorDistortion, depthIntrinsic, depthDistortion, transDepthToColor, transColorToDepth, OBPoint2f({x,y}), pcLocked->depth, &result);
                        return std::pair<float,float>(result.x, result.y);
                    }
                    return std::pair<float,float>(0.f, 0.f);
                };

                pc->projectHighResIn3D = [colorIntrinsic, transIdentity](float x, float y){
                    OBPoint2f src;
                    src.x = x;
                    src.y = y;

                    OBPoint3f result;
                    ob_error* error;

                    ob_transformation_2d_to_3d(src, 1000.f, colorIntrinsic, transIdentity, &result, &error);
                    return Vec4f(result.x, result.y, result.z, 0).normalized();
                };

                std::memcpy(pc->depth, frameSet->depthFrame()->data(), width*height * sizeof(uint16_t));
                std::shared_ptr<ob::FrameSet> alignedFrameset = std::static_pointer_cast<ob::FrameSet>(aligner->process(frameSet));
                convert3DTo4D((uint8_t*)(alignedFrameset->colorFrame()->data()), (uint8_t*)(pc->colors), width * height);

                pointCloudCallback(deviceIndex, pc);

            } catch (const ob::Error& e) {
                std::cerr << "[Orbbec/Callback] "
                          << /* e.message()/e.getMessage()/e.what() */ "exception in frame callback"
                          << std::endl;
                // optional: mehr Details, falls verfügbar
            } catch (const std::exception& e) {
                std::cerr << "[Callback std::exception] " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[Callback] Unknown exception\n";
            }
        });
    }

    virtual std::string getSerial() override{
        return cameraSerial;
    }

    virtual int responsibilityFlags() override{
        return CAMERA_RESPONSIBILITY_GESTURES | CAMERA_RESPONSIBILITY_OCCLUSION;
    }

    virtual bool start() override {
        startStream();
        return true;
    }

    virtual std::string type() override {
        return "Orbbec_Femto";
    }

    virtual bool enableIRLight() override {
        if(isPipeRunning)
            return false;

        startStream();

        return true;
    }

    virtual bool disableIRLight() override {
        if(!isPipeRunning)
            return false;

        pipe->stop();
        isPipeRunning = false;

        return false;
    }


private:

    void convert3DTo4D(const float* src, float* dest, size_t N, float w = 1.0f) {
        const float divisor = 1000.0f;
        size_t i = 0;

#ifdef __AVX__
        __m128 w_vec = _mm_set_ps(w, 0.0f, 0.0f, 0.0f);
        __m128 divisor_vec = _mm_set1_ps(divisor);
        for (; i + 3 < N; i += 4) {
            __m128 a = _mm_div_ps(_mm_loadu_ps(&src[i * 3]), divisor_vec);
            __m128 b = _mm_div_ps(_mm_loadu_ps(&src[(i + 1) * 3]), divisor_vec);
            __m128 c = _mm_div_ps(_mm_loadu_ps(&src[(i + 2) * 3]), divisor_vec);
            __m128 d = _mm_div_ps(_mm_loadu_ps(&src[(i + 3) * 3]), divisor_vec);

            _mm_storeu_ps(&dest[i * 4], _mm_blend_ps(a, w_vec, 0x8));
            _mm_storeu_ps(&dest[(i + 1) * 4], _mm_blend_ps(b, w_vec, 0x8));
            _mm_storeu_ps(&dest[(i + 2) * 4], _mm_blend_ps(c, w_vec, 0x8));
            _mm_storeu_ps(&dest[(i + 3) * 4], _mm_blend_ps(d, w_vec, 0x8));
        }
#endif

        for (; i < N; ++i) {
            dest[i * 4] = src[i * 3] / divisor;
            dest[i * 4 + 1] = src[i * 3 + 1] / divisor;
            dest[i * 4 + 2] = src[i * 3 + 2] / divisor;
            dest[i * 4 + 3] = w;
        }
    }
    void convert3DTo4D(const uint8_t* src, uint8_t* dest, size_t N, uint8_t w = 255) {
        size_t i = 0;

        for (; i < N; ++i) {
            dest[i * 4] = src[i * 3 + 2];
            dest[i * 4 + 1] = src[i * 3 + 1];
            dest[i * 4 + 2] = src[i * 3 ];
            dest[i * 4 + 3] = w;
        }
    }


    /**
     * Function from orbbec sdk to project color pixel to depth pixel coordinate
     */
    const bool isPixelValid(const OBPoint2f curr, const OBPoint2f start, const OBPoint2f end) {
        bool valid1 = end.x >= start.x && end.x >= curr.x && curr.x >= start.x;
        bool valid2 = end.x <= start.x && end.x <= curr.x && curr.x <= start.x;
        bool valid3 = end.y >= start.y && end.y >= curr.y && curr.y >= start.y;
        bool valid4 = end.y <= start.y && end.y <= curr.y && curr.y <= start.y;
        return (valid1 || valid2) && (valid3 || valid4);
    }

    /**
     * Function from orbbec sdk to project color pixel to depth pixel coordinate
     */
    const void nextPixel(OBPoint2f &curr, const OBPoint2f start, const OBPoint2f end) {
        if(fabsf(end.x - start.x) < 1e-4) {
            curr.y = end.y > curr.y ? curr.y + 1 : curr.y - 1;
        }
        else {
            float slope = (end.y - start.y) / (end.x - start.x);

            if(fabs(end.x - curr.x) > fabs(end.y - curr.y)) {
                curr.x = end.x > curr.x ? curr.x + 1 : curr.x - 1;
                curr.y = end.y - slope * (end.x - curr.x);
            }
            else {
                curr.y = end.y > curr.y ? curr.y + 1 : curr.y - 1;
                curr.x = end.x - ((end.y - curr.y) / slope);
            }
        }
    }


    /**
     * Function from orbbec sdk to project color pixel to depth pixel coordinate
     */
    const bool transformationColor2dToDepth2d(OBCameraIntrinsic colorIntrinsic, OBCameraDistortion colorDistortion, OBCameraIntrinsic depthIntrinsic, OBCameraDistortion depthDistortion, OBD2CTransform transDepthToColor, OBD2CTransform transColorToDepth, const OBPoint2f colorPixel, uint16_t *depthMap, OBPoint2f *depthPixel) {
        float     depthRangeMm[2] = { 60.f, 8000.f };
        OBPoint2f nearPixelDepth, farthestPixelDepth;
        bool      nearValid = 0, farthestValid = 0;
        ob_error* error;

        // color pixel to depth pixel when z= depthRangeMm[0]
        nearValid = ob_transformation_2d_to_2d(colorPixel, depthRangeMm[0], colorIntrinsic, colorDistortion, depthIntrinsic, depthDistortion, transColorToDepth, &nearPixelDepth, &error);

        if(!nearValid) {
            if(nearPixelDepth.x < 0) {
                nearPixelDepth.x = 0;
            }
            if(nearPixelDepth.y < 0) {
                nearPixelDepth.y = 0;
            }
            if(nearPixelDepth.x > depthIntrinsic.width - 1) {
                nearPixelDepth.x = (float)depthIntrinsic.width - 1;
            }
            if(nearPixelDepth.y > depthIntrinsic.height - 1) {
                nearPixelDepth.y = (float)depthIntrinsic.height - 1;
            }
        }

        // color pixel to depth pixel when z= depthRangeMm[1]
        farthestValid = ob_transformation_2d_to_2d(colorPixel, depthRangeMm[1], colorIntrinsic, colorDistortion, depthIntrinsic, depthDistortion, transColorToDepth, &farthestPixelDepth, &error);

        if(!farthestValid) {
            if(farthestPixelDepth.x < 0) {
                farthestPixelDepth.x = 0;
            }
            if(farthestPixelDepth.y < 0) {
                farthestPixelDepth.y = 0;
            }
            if(farthestPixelDepth.x > depthIntrinsic.width - 1) {
                farthestPixelDepth.x = (float)depthIntrinsic.width - 1;
            }
            if(farthestPixelDepth.y > depthIntrinsic.height - 1) {
                farthestPixelDepth.y = (float)depthIntrinsic.height - 1;
            }
        }

        // search along line for the depth pixel that it's projected pixel is the closest to the input pixel
        float minDist = std::numeric_limits<float>::max();
        for(OBPoint2f curPixel = nearPixelDepth; isPixelValid(curPixel, nearPixelDepth, farthestPixelDepth); nextPixel(curPixel, nearPixelDepth, farthestPixelDepth)){
            int x     = (int)curPixel.x;
            int y     = (int)curPixel.y;
            int index = y * depthIntrinsic.width + x;

            if(depthMap[index] == 0)
                continue;

            float depth = depthMap[index];

            OBPoint2f curColorPixel{};
            ob_transformation_2d_to_2d(curPixel, depth, depthIntrinsic, depthDistortion, colorIntrinsic, colorDistortion, transDepthToColor, &curColorPixel, &error);
            float errDist = (float)(pow((curColorPixel.x - colorPixel.x), 2) + pow((curColorPixel.y - colorPixel.y), 2));

            if(errDist < minDist) {
                minDist       = errDist;
                depthPixel->x = (int)curPixel.x;
                depthPixel->y = (int)curPixel.y;
            }
        }

        return true;
    }

};
