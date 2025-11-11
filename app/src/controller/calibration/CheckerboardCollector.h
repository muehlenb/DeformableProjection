// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include "src/Data.h"
#include "src/simulation/scene/components/Projector.h"

#include <vector>
#include <opencv2/opencv.hpp>

#define MAX_STATE 16

#include "src/controller/calibration/CalibrationState.h"

struct FoundCheckerboard {
    cv::Point2f cameraImageMid;
    Vec4f cameraMid3D;
    std::vector<cv::Point2f> cameraImagePoints;
    std::vector<cv::Point2f> beamerImagePoints;
    std::vector<Vec4f> cameraPoints3D;
    bool isFound = false;

    FoundCheckerboard(bool isFound)
        : isFound(isFound){
    }
};

struct CheckerboardCollector : public CalibrationState {
    std::shared_ptr<Shader> chessboardShader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/calibration/chessboard.vert", CMAKE_SOURCE_DIR "/shader/calibration/chessboard.frag");
    std::shared_ptr<Mesh> quadMesh;

    // Settings of this Checkerboard Collector:
    int projectorID = 0;
    std::vector<std::pair<uchar, uchar>> intensityRanges;

    std::vector<Vec4f*> reconstructedPointClouds;

    // Found Checkerboards:
    std::vector<std::vector<FoundCheckerboard>> foundCheckerboards;

    // Last found CV corners:
    std::vector<std::vector<cv::Point2f>> lastFoundCVCorners;

    // Checkerboard of current camera found:
    std::vector<bool> wasCheckerboardFound;

    // Checkerboard of current camera found:
    std::vector<bool> lastDistanceCondition;

    int settingTimeout = 15;

    int state = 0;
    int timeout = settingTimeout;

    int chessboard_margin = 50 * 2;
    int chessboard_fieldSize = 100 * 2;
    int chessboard_xCount = 7;
    int chessboard_yCount = 5;

    void nextRound(){
        lastDistanceCondition.clear();
        ++state;
    }

