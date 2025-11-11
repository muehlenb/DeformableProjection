// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Yaroslav Purgin, Andre Muehlenbrock


// Include this file only once when compiling:
#pragma once

// Include the GUI:
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// Include functions to display Mat4f and Vec4f in the GUI:
#include <imgui_cg1_helpers.h>

// Include Data Struct:
#include "src/Data.h"
#include "src/simulation/scene/components/Projector.h"
#include "src/simulation/scene/components/VirtualRGBDCamera.h"

#include "src/ui/PipelineVisualization.h"

// Include Camera:
#include "src/simulation/scene/Camera.h"

#include <functional>

#include "GLFW/glfw3.h"

class GUI {
public:
    /**
     * Function that draws the GUI.
     */
    static void drawGui() {
        // Create Window with settings:
        ImVec2 guiPosition = ImVec2(static_cast<float>(Data::instance.display_w), 0);
        if (ImGuiConfigFlags_ViewportsEnable) {
            guiPosition += ImGui::GetMainViewport()->Pos;
        }
        ImGui::SetNextWindowPos(guiPosition);
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(Data::instance.menuWidth), static_cast<float>(Data::instance.display_h)));
        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        ImGui::Text("Main / OpenGL-Thread: %.1f ms", Data::instance.timingFrame);

        // Reset camera button:
        if (ImGui::Button("Reset Camera")) {
            Data::instance.camera = std::make_shared<Camera>();
        }

        ImGui::Separator();
        ImGui::Text(" ");
        ImGui::Separator();
        ImGui::Text("Performance Settings");
        ImGui::Separator();
        static const char* labels[] = { "Off", "Very high", "Medium", "Low" };
        ImGui::SliderInt("Mesh Res.", &Data::instance.rectificationMeshStride, 1, 3, labels[Data::instance.rectificationMeshStride]);
        ImGui::Separator();

        auto DrawResButton = [](const char* label, int w, int h)
        {
            bool disabled = (Data::instance.projectorImageWidth == w);
            if (disabled) ImGui::BeginDisabled(true);
            if (ImGui::Button(label))
            {
                Data::instance.projectorImageWidth  = w;
                Data::instance.projectorImageHeight = h;
            }
            if (disabled) ImGui::EndDisabled();
        };
        ImGui::Text("Resolution of Projectors: ");
        DrawResButton("3840x2160", 3840, 2160);
        ImGui::SameLine();
        DrawResButton("1920x1080", 1920, 1080);
        ImGui::SameLine();
        DrawResButton("1280x720",  1280,  720);

        ImGui::Separator();
        ImGui::Text(" ");
        ImGui::Separator();

        ImGui::Text("Calibration");
        ImGui::Separator();
        ImGui::SliderInt("Projectors", &Data::instance.projectorNumToCalibrate, 1, 3);
        ImGui::SliderInt("Passes", &Data::instance.calibrationPasses, 1, 3);
        ImGui::Separator();
        if(Data::instance.isCalibrating){
            ImGui::Text(("State: " + Data::instance.calibrationStateInfo).c_str());
        } else {
            if (ImGui::Button("Calibrate")) {
                Data::instance.isCalibrating = true;
            }
        }
        ImGui::Separator();
        ImGui::Text(" ");
        ImGui::Separator();
        ImGui::Text("Rectification");
        ImGui::Separator();
        ImGui::Checkbox("Follow Camera", &Data::instance.rectifyFollowCam);
        //ImGui::SliderFloat("Follow Y Rotation Factor", &Data::instance.rectifyYRotationFactor, 0.f, 1.0f);
        ImGui::Separator();
        ImGui::Text(" ");
        ImGui::Separator();
        ImGui::Text("Virtual Display");
        ImGui::Separator();
        ImGui::SliderFloat("Height", &Data::instance.virtualDisplayHeight, 0.5f, 1.5f);

        ImGui::Separator();
        ImGui::Text(" ");
        ImGui::Separator();
        ImGui::Text("Projection Surface");
        ImGui::Separator();
        ImGui::Checkbox("Plane In Scene", &Data::instance.isProjectionPlaneInScene);
        ImGui::Checkbox("Display Plane", &Data::instance.showProjectionPlane);
        ImGui::Checkbox("Simulate Folds", &Data::instance.projectionPlaneSimulateFolds);
        ImGui::Checkbox("Simulate Wind", &Data::instance.projectionPlaneSimulateWind);
        ImGui::Separator();
        ImGui::SliderFloat("Y Offset", &Data::instance.projectionPlanePositionOffset.y, 0.0f, 0.05f);
        ImGui::SliderFloat("Bump", &Data::instance.projectionPlaneBumpIntensity, -0.1f, 0.1f);

        ImGui::Separator();
        ImGui::Text(" ");

        // Display projectorSettings:
        showProjectors();

        ImGui::Separator();
        ImGui::Text(" ");
        // Display rendered RGBD images:
        showRGBDCameras();

        ImGui::Separator();
        ImGui::Text(" ");
        ImGui::Separator();
        // Display other settings:
        otherSettings();
        ImGui::Separator();

        ImGui::Separator();

        ImGui::End();
    }
