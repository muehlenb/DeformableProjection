// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include "src/Data.h"
#include "src/simulation/scene/components/Projector.h"
#include "src/controller/calibration/CalibrationState.h"

#include <vector>
#include <opencv2/opencv.hpp>

/**
 * Finds the projection area and checks how the minimal and maximal
 * brightness of a projector are perceived in the cameras image.
 */
struct DepthReconstructor : public CalibrationState {
    std::vector<double*> smoothedDepthImages;
    std::vector<Vec4f*> smoothedPointCloud;

    std::vector<std::shared_ptr<OrganizedPointCloud>> previousPointClouds;

    int currentDepthCamera = 0;
    int currentState = 0;
    int currentTick = 0;

    bool finished = false;

    ~DepthReconstructor(){
        for(double* dI : smoothedDepthImages){
            delete[] dI;
        }
    }

    void showDoubleGrayImage(const double* data, int width, int height)
    {
        // Wrappe die Daten in ein CV_64F-Mat
        cv::Mat img64(height, width, CV_64F, (void*)data);

        // Ziel-Matrix in 8 Bit
        cv::Mat img8(height, width, CV_8U);

        // Clip und lineare Skalierung: [1000,4000] → [0,255]
        double minVal = 1400.0;
        double maxVal = 3000.0;
        double scale = 255.0 / (maxVal - minVal);

        for (int y = 0; y < height; ++y)
        {
            const double* srcRow = img64.ptr<double>(y);
            uchar* dstRow = img8.ptr<uchar>(y);
            for (int x = 0; x < width; ++x)
            {
                double v = srcRow[x];
                if (v <= minVal) v = 0;
                else if (v >= maxVal) v = 255;
                else v = (v - minVal) * scale;
                dstRow[x] = static_cast<uchar>(v);
            }
        }

        // Anzeigen
        cv::imshow("Graustufenbild", img8);
        cv::waitKey(20);
    }

