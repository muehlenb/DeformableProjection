// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include "src/math/Mat4.h"

/**
 * Represents a camera in the scene
 */
class Camera {
    Vec4f viewTarget = Vec4f(0,1.2f,0);
    float viewDistance = 0.35;

    /** Rotation around X axis */
    float viewRotationX = 0.5 * M_PI;//-0.9f;

    /** Rotation around Y axis */
    float viewRotationY = -1.35;

    /** Precalculated rotation matrix from viewRotationX and viewRotationY */
    Mat4f rotation;

    // Previous mouse position:
    ImVec2 lastMousePosition = ImGui::GetMousePos();

public:
    Camera(){
        rotateCamera(Vec4f());
    }

    /** Processes the ImGui input, e.g. mouse input */
    void processImGuiInput(){
        // Rotate camera:
        if(ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse) {
            ImVec2 mouseDelta = ImGui::GetMousePos() - lastMousePosition;
            rotateCamera(Vec4f(mouseDelta.x, mouseDelta.y, 0));
        }

        // Move Target:
        if(ImGui::IsMouseDown(ImGuiMouseButton_Middle) && !ImGui::GetIO().WantCaptureMouse) {
            ImVec2 mouseDelta = ImGui::GetMousePos() - lastMousePosition;
            moveTarget(Vec4f(mouseDelta.x, mouseDelta.y, 0));
        }

        // Scale distance to view target:
        if(ImGui::GetIO().MouseWheel != 0){
            if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
                scaleDistance(ImGui::GetIO().MouseWheel > 0 ? 0.9f : 1.1f);
            }
        }

        // Store this mouse position for the next frame:
        lastMousePosition = ImGui::GetMousePos();
    }

    /** Changes the distance to the viewTarget */
    void scaleDistance(float multiplier){
        viewDistance *= multiplier;
    }

    /** Rotates the camera by a given screen coordinate delta */
    void rotateCamera(Vec4f mouseDeltaXY){
        viewRotationX -= mouseDeltaXY.x * 0.01f;
        viewRotationY -= mouseDeltaXY.y * 0.01f;

        // Precalculate rotation matrix:
        rotation = Mat4f({
            cos(viewRotationX), -sin(viewRotationY) * -sin(viewRotationX), cos(viewRotationY) * -sin(viewRotationX), 0,
            0, cos(viewRotationY), sin(viewRotationY), 0,
            sin(viewRotationX), -sin(viewRotationY) * cos(viewRotationX), cos(viewRotationY) * cos(viewRotationX), 0,
            0,0,0,1
        });
    }

    /** Moves the viewTarget by a given screen coordinate delta */
    void moveTarget(Vec4f mouseDeltaXY){
        Mat4f invRotationMat = rotation.inverse();
        Vec4f xDir = invRotationMat * Vec4f(1, 0, 0, 0);
        Vec4f yDir = invRotationMat * Vec4f(0, 1, 0, 0);

        viewTarget = viewTarget + (-xDir * mouseDeltaXY.x * 0.002f * viewDistance) + (yDir * mouseDeltaXY.y * 0.002f * sqrt(viewDistance));
    }

    /** Returns the projection matrix for this camera */
    Mat4f getProjection(int display_w, int display_h){
        return Mat4f::perspectiveTransformation(float(display_w) / float(display_h));
    };

    /** Returns the view matrix for this camera */
    Mat4f getView(){
        return Mat4f::translation(0, 0, viewDistance) * rotation * Mat4f::translation(-viewTarget.x, -viewTarget.y, -viewTarget.z);
    };
};
