#pragma once

#include "src/simulation/scene/components/Projector.h"
#include "src/processing/devices/RGBDCameraManager.h"
#include "src/Data.h"

#include <GLFW/glfw3.h>

void RGBDCameraManager::save(const std::string filename) {
    nlohmann::json j;
    nlohmann::json projectorsJson;
    for (const auto& projector : Data::instance.projectors) {
        projectorsJson.push_back({
            {"projectionMatrix", projector->projectionMatrix.data},
            {"transformation", projector->transformation.data}
        });
    }
    j["projectors"] = projectorsJson;

    nlohmann::json depthSensorsJson;
    for (const auto& camera : cameras) {
        nlohmann::json camJson;

        camJson["transformation"] = camera->transformation.data;
        camJson["serial"] = camera->getSerial();
        depthSensorsJson.push_back(camJson);
    }
    j["depthSensors"] = depthSensorsJson;

    nlohmann::json projectorMonitorAssignmentJson;
    for (const auto& [projectorId, monitorID] : Data::instance.projectorIdMonitorIdMap) {
        projectorMonitorAssignmentJson.push_back({
            {"projectorID", projectorId},
            {"monitorID", monitorID}
        });
    }

    if(!projectorMonitorAssignmentJson.empty())
        j["projectorMonitorAssignment"] = projectorMonitorAssignmentJson;

    std::ofstream file(filename);
    file << j.dump(4); // Schön formatiert speichern
}

void RGBDCameraManager::load(const std::string filename) {
    std::ifstream file(filename);
    if (!file) return;

    nlohmann::json j;
    file >> j;

    auto& projectors = Data::instance.projectors;

    if (j.contains("projectors")) {
        for (size_t i = 0; i < j["projectors"].size() && i < projectors.size(); ++i) {
            std::cout << "Load projector " << i << std::endl;

            projectors[i]->projectionMatrix = Mat4f(j["projectors"][i]["projectionMatrix"].get<std::vector<float>>());
            projectors[i]->transformation = Mat4f(j["projectors"][i]["transformation"].get<std::vector<float>>());
        }
    }

    if (j.contains("depthSensors")) {
        std::map<std::string, Mat4f> sensorMap;

        for (const auto& entry : j["depthSensors"]) {
            std::string serial = entry["serial"];
            std::vector<float> vec = entry["transformation"].get<std::vector<float>>();
            sensorMap.emplace(serial, Mat4f(vec));
        }

        for(auto& camera : cameras){
            if(sensorMap.count(camera->getSerial())){
                std::cout << "Found Sensor " << camera->getSerial() << std::endl;
                camera->transformation = sensorMap[camera->getSerial()];
            }
        }
    }

    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);

    if (j.contains("projectorMonitorAssignment")) {
        for (const auto& item : j["projectorMonitorAssignment"]) {
            int projectorID = item["projectorID"];
            int monitorID = item["monitorID"];

            std::cout << "Assign projector " << projectorID << " to monitor " << monitorID << std::endl;

            if(monitorID < count){
                Data::instance.projectorMonitorMap.insert_or_assign(projectors[projectorID], monitors[monitorID]);
                Data::instance.projectorIdMonitorIdMap.insert_or_assign(projectorID, monitorID);
            }
        }
    }
}