    void smoothIgnoreLow(const double* src, double* dst, int width, int height)
    {
        const int ksize = 11;
        const int kradius = ksize / 2;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                double centerVal = src[y * width + x];
                double sum = 0.0;
                int count = 0;

                // über 5x5-Kernel iterieren
                for (int ky = -kradius; ky <= kradius; ++ky)
                {
                    int yy = y + ky;
                    if (yy < 0 || yy >= height) continue;

                    for (int kx = -kradius; kx <= kradius; ++kx)
                    {
                        int xx = x + kx;
                        if (xx < 0 || xx >= width) continue;

                        double val = src[yy * width + xx];
                        if (val >= 10.0 && std::abs(val - centerVal) < 10.0) {  // nur Werte >= 10 berücksichtigen
                            sum += val;
                            count++;
                        }
                    }
                }

                if (count > 0)
                    dst[y * width + x] = sum / count;
                else
                    dst[y * width + x] = 0.0; // oder src[y*width+x], falls gewünscht
            }
        }
    }

    void showPointCloudXYZasRGB(Vec4f*& cloud, int width, int height)
    {
        cv::Mat rgbImg(height, width, CV_8U);

        const float minVal = 1.8f;
        const float maxVal = 2.2f;
        const float maxValXY = 1.0f;
        const float minValXY = -1.0f;
        const float scale = 255.0f / (maxVal - minVal);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                Vec4f p = cloud[y * width + x];

                float X = std::max(minVal, std::min(maxValXY, p.x));
                float Y = std::max(minVal, std::min(maxValXY, p.y));
                float Z = std::max(minVal, std::min(maxVal, p.z));

                uchar r = static_cast<uchar>((X - minValXY) * scale);
                uchar g = static_cast<uchar>((Y - minValXY) * scale);
                uchar b = static_cast<uchar>((Z - minVal) * scale);

                rgbImg.at<uchar>(y, x) = b;//cv::Vec3b(b, g, r);
            }
        }

        cv::imshow("PointCloud RGB", rgbImg);
        cv::waitKey(20);
    }

    void renderNothingOnProjectors(){
        for(std::shared_ptr<Projector>& projector : Data::instance.projectors){
            if(projector != nullptr){
                int imageWidth = 3840;
                int imageHeight = 2160;
                projector->imageFBO.bind(imageWidth, imageHeight);
                glViewport(0, 0, imageWidth, imageHeight);

                glClearColor(0.0, 0.0, 0.0, 1.0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    virtual void process(std::vector<std::shared_ptr<OrganizedPointCloud>>& pointClouds) override {
        renderNothingOnProjectors();

        if(finished)
            return;

        // Ensure that enough temporary depth camera exists:
        for(int i = smoothedDepthImages.size(); i < pointClouds.size(); ++i){
            smoothedDepthImages.push_back(new double[pointClouds[i]->width * pointClouds[i]->height]);

            // Copy Initial Depth Image:
            for(int y = 0; y < pointClouds[i]->height; ++y){
                for(int x = 0; x < pointClouds[i]->width; ++x){
                    smoothedDepthImages[i][y * pointClouds[i]->width + x] = pointClouds[i]->depth[y * pointClouds[i]->width + x];
                }
            }

            previousPointClouds = pointClouds;
        }

        // State 0: Enable the right camera and wait a bit:
        if(currentState == 0){
            if(currentTick == 0){
                for(int i = 0; i < pointClouds.size(); ++i){
                    bool shouldEnable = i == currentDepthCamera;
                    std::cout << "Request " << (shouldEnable ? "Enable" : "Disable") << " IR Light of Camera " << i << std::endl;
                    if(shouldEnable){
                        pointClouds[i]->camera->enableIRLight();
                    } else {
                        pointClouds[i]->camera->disableIRLight();
                    }
                }
            }

            // Jump to next state:
            if(currentTick > 120){
                currentState = 1;
                currentTick = 0;
                return;
            }
            ++currentTick;
        }

        // Capture (here tick stands for a new point cloud):
        else if(currentState == 1){
            if(pointClouds[currentDepthCamera] != previousPointClouds[currentDepthCamera]){
                std::shared_ptr<OrganizedPointCloud>& pc = pointClouds[currentDepthCamera];
                double* sDI = smoothedDepthImages[currentDepthCamera];
                for(int y = 0; y < pc->height; ++y){
                    for(int x = 0; x < pc->width; ++x){
                        double& smoothed = sDI[y * pc->width + x];
                        uint16_t& origin = pc->depth[y * pc->width + x];

                        // Only if valid, else skip:
                        if(origin > 0){

                            // If smoothed not valid, directly set:
                            if(smoothed < 10 || abs(smoothed - origin) > 20){
                                smoothed = origin;
                            } else { // Else smooth:
                                smoothed = origin * 0.05 + smoothed * 0.95;
                            }
                        }
                    }
                }
                showDoubleGrayImage(smoothedDepthImages[currentDepthCamera], pc->width, pc->height);
                previousPointClouds[currentDepthCamera] = pointClouds[currentDepthCamera];
                ++currentTick;
            }

            if(currentTick > 30){
                std::shared_ptr<OrganizedPointCloud>& pc = pointClouds[currentDepthCamera];
                double* sDI = smoothedDepthImages[currentDepthCamera];
                double* sDISmooth = new double[pc->width * pc->height];
                smoothIgnoreLow(sDI, sDISmooth, pc->width, pc->height);

                Vec4f* smoothedPC = new Vec4f[pc->width * pc->height];

                for(int y = 0; y < pc->height; ++y){
                    for(int x = 0; x < pc->width; ++x){

                        int pixelID = y * pc->width + x;
                        float z3D = sDISmooth[pixelID] / 1000.f;

                        float x3D = pc->lookupImageTo3D[pixelID * 2] * z3D;
                        float y3D = pc->lookupImageTo3D[pixelID * 2 + 1] * z3D;

                        smoothedPC[pixelID] = Vec4f(x3D, y3D, z3D, 1.f);
                    }
                }
                showDoubleGrayImage(sDISmooth, pc->width, pc->height);
                delete[] sDISmooth;
                sDISmooth = nullptr;
                smoothedPointCloud.push_back(smoothedPC);

                if(currentDepthCamera + 1 >= pointClouds.size()){
                    finished = true;
                    cv::destroyAllWindows();
                } else {
                    currentState = 0;
                    currentTick = 0;
                    ++currentDepthCamera;
                }
            }
        }
    }


    virtual bool isFinished() override {
        return finished;
    }
};
