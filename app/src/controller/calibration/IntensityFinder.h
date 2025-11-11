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
struct IntensityFinder : public CalibrationState {
    std::shared_ptr<Shader> chessboardShader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/calibration/chessboard.vert", CMAKE_SOURCE_DIR "/shader/calibration/chessboard.frag");
    std::shared_ptr<Shader> debugTextShader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/calibration/debugText.vert", CMAKE_SOURCE_DIR "/shader/calibration/debugText.frag");

    std::shared_ptr<Mesh> quadMesh;

    int projectorID = 0;

    int minBound = 0;
    int maxBound = 255;

    int currentCameraID = -1;

    // Found intensity values:
    std::vector<std::pair<uchar, uchar>> intensityValues;

    IntensityFinder(int projectorID) : projectorID(projectorID){
        std::vector<Triangle> triangles;
        triangles.push_back(Triangle(Vertex(Vec4f(-1.0f, -1.0f, 0.f)), Vertex(Vec4f(1.0f, -1.0f, 0.f)), Vertex(Vec4f(1.0f, 1.0f, 0.f))));
        triangles.push_back(Triangle(Vertex(Vec4f(-1.0f, -1.0f, 0.f)), Vertex(Vec4f(1.0f, 1.0f, 0.f)), Vertex(Vec4f(-1.0f, 1.0f, 0.f))));
        quadMesh = std::make_shared<Mesh>(triangles);
    }

    void renderChessboard(float x, float y, int projectorId, Vec4f clearColor = Vec4f(0.0f, 0.0f, 0.0f, 1.0f)){
        int i=0;
        for(std::shared_ptr<Projector>& projector : Data::instance.projectors){
            if(projector != nullptr){
                int imageWidth = 3840;
                int imageHeight = 2160;
                projector->imageFBO.bind(imageWidth, imageHeight);
                glViewport(0, 0, imageWidth, imageHeight);

                glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Render Point
                if(projectorId == i){
                    chessboardShader->bind();
                    chessboardShader->setUniform("margin", 100);
                    chessboardShader->setUniform("fieldColor", Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
                    chessboardShader->setUniform("fieldSize", 200);
                    chessboardShader->setUniform("xCount", 7);
                    chessboardShader->setUniform("yCount", 5);
                    chessboardShader->setUniform("screenWidth", imageWidth);
                    chessboardShader->setUniform("screenHeight", imageHeight);
                    chessboardShader->setUniform("positionX", x);
                    chessboardShader->setUniform("positionY", y);
                    quadMesh->render();
                }
            }
            ++i;
        }


        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    virtual void process(std::vector<std::shared_ptr<OrganizedPointCloud>>& pointClouds) override {
        renderChessboard(0.5f, 0.5f, projectorID);

        bool init = false;

        for(int i = intensityValues.size(); i < pointClouds.size(); ++i){
            intensityValues.push_back(std::pair<uchar, uchar>(0,0));
            init = true;
        }

        if(currentCameraID == -1 || intensityValues[currentCameraID].second > 0){
            ++currentCameraID;

            for(int i = 0; i < pointClouds.size(); ++i){
                bool shouldEnable = i == currentCameraID;
                if(shouldEnable && pointClouds[i]->camera->type() == "Orbbec_Femto"){
                    pointClouds[i]->camera->enableIRLight();
                } else {
                    pointClouds[i]->camera->disableIRLight();
                }
            }
        }

        if(currentCameraID < pointClouds.size()){
            std::shared_ptr<OrganizedPointCloud> current = pointClouds[currentCameraID];
            cv::Mat grayscale = cv::Mat(current->height, current->width, CV_8UC1);
            cv::Mat rgb = cv::Mat(current->height, current->width, CV_8UC4, current->colors);
            cv::cvtColor(rgb, grayscale, cv::COLOR_RGBA2GRAY);

            double min_val = minBound;
            double max_val = maxBound;

            if(intensityValues[currentCameraID].second == 0){
                min_val = minBound += 2;
                max_val = maxBound;

                if(minBound >= maxBound)
                    minBound = 0;
            }

            cv::Mat normalizedGrayscale = (grayscale - min_val) * (255.0 / (max_val - min_val));

            cv::Size patternsize(6,4);
            std::vector<cv::Point2f> corners;

            bool patternfound = cv::findChessboardCornersSB(normalizedGrayscale, patternsize, corners);

            if(patternfound){
                uchar minVal = 255;
                uchar maxVal = 0;

                for(cv::Point2f corner : corners){
                    std::cout << corner << std::endl;
                    if(corner.y > 5 && corner.y < current->height - 5 && corner.x > 5 && corner.x < current->width - 5){
                        for(int y = int(corner.y - 3); y < corner.y + 3; ++y){
                            for(int x = int(corner.x - 3); x < corner.x + 3; ++x){
                                uchar pixVal = normalizedGrayscale.at<uchar>(y,x);

                                if(pixVal > 0){
                                    minVal = std::min(pixVal, minVal);
                                    maxVal = std::max(pixVal, maxVal);
                                }
                            }
                        }
                    }
                }

                int foundMin = (minVal * ((maxBound - minBound) / 255.0)) + minBound;
                int foundMax = (maxVal * ((maxBound - minBound) / 255.0)) + minBound;

                std::cout << "MinBound: " << foundMin << " | MaxBound: " << foundMax << std::endl;

                intensityValues[currentCameraID].first = foundMin;
                intensityValues[currentCameraID].second = foundMax;
            }

            cv::drawChessboardCorners(normalizedGrayscale, patternsize, cv::Mat(corners), patternfound);
            cv::imshow("cvImage", normalizedGrayscale);
        }
    }


    virtual bool isFinished() override {
        if(intensityValues.size() == 0)
            return false;

        for(std::pair<uchar,uchar>& p : intensityValues){
            if(p.second == 0)
                return false;
        }

        renderChessboard(-1, -1, projectorID);

        return true;
    }
};
