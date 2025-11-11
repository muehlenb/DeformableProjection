// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#pragma once

#include <vector>

#include "src/math/Mat4.h"

#include "src/gl/Shader.h"

/** Data from a light source */
struct LightData {
    /** Position of the light, already in world space */
    Vec4f positionWS;

    /** Color and intensity of the light */
    Vec4f color;

    LightData(const Vec4f positionWS, const Vec4f color)
        : positionWS(positionWS)
        , color(color){}
};

/**
 * Defines the current render pass of the scene data.
 *
 * Using this, single components can decide whether they should be
 * rendered or not.
 */
enum RenderPassType {
    SD_MAIN, SD_SIMULATED_CAMERA, SD_NONE
};

/**
 * This struct contains the current SceneData for a single frame
 * and is used to exchange data between individual scene components
 * during the render. For example, each projector enters the current
 * image texture here, and the SurgeryTable can use it to render the
 * projected image onto itself.
 */
struct SceneData {
    SceneData(RenderPassType type)
        : type(type){}

    /** Stores which pass is currently rendered */
    RenderPassType type;

    /** Current projection matrix */
    Mat4f projection;

    /** Current view matrix */
    Mat4f view;

    /** Ambient color of the scene */
    Vec4f ambientColor = Vec4f(0.1f, 0.1f, 0.1f, 1.0f);

    /** List of lights that are active this frame */
    std::vector<LightData> lights;

    /** Function that sets the light data in the shader, since multiple objects use the same lighting model */
    void setLightUniforms(const std::shared_ptr<Shader>& shader) const {
        shader->setUniform("light_num", static_cast<int>(lights.size()));

        for (int i = 0; i < lights.size(); ++i) {
            const LightData& lightData = lights[i];
            std::string lightIdStr = std::to_string(i);
            const Vec4f lightPosCS = view * lightData.positionWS;
            constexpr float range = 15.0f;

            shader->setUniform("lights[" + lightIdStr + "].position", lightPosCS);
            shader->setUniform("lights[" + lightIdStr + "].range", range);
            shader->setUniform("lights[" + lightIdStr + "].color", lightData.color);
        }
    }
};