private:
    /** Custom popup with a close button */
    static void ShowCustomPopup(const std::string& popupName, const std::function<void()>& customContent) {
        bool popup_open = true;
        if (ImGui::BeginPopupModal(popupName.c_str(), &popup_open, ImGuiWindowFlags_AlwaysAutoResize)) {

            // Call the custom content function:
            if (customContent) {
                customContent();
            }

            // Handle closing via Esc key or button press:
            if (ImGui::IsKeyPressed(ImGuiKey_Escape) || !popup_open) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    /**
     * Displays settings for the projectors.
     */
    static void showProjectors() {
        ImGui::Separator();
        ImGui::Text("Projectors");
        ImGui::Separator();
        ImGui::SliderFloat("Intensity", &Data::instance.projectorIntensity, 0.f, 10.f);
        ImGui::SliderFloat("Min Brightness", &Data::instance.projectorMinBrightness, 0.f, 0.1f);
        ImGui::Checkbox("Project Only Color of Projector ID", &Data::instance.projectOnlyProjectorIDColor);
        ImGui::Checkbox("Ignore Shadow Avoidance", &Data::instance.ignoreShadowAvoidance);
        ImGui::Checkbox("Tile-based Shadow Avoidance", &Data::instance.tileBasedShadowAvoidance);

        ImGui::Separator();
        const float remainingWidth = ImGui::GetContentRegionAvail().x;

        // Draw settings for each projector:
        for (int projectorID = 0; projectorID < Data::instance.projectors.size(); projectorID++) {
            std::shared_ptr<Projector> projector = Data::instance.projectors[projectorID];
            std::string uiProjectorID = std::to_string(projectorID + 1);

            // Projector active:
            ImGui::Checkbox(("##Projector" + uiProjectorID).c_str(), &projector->active);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Active\nRight click to set as only active");

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    // Set only the clicked projector as active:
                    for (const std::shared_ptr<Projector>& _projector : Data::instance.projectors) {
                        _projector->active = _projector == projector;
                    }
                }
            }
            // Collapsable section:
            ImGui::SameLine();
            std::string sectionName = "Projector " + uiProjectorID;
            if (!ImGui::CollapsingHeader(sectionName.c_str(), ImGuiTreeNodeFlags_DefaultOpen )) {
                continue;
            }

            // For the later display of the projected image:
            const float imageWidth = static_cast<float>(Data::instance.menuWidth) * 0.88f;
            const float imageHeight = imageWidth / 1920.f * 1080.f;

            // The outline for the content of the collapsable section:
            std::string childName = "##ProjectorContentChild " + uiProjectorID;
            float childHeight = 70 + (projector->active ? imageHeight : 0);
            ImGui::BeginChild(childName.c_str(), ImVec2(0, childHeight), true, ImGuiWindowFlags_AlwaysUseWindowPadding);

            // FOV input section:
            ImGui::PushItemWidth(remainingWidth * 0.48f);
            ImGui::PopItemWidth();

            if (!projector->active) {
                ImGui::Text("No Projected Image");
                ImGui::EndChild(); 
                continue;
            }

            // --- Dropdown --- //
            if (auto& selectedMonitor = Data::instance.projectorMonitorMap[projector];
                ImGui::BeginCombo("Select Monitor", selectedMonitor ? glfwGetMonitorName(selectedMonitor) : "Select..."))
            {
                // Get the list of monitors:
                int count = 0;
                GLFWmonitor** monitors = glfwGetMonitors(&count);

                // Add "None" as the first selectable option:
                const bool isNoneSelected = (selectedMonitor == nullptr);
                if (ImGui::Selectable("None", isNoneSelected)) {
                    Data::instance.projectorMonitorMap.erase(projector);
                    Data::instance.projectorIdMonitorIdMap.erase(projectorID);
                    ImGui::SetItemDefaultFocus();
                }

                // All selectable monitors:
                for (int n = 0; n < count; n++) {
                    const bool isMonitorSelected = (monitors[n] == selectedMonitor);

                    bool monitorInUse = false;
                    for (const auto& [_, monitor] : Data::instance.projectorMonitorMap) {
                        if (isMonitorSelected) break;
                        if (monitorInUse = monitor == monitors[n]) break;
                    }

                    std::string monitorNameWithIndex = std::to_string(n + 1) + ": " + std::string(glfwGetMonitorName(monitors[n]));
                    if (ImGui::Selectable(monitorNameWithIndex.c_str(), isMonitorSelected, monitorInUse ? ImGuiSelectableFlags_Disabled : ImGuiSelectableFlags_None)) {
                        selectedMonitor = monitors[n];
                        Data::instance.projectorMonitorMap.insert_or_assign(projector, selectedMonitor);
                        Data::instance.projectorIdMonitorIdMap.insert_or_assign(projectorID, n);
                    }

                    // Focus if was selected in this frame or already has a window (was selected before):
                    if (isMonitorSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // --- Image --- //

            ImGui::Text("Projected Image:");

            GLuint textureID = projector->getTexture().texture;

            // Display the image, with custom UV to mirror vertically:
            ImGui::Image(textureID, ImVec2(imageWidth, imageHeight), ImVec2(0, 1), ImVec2(1, 0));
            
            // Image clickable for a larger view:
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Click to view");

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    // Need to end child before opening the popup:
                    ImGui::EndChild();
                    std::string popupName = "Large Image View from Projector " + uiProjectorID;
                    ImGui::OpenPopup(popupName.c_str());
                    Data::instance.selectedProjector = projectorID;
                    continue;
                }
            }

            ImGui::EndChild();
        }

        // Popup window to show the larger image.
        // Outside the for-loop, since only one projector can be selected:
        std::string popupName = "Large Image View from Projector " + std::to_string(Data::instance.selectedProjector + 1);
        ShowCustomPopup(popupName, [&]() {
            if (Data::instance.selectedProjector == -1) { return; }

            // Retrieve the data for the selected projector:
            auto projector = Data::instance.projectors[Data::instance.selectedProjector];
            const GLuint textureID = projector->getTexture().texture;

            float largeImageWidth = 1920.f;
            float largeImageHeight = 1080.f;

            if (1920.f * 2.f > static_cast<float>(Data::instance.display_w)) {
                largeImageWidth = std::max(240.0f, static_cast<float>(Data::instance.display_w) * 0.8f);
                largeImageHeight = largeImageWidth * 1080.f / 1920.f;
            }

            ImGui::Image(textureID, ImVec2(largeImageWidth, largeImageHeight), ImVec2(0, 1), ImVec2(1, 0));
        });
    }

    /**
     * Displays images from RGBD cameras and associated settings.
     */
    static void showRGBDCameras() {
        ImGui::Separator();
        ImGui::Text("RGBD Cameras");
        ImGui::Separator();
        ImGui::Checkbox("Render Point Cloud", &Data::instance.renderRawPointCloud);
        ImGui::Checkbox("Simulate Noise", &Data::instance.simulateRGBDNoise);
        ImGui::Checkbox("Show Color", &Data::instance.colorDistanceToggle);
        ImGui::Separator();

        for (int cameraID = 0; cameraID < Data::instance.rgbdCameras.size(); cameraID++) {
            std::shared_ptr<VirtualRGBDCamera> rgbdCam = Data::instance.rgbdCameras[cameraID];
            std::string uiCameraID = std::to_string(cameraID + 1);

            // Show cloud:
            ImGui::Checkbox(("##RGBD Camera show cloud" + uiCameraID).c_str(), &rgbdCam->active);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("Show point cloud\nRight click to only show the cloud of RGBD Camera %i", cameraID + 1);

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    // Show only the clicked camera's cloud:
                    for (const std::shared_ptr<VirtualRGBDCamera>& _rgbdCam : Data::instance.rgbdCameras) {
                        _rgbdCam->active = _rgbdCam == rgbdCam;
                    }
                }
            }
            // Collapsable section:
            ImGui::SameLine();
            const std::string sectionName = "RGBD Camera " + uiCameraID;
            if (!ImGui::CollapsingHeader(sectionName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                continue;
            }

            if (!rgbdCam->active) { continue; }

            const float imageWidth = static_cast<float>(Data::instance.menuWidth) * 0.88f;
            const float imageHeight = imageWidth / (float)rgbdCam->width * (float)rgbdCam->height;

            std::string childName = "##RGBDContentChild " + uiCameraID;
            ImGui::BeginChild(childName.c_str(), ImVec2(0, imageHeight + 20), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
            
            // Calculate available space:
            if(rgbdCam->getFBO().getName() != 0){
                GLuint textureID;
                if (Data::instance.colorDistanceToggle) {
                    textureID = rgbdCam->getColorTexture2D().texture;
                } else {
                    textureID = rgbdCam->getDataTexture2D().texture;
                }
                // Display the image, with custom UV to mirror vertically:
                ImGui::Image(textureID, ImVec2(imageWidth, imageHeight), ImVec2(0, 1), ImVec2(1, 0));
            }

            ImGui::EndChild();

            // Image clickable for a larger view:
            if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)){
                continue;
            }

            ImGui::SetTooltip("Click to view");
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                std::string popupName = "Large Image View from RGBD Camera " + uiCameraID;
                ImGui::OpenPopup(popupName.c_str());
                Data::instance.selectedRGBDCamera = cameraID;
            }

        }
        
        // Popup window to show the larger image:
        std::string popupName = "Large Image View from RGBD Camera " + std::to_string(Data::instance.selectedRGBDCamera + 1);
        ShowCustomPopup(popupName, [&]() {
            if (Data::instance.selectedRGBDCamera == -1) {  return; }

            // Retrieve the data for the selected RGBD camera:
            auto rgbdCam = Data::instance.rgbdCameras[Data::instance.selectedRGBDCamera];
            const GLuint colorTextureID = rgbdCam->getColorTexture2D().texture;
            const GLuint dataTextureID = rgbdCam->getDataTexture2D().texture;

            // Display the larger image:
            float largeImageWidth = static_cast<float>(rgbdCam->width);
            float largeImageHeight = static_cast<float>(rgbdCam->height);

            if (rgbdCam->width * 2 > Data::instance.display_w) {
                largeImageWidth = std::max(240.0f, (float)Data::instance.display_w * 0.5f);
                largeImageHeight = largeImageWidth * (float)rgbdCam->height / (float)rgbdCam->width;
            }

            ImGui::Image(colorTextureID, ImVec2(largeImageWidth, largeImageHeight), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::SameLine();
            ImGui::Image(dataTextureID, ImVec2(largeImageWidth, largeImageHeight), ImVec2(0, 1), ImVec2(1, 0));
        });
    }

    /**
     * Displays other miscellaneous settings.
     */
    static void otherSettings() {
        ImGui::Text("Other Settings");

        if (!ImGui::CollapsingHeader("Post-Processing", ImGuiTreeNodeFlags_None)) {
            return;
        }

        ImGui::DragFloat("Saturation", &Data::instance.saturation, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Contrast", &Data::instance.contrast, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Gamma", &Data::instance.gamma, 0.1f, 0.1f, 10.0f);
    }
};
