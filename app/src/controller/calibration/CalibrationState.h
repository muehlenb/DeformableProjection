// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include <vector>
#include <opencv2/opencv.hpp>

#include "src/processing/OrganizedPointCloud.h"

/**
 * Finds the projection area and checks how the minimal and maximal
 * brightness of a projector are perceived in the cameras image.
 */

class CalibrationState {
    virtual void process(std::vector<std::shared_ptr<OrganizedPointCloud>>& pointClouds) = 0;
    virtual bool isFinished() = 0;
};
