#pragma once

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "src/simulation/scene/components/RectifiedProjection.h"
#include "src/simulation/scene/components/Projector.h"

class PipelineVisualization {
    static bool show_custom_window;
public:
    static void render(){
        if (ImGui::Button("Fenster öffnen"))
        {
            show_custom_window = true;
        }

        // Separates Fenster anzeigen
        if (show_custom_window)
        {

            ImGuiViewport* main_viewport = ImGui::GetMainViewport();

            ImVec2 new_size = ImVec2(main_viewport->Size.x * 0.9f, main_viewport->Size.y * 0.9f);
            ImVec2 new_pos  = ImVec2(main_viewport->Pos.x + main_viewport->Size.x * 0.05f,
                                    main_viewport->Pos.y + main_viewport->Size.y * 0.05f);

            ImGui::SetNextWindowSize(new_size, ImGuiCond_Always);
            ImGui::SetNextWindowPos(new_pos, ImGuiCond_Always);

            CameraPasses& pctextures = CameraPasses::getInstance();
            if (ImGui::Begin("Separates Fenster", &show_custom_window))
            {
                if(RectifiedProjection::instance != nullptr){
                    std::shared_ptr<ShadowAvoidance> avd = RectifiedProjection::instance->shadowAvoidance;

                    for(int i=0; i < 3; ++i){
                        if(pctextures.cameraWidth[i] <= 0)
                            continue;

                        int vertexDistTexID = avd->texture2D_projectorDistanceMap[i];
                        ImGui::SetCursorPos(ImVec2(10, 50 + 300*i));
                        ImGui::Image(vertexDistTexID, ImVec2(1920 / 4, 1080 / 4));
                    }

                    for(int i=0; i < 3; ++i){
                        if(pctextures.cameraWidth[i] <= 0)
                            continue;

                        int vertexDistTexID = pctextures.texture2D_vertexShadowMap[i];
                        ImGui::SetCursorPos(ImVec2(520, 50 + 300*i));
                        ImGui::Image(vertexDistTexID, ImVec2(640 / 2, 576 / 2));
                    }

                    for(int i=0; i < 3; ++i){
                        if(pctextures.cameraWidth[i] <= 0)
                            continue;

                        int vertexDistTexID = pctextures.texture2D_vertexDistanceMap[i];
                        ImGui::SetCursorPos(ImVec2(920, 50 + 300*i));
                        ImGui::Image(vertexDistTexID, ImVec2(640 / 2, 576 / 2));
                    }

                    for(int i=0; i < 3; ++i){
                        if(pctextures.cameraWidth[i] <= 0)
                            continue;

                        int vertexDistTexID = pctextures.texture2D_temporalDistanceMapA[i];
                        ImGui::SetCursorPos(ImVec2(1320, 50 + 300*i));
                        ImGui::Image(vertexDistTexID, ImVec2(640 / 2, 576 / 2));
                    }

                    for(int i=0; i < 3; ++i){
                        if(pctextures.cameraWidth[i] <= 0)
                            continue;

                        int vertexDistTexID = pctextures.texture2D_vertexProjectorAssignment[i];
                        ImGui::SetCursorPos(ImVec2(1720, 50 + 300*i));
                        ImGui::Image(vertexDistTexID, ImVec2(640 / 2, 576 / 2));
                    }
                }

                int i = 0;
                for(std::shared_ptr<Projector> projector : Data::instance.projectors){
                    int vertexDistTexID = projector->imageFBO.getTexture2D(0).texture;
                    ImGui::SetCursorPos(ImVec2(2100, 50 + 300*i));
                    ImGui::Image(vertexDistTexID, ImVec2(1920 / 4, 1080 / 4));
                    ++i;
                }


/*
                std::shared_ptr<BlendPCRShadowAvoidance> avd = RectifiedProjection::instance->blendPCRShadowAvoidance;
                for(int i=0; i < 3; ++i){
                    int texID = avd->texture2D_segmentationShadowDistance[i];

                    ImGui::SetCursorPos(ImVec2(50 + i * 700, 200));
                    ImGui::Image(texID, ImVec2(640, 576));
                }
*/
            }
            ImGui::End();
        }
    }
};
