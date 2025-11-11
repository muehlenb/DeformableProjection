// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#pragma once

#include <functional>
#include "src/processing/OrganizedPointCloud.h"


/**
 * Represents a single camera, which notifies the Camera Manager when there are
 * new point clouds.
 */
class RGBDCamera {
protected:
    /** Callback function */
    std::function<std::shared_ptr<OrganizedPointCloud>()> callbackFunction;

public:
    /** Returns the responsibility flags */
    virtual int responsibilityFlags() = 0;

    /** Starts the camera thread */
    virtual bool start() = 0;

    /** Activates the camera thread (and the light / light projector) */
    virtual bool enableIRLight() = 0;

    /** Deactivates the camera thread (and the light / light projector) */
    virtual bool disableIRLight() = 0;

    virtual std::string type() = 0;

    virtual std::string getSerial() = 0;

    Mat4f transformation;

    // Just for virtual sensors:
    virtual void receiveSimulatedRGBDData(std::vector<std::shared_ptr<OrganizedPointCloud>> pointClouds){}

    // To detect whether rgb sensors should be simulated in main thread:
    virtual bool requiresSimulatedRGBDData(){ return false; }
};
