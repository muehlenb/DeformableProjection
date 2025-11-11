// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include<vector>

#include "src/processing/devices/orbbec/OrbbecCameraProvider.h"

#include "src/processing/devices/RGBDCamera.h"
#include "src/processing/OrganizedPointCloud.h"

#include <nlohmann/json.hpp>

class RGBDCameraManager {
    /** Same order as cameras */
    std::mutex pointCloudsMutex;

    std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds;
    std::vector<std::shared_ptr<RGBDCamera>> cameras;

    std::vector<std::function<void(int deviceIndex, std::shared_ptr<OrganizedPointCloud> pointCloud)>> pointCloudCallbacks;

    // When no real camera is found, switch to simulation mode:
    bool simulationMode = false;

public:
    std::vector<std::shared_ptr<OrganizedPointCloud>> getCurrentPointClouds(){
        std::unique_lock l(pointCloudsMutex);
        std::vector<std::shared_ptr<OrganizedPointCloud>> copy = pointClouds;
        return copy;
    }

    /** */
    RGBDCameraManager(){
        std::cout << "Searching for Orbbec Camera Devices..." << std::endl;
        OrbbecCameraProvider orbbecProvider;
        std::vector<std::shared_ptr<OrbbecCamera>> orbbecCameras = orbbecProvider.getCameras(0, [this](int id, std::shared_ptr<OrganizedPointCloud> cloud) {
            pointCloudCallback(id, cloud);
        });
        for(std::shared_ptr<OrbbecCamera>& orbbecCam : orbbecCameras){
            cameras.push_back(orbbecCam);
        }
        std::cout << "    Found " << orbbecCameras.size() << " Cameras." << std::endl << std::endl;

        std::cout << "Start Cameras... " << std::endl;
        for(std::shared_ptr<RGBDCamera>& cam : cameras){
            cam->start();
        }
        std::cout << "Started " << cameras.size() << " cameras." << std::endl;

        // If no camera is connected, put virtual ones into the world:
        if(cameras.size() == 0){
            simulationMode = true;
        }
    }

    void pointCloudCallback(int deviceIndex, std::shared_ptr<OrganizedPointCloud> pointCloud){
        for(std::function<void(int deviceIndex, std::shared_ptr<OrganizedPointCloud> pointCloud)>& callback : pointCloudCallbacks){
            if(callback)
                callback(deviceIndex, pointCloud);
        }

        std::unique_lock l(pointCloudsMutex);
        while(pointClouds.size() <= deviceIndex)
            pointClouds.push_back(nullptr);

        pointClouds[deviceIndex] = pointCloud;
    }

    void registerCallback(std::function<void(int deviceIndex, std::shared_ptr<OrganizedPointCloud> pointCloud)> pointCloudCallback){
        pointCloudCallbacks.push_back(pointCloudCallback);
    }

    bool requiresSimulatedRGBDData(){
        return simulationMode;
    }

    void save(const std::string filename = "config.json");
    void load(const std::string filename = "config.json");

};
