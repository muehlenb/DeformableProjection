// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#define MAX_PROJECTOR_NUM 3

#include "src/Data.h"
#include "src/simulation/scene/components/Projector.h"

#include "src/controller/calibration/DepthReconstructor.h"
#include "src/controller/calibration/IntensityFinder.h"
#include "src/controller/calibration/CheckerboardCollector.h"
#include "src/controller/calibration/MarkerAligner.h"
#include "src/controller/calibration/Calibrator.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

/**
 * This class manages the overall calibration process from start to end
 * and switches between it.
 */
class CalibrationManager
{
    MarkerAligner markerAligner;

    // Intensity Finder and Checkerboard Collectors per projector:
    DepthReconstructor depthReconstructor;
    std::vector<IntensityFinder> intensityFinders;
    std::vector<CheckerboardCollector> checkerboardCollectors;

    Calibrator calibrator;

    std::shared_ptr<Shader> calibrationShader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/calibration/point.vert", CMAKE_SOURCE_DIR "/shader/calibration/point.frag");
    std::shared_ptr<Mesh> quadMesh;

    int projectorNumToCalibrate = -1;
    int collectingPassesPerProjector = 1;

    /**
     * State:
     * 0: Start
     * 1: Flicker-Initialisation
     *  - Substate flickers between on off
     * 2: Pause
     * 3: Checkerboard Detection
     *
     */

    int state = 0;
    int collectorsState = 0;
    int substate = 1;
    int collectedChessboardsPP = 0;

    void switchToState(int val){
        state = val;
        substate = 0;
        std::cout << "Switched to STATE " << val << std::endl;
    }

public:
    CalibrationManager()
    {
        std::vector<Triangle> triangles;
        triangles.push_back(Triangle(Vertex(Vec4f(-1.0f, -1.0f, 0.f)), Vertex(Vec4f(1.0f, -1.0f, 0.f)), Vertex(Vec4f(1.0f, 1.0f, 0.f))));
        triangles.push_back(Triangle(Vertex(Vec4f(-1.0f, -1.0f, 0.f)), Vertex(Vec4f(1.0f, 1.0f, 0.f)), Vertex(Vec4f(-1.0f, 1.0f, 0.f))));
        quadMesh = std::make_shared<Mesh>(triangles);

        for(int i=0; i < MAX_PROJECTOR_NUM; ++i){
            intensityFinders.emplace_back(i);
            checkerboardCollectors.emplace_back(i);
        }
    }

    /**
     * @brief tickCollectorsPass
     */
    void tickCollectorsPass(){
        int modCollectorsState = collectorsState % 5;
        int projectorID = collectorsState / 5;

        std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds = Data::instance.cameraManager.getCurrentPointClouds();

        if(modCollectorsState == 0){
            Data::instance.calibrationStateInfo = "Reconstructing smoothed surfaces.";
            depthReconstructor.process(pointClouds);

            if(depthReconstructor.isFinished()){
                ++collectorsState;
                substate = 0;
                Data::instance.calibrationStateInfo = "";
                checkerboardCollectors[projectorID].reconstructedPointClouds = depthReconstructor.smoothedPointCloud;
            }
        } else if(modCollectorsState == 1){
            intensityFinders[projectorID].process(pointClouds);

            if(intensityFinders[projectorID].isFinished()){
                checkerboardCollectors[projectorID].intensityRanges = intensityFinders[projectorID].intensityValues;
                ++collectorsState;
                substate = 0;
                Data::instance.calibrationStateInfo = "";
            }
        } else if(modCollectorsState == 2){
            if(substate > 30){
                for(int i = 0; i < pointClouds.size(); ++i){
                    if(pointClouds[i]->camera->type() == "Orbbec_Femto"){
                        pointClouds[i]->camera->enableIRLight();
                    } else {
                        pointClouds[i]->camera->disableIRLight();
                    }
                }

                ++collectorsState;
                substate = 0;
                Data::instance.calibrationStateInfo = "Find Checkerboards for Projector "+std::to_string(projectorID);
            }
            ++substate;
        } else if(modCollectorsState == 3){
            checkerboardCollectors[projectorID].process(pointClouds);

            if(checkerboardCollectors[projectorID].isFinished()){
                ++collectorsState;
                substate = 0;
                Data::instance.calibrationStateInfo = "";
            }
        } else if(modCollectorsState == 4){
            ++substate;
            if(substate > 30){
                // If there is a next projector to calibrate, repeat, else switch to next state:
                std::cout << (projectorID + 1) << " < " << projectorNumToCalibrate << std::endl;

                if(projectorID + 1 < projectorNumToCalibrate){
                    ++collectorsState;
                    substate = 0;
                    Data::instance.calibrationStateInfo = "Find Projector "+std::to_string(projectorID+1)+"'s Intensity Range";
                } else {
                    collectorsState = 0;
                    collectedChessboardsPP++;
                    switchToState(state+1);
                    Data::instance.calibrationStateInfo = "";
                }
            }
        }
    }