    CheckerboardCollector(int projectorID)
        : projectorID(projectorID)
    {
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
                    chessboardShader->setUniform("margin", chessboard_margin);
                    chessboardShader->setUniform("fieldColor", Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
                    chessboardShader->setUniform("fieldSize", chessboard_fieldSize);
                    chessboardShader->setUniform("xCount", chessboard_xCount);
                    chessboardShader->setUniform("yCount", chessboard_yCount);
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


    Vec4f bilinearInterpolate(Vec4f* smoothedPC, int width, int height, float x, float y) {
        int x0 = floor(x);
        int x1 = x0 + 1;
        int y0 = floor(y);
        int y1 = y0 + 1;

        Vec4f Q11 = smoothedPC[x0 + y0 * width];
        Vec4f Q21 = smoothedPC[x1 + y0 * width];
        Vec4f Q12 = smoothedPC[x0 + y1 * width];
        Vec4f Q22 = smoothedPC[x1 + y1 * width];

        float fx1 = x - x0;
        float fx0 = 1 - fx1;
        float fy1 = y - y0;
        float fy0 = 1 - fy1;

        Vec4f interpolated = Q11 * fx0 * fy0 + Q21 * fx1 * fy0 + Q12 * fx0 * fy1 + Q22 * fx1 * fy1;
        interpolated.w = 1;

        if(interpolated.valid() && interpolated.squaredLength() > 0.01f)
            return interpolated;
        else if(Q11.valid() && Q11.squaredLength() > 0.01f)
            return Q11;
        else if(Q21.valid() && Q21.squaredLength() > 0.01f)
            return Q21;
        else if(Q12.valid() && Q12.squaredLength() > 0.01f)
            return Q12;
        else if(Q22.valid() && Q22.squaredLength() > 0.01f)
            return Q22;

        return Vec4f();
    }


    Vec4f bilinearInterpolate(std::shared_ptr<OrganizedPointCloud> smoothedPC, int width, int height, float x, float y) {
        int x0 = floor(x);
        int x1 = x0 + 1;
        int y0 = floor(y);
        int y1 = y0 + 1;

        Vec4f Q11 = smoothedPC->getPosition(x0, y0);
        Vec4f Q21 = smoothedPC->getPosition(x1, y0);
        Vec4f Q12 = smoothedPC->getPosition(x0, y1);
        Vec4f Q22 = smoothedPC->getPosition(x1, y1);

        float fx1 = x - x0;
        float fx0 = 1 - fx1;
        float fy1 = y - y0;
        float fy0 = 1 - fy1;

        Vec4f interpolated = Q11 * fx0 * fy0 + Q21 * fx1 * fy0 + Q12 * fx0 * fy1 + Q22 * fx1 * fy1;
        interpolated.w = 1;

        if(interpolated.valid() && interpolated.squaredLength() > 0.01f)
            return interpolated;
        else if(Q11.valid() && Q11.squaredLength() > 0.01f)
            return Q11;
        else if(Q21.valid() && Q21.squaredLength() > 0.01f)
            return Q21;
        else if(Q12.valid() && Q12.squaredLength() > 0.01f)
            return Q12;
        else if(Q22.valid() && Q22.squaredLength() > 0.01f)
            return Q22;

        return Vec4f();
    }

    cv::Point2f getCornerImageInBeamerSpace(int i, float chessboardX, float chessboardY){
        int imageWidth = 3840;
        int imageHeight = 2160;

        std::shared_ptr<Projector>& projector = Data::instance.projectors[projectorID];
        int xCornerCount = chessboard_xCount - 1;

        int posX = i % xCornerCount;
        int posY = i / xCornerCount;

        int offsetX = chessboardX * (imageWidth - (2 * chessboard_margin + chessboard_fieldSize * chessboard_xCount));
        int offsetY = chessboardY * (imageHeight - (2 * chessboard_margin + chessboard_fieldSize * chessboard_yCount));

        float cornerX = (posX + 1) * chessboard_fieldSize + chessboard_margin + offsetX;
        float cornerY = (chessboard_fieldSize * chessboard_yCount - (posY + 1) * chessboard_fieldSize + chessboard_margin + offsetY);

        return cv::Point2f(cornerX, cornerY);
    }



    bool findVerifiedChessboardCorners(int deviceIndex, cv::Mat normalizedGrayscale, cv::Size patternsize, std::vector<cv::Point2f>& corners){
        bool found = cv::findChessboardCornersSB(normalizedGrayscale, patternsize, corners, cv::CALIB_CB_ACCURACY);

        if(found){
            if(lastFoundCVCorners[deviceIndex].size() == corners.size() && corners.size() > 0){
                for(int i=0; i < corners.size(); ++i){
                    cv::Point2f p1 = lastFoundCVCorners[deviceIndex][i];
                    cv::Point2f p2 = corners[i];

                    float dist = std::sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));

                    if(dist > 1.f || dist < 0.00001f){
                        lastFoundCVCorners[deviceIndex] = corners;

                       // cv::drawChessboardCorners(normalizedGrayscale, patternsize, cv::Mat(corners), false);
                        //cv::imshow("cvImage "+std::to_string(deviceIndex), normalizedGrayscale);
                      //  cv::waitKey(10);
                        std::cout << "Chessboard REJECTED!" << dist << " | " << p1.x << " x " << p1.y << std::endl;
                        return false;
                    }
                }

                //cv::drawChessboardCorners(normalizedGrayscale, patternsize, cv::Mat(corners), true);
                //cv::imshow("cvImage "+std::to_string(deviceIndex), normalizedGrayscale);
                //cv::waitKey(20);
                std::cout << "Chessboard Accepted!" << std::endl;
                lastFoundCVCorners[deviceIndex] = corners;
                return true;
            }
        }

        lastFoundCVCorners[deviceIndex] = corners;
        return false;
    }

