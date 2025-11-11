#pragma once

#include <libobsensor/ObSensor.hpp>
#include <immintrin.h> // Für SSE/AVX-Instruktionen

#include <vector>

#include "src/processing/devices/orbbec/OrbbecCamera.h"

/**
 *
 */
struct OrbbecCameraProvider {
    static ob::Context* ctx;

    std::vector<std::shared_ptr<OrbbecCamera>> getCameras(int indexOffset, std::function<void(int, std::shared_ptr<OrganizedPointCloud>)> pointCloudCallback){
        if(ctx == nullptr)
            ctx = new ob::Context();


        std::vector<std::shared_ptr<OrbbecCamera>> result;

        // Query the list of connected devices
        auto devList = ctx->queryDeviceList();

        // Get the number of connected devices
        int devCount = devList->getCount();

        for(int i = 0; i < devCount; i++){
            std::shared_ptr<ob::Device> dev  = devList->getDevice(i);
            std::shared_ptr<ob::Pipeline> pipe = std::make_shared<ob::Pipeline>(dev);

            std::cout << "    Added Camera " << i + indexOffset << "." << std::endl;
            result.push_back(std::make_shared<OrbbecCamera>(pipe, i + indexOffset, pointCloudCallback));
        }

        return result;
    }

    void free(){

    }
};