    /**
     * Is called by the main function, like a tick function of an Unreal actor :-) It manages
     * the calibration functions.
     */
    void tick(){
        if(!Data::instance.isCalibrating)
            return;

        if(state == 0){
            ++substate;
            if(substate > 30){
                collectorsState = 0;
                Data::instance.calibrationStateInfo = "Find Projector 0's Intensity Range";
                switchToState(1);
                startCalibration();
            }
        } else if(state == 1){
            // Collect chessboard image pattern from all projectors:
            tickCollectorsPass();
        } else if(state == 2){
            if(collectedChessboardsPP < collectingPassesPerProjector){
                if(substate == 0){
                    Data::instance.calibrationStateInfo = "Waiting for next Pass.";
                }
                ++substate;
                if(substate > 300){
                    // If projector
                    for(CheckerboardCollector& collector : checkerboardCollectors){
                        collector.nextRound();
                    }
                    depthReconstructor = DepthReconstructor();

                    for(int i=0; i < intensityFinders.size(); ++i){
                        intensityFinders[i] = IntensityFinder(i);
                    }

                    switchToState(1);
                }
            } else {
                ++substate;
                if(substate > 30){
                    switchToState(3);
                    Data::instance.calibrationStateInfo = "Calibrating...";
                }
            }
        } else if(state == 3){
            std::vector<std::pair<std::vector<cv::Point2f>, std::vector<std::vector<Vec4f>>>> collectedCorrespondencePoints;

            for(int i=0; i < projectorNumToCalibrate; ++i){
                collectedCorrespondencePoints.emplace_back(markerAligner.getCorrespondencePoints(checkerboardCollectors[i].foundCheckerboards));
                std::cout << "Checkerboards: " << checkerboardCollectors[i].foundCheckerboards[0].size() << std::endl;
            }

            calibrator.calibrate(collectedCorrespondencePoints);
            endCalibration();

        }
    }

    /**
     * Starts the calibration process.
     *
     * Is called by the GUI.
     */
    void startCalibration(){
        for(std::shared_ptr<Projector>& projector : Data::instance.projectors){
            if(projector != nullptr){
                projector->shouldRenderRectifiedImage = false;
            }
        }

        substate = 0;
        collectorsState = 0;
        collectedChessboardsPP = 0;
        Data::instance.calibrationStateInfo = "Start calibration...";
        projectorNumToCalibrate = Data::instance.projectorNumToCalibrate;
        collectingPassesPerProjector = Data::instance.calibrationPasses;
        Data::instance.isCalibrating = true;
    }

    /**
     * Ends the calibration process and enabled the rendering of rectified images.
     *
     * Is called when calibration finishes automatically or via GUI.
     */
    void endCalibration(){
        for(std::shared_ptr<Projector>& projector : Data::instance.projectors){
            if(projector != nullptr){
                projector->shouldRenderRectifiedImage = true;
            }
        }

        Data::instance.calibrationStateInfo = "";
        Data::instance.isCalibrating = false;

        for(int i=0; i < MAX_PROJECTOR_NUM; ++i){
            checkerboardCollectors[i] = CheckerboardCollector(i);
            intensityFinders[i] = IntensityFinder(i);
        }
        state = 0;
    }
};