    virtual void process(std::vector<std::shared_ptr<OrganizedPointCloud>>& pointClouds) override{
        int relState = state % 17;

        if(relState % 17 == 16){
            return;
        }

        float chessboardPosX = 0.2f + (relState % 4) * 0.2f;// 0.3333334f; // 0.3f + (state % 4) * 0.13f;
        float chessboardPosY = 0.2f + (relState / 4) * 0.2f;//0.3333334f; // 0.3f + (state / 4) * 0.13f;

        renderChessboard(chessboardPosX, chessboardPosY, projectorID);

        // Ensure that for each camera, an array of checkerboards is available:
        for(int i = foundCheckerboards.size(); i < pointClouds.size(); ++i){
            foundCheckerboards.push_back(std::vector<FoundCheckerboard>());
            wasCheckerboardFound.push_back(false);
            lastDistanceCondition.push_back(false);
            lastFoundCVCorners.push_back(std::vector<cv::Point2f>());

        }

        #pragma omp parallel for
        for(int i = 0; i < pointClouds.size(); ++i){
            if(wasCheckerboardFound[i])
                continue;

            int minVal = intensityRanges[i].first;
            int maxVal = intensityRanges[i].second;

            std::shared_ptr<OrganizedPointCloud> current = pointClouds[i];
            cv::Mat grayscale = cv::Mat(current->height, current->width, CV_8UC1);
            cv::Mat rgb = cv::Mat(current->height, current->width, CV_8UC4, current->colors);
            cv::cvtColor(rgb, grayscale, cv::COLOR_RGBA2GRAY);

            cv::Mat normalizedGrayscale = (grayscale - minVal) * (255.0 / (maxVal - minVal));

            cv::Size patternsize(chessboard_xCount - 1, chessboard_yCount - 1);
            std::vector<cv::Point2f> corners;

            bool patternfound = findVerifiedChessboardCorners(i, normalizedGrayscale, patternsize, corners); //cv::findChessboardCornersSB();

            if(patternfound){
                // Checkerboard pattern:
                FoundCheckerboard checkerboard(true);
                checkerboard.cameraImagePoints = corners;
                checkerboard.cameraMid3D = Vec4f();
                int weight = 0;
                for(cv::Point2f c : corners){
                    Vec4f p;
                    if(pointClouds[i]->camera->type() == "Orbbec_Femto"){
                        p = bilinearInterpolate(pointClouds[i], current->width, current->height, c.x, c.y);
                        std::cout << "Orbbec Femto: Use Current Point Cloud!" << std::endl;
                    } else {
                        p = bilinearInterpolate(reconstructedPointClouds[i], current->width, current->height, c.x, c.y);
                    }
                    checkerboard.cameraPoints3D.push_back(p);

                    p.w = 0;
                    checkerboard.cameraMid3D = checkerboard.cameraMid3D + p;
                    checkerboard.beamerImagePoints.push_back(getCornerImageInBeamerSpace(weight, chessboardPosX, chessboardPosY));
                    ++weight;
                }

                int a = 0;
                for(cv::Point2f p : checkerboard.beamerImagePoints){
                    std::cout << a++ << ": " << p << std::endl;
                }

                checkerboard.cameraMid3D = checkerboard.cameraMid3D / weight;

                bool shouldBeEntered = false;
                if(foundCheckerboards[i].size() == 0){
                    shouldBeEntered = true;
                } else {
                    bool distanceCondition = foundCheckerboards[i].back().cameraMid3D.distanceTo(checkerboard.cameraMid3D) > 0.01;
                    if(lastDistanceCondition[i] && distanceCondition){
                        std::cout << "Last Mid: " << foundCheckerboards[i].back().cameraMid3D << std::endl;
                        std::cout << "Current Mid: " << checkerboard.cameraMid3D << std::endl;
                        shouldBeEntered = true;
                        lastDistanceCondition[i] = false;
                    } else {
                        lastDistanceCondition[i] = distanceCondition;
                    }
                }

                if(shouldBeEntered){
                    wasCheckerboardFound[i] = true;
                    foundCheckerboards[i].push_back(checkerboard);
                }
            }

            //cv::drawChessboardCorners(normalizedGrayscale, patternsize, cv::Mat(corners), patternfound);
            //cv::imshow("cvImage "+std::to_string(i), normalizedGrayscale);
        }

        bool allFound = true;
        for(int i = 0; i < wasCheckerboardFound.size(); ++i){
            allFound &= wasCheckerboardFound[i];
        }

        // When checkerboards for this state where found in each camera,
        // switch to next state:
        if(allFound && relState < MAX_STATE){
            ++state;
            timeout = settingTimeout;

            for(int i=0; i < wasCheckerboardFound.size(); ++i){
                wasCheckerboardFound[i] = false;
                lastDistanceCondition[i] = false;
            }
        }

        // When checkerboards where not found in each camera until timeout
        // switch to next state and enter "empty" checkerboards to ensure
        // same number of checkerboards:
        --timeout;
        if(timeout <= 0){
            for(int i=0; i < wasCheckerboardFound.size(); ++i){
                if(!wasCheckerboardFound[i]){
                    foundCheckerboards[i].push_back(FoundCheckerboard(false));
                }

                wasCheckerboardFound[i] = false;
                lastDistanceCondition[i] = false;
            }

            ++state;
            timeout = settingTimeout;
        }

        // If state is 16, reset beamer image:
        if(relState == MAX_STATE){
            renderChessboard(0,0,-1);
        }
    }

    virtual bool isFinished() override {
        bool isFinished = state % 17 == MAX_STATE;

        if(isFinished)
            renderChessboard(-1,-1, projectorID);

        return isFinished;
    }
};
