// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Yaroslav Purgin
#pragma once

#include "src/gl/TextureFBO.h"
#include "src/math/Vec4.h"
#include "src/gl/primitive/Triangle.h"
#include "src/gl/Mesh.h"
#include "src/gl/Shader.h"
#include <algorithm>

#include "src/Data.h"

/**
* Class for the post-processing pass of the main camera. Only header file, because it is used only once per scene.
*/
class PostProcessing {
private:
    // Quad mesh to render on:
    Mesh quadMesh;

    // FBO for the first pass:
    TextureFBO firstPassFBO;

    // Post-processing shader. Handles exposure, saturation, contrast and gamma of the final image:
    Shader postShader;

    // Constants for exposure calculation:
    const float KEY_VALUE = 0.3f;   // The higher - the brighter
    const float EPSILON = 0.0001f;
    const float ADJ_SPEED = 0.05f;

    /**  Linear interpolation for floats. */
    static float lerp(const float a, const  float b, const float t) {
        return a + t * (b - a);
    }
public:
    /**
     * Construct, load the shader and create a mesh.
     */
    PostProcessing()
        // Create a simple quad mesh for rendering from [-1, 1, 0]:
        : quadMesh(std::vector<Triangle>({
              Triangle(Vertex(Vec4f(-1.0f, -1.0f, -1.0f)), Vertex(Vec4f(1.0f, -1.0f, -1.0f)), Vertex(Vec4f(1.0f, 1.0f, -1.0f))),
              Triangle(Vertex(Vec4f(-1.0f, -1.0f, -1.0f)), Vertex(Vec4f(1.0f, 1.0f, -1.0f)), Vertex(Vec4f(-1.0f, 1.0f, -1.0f)))
            }))
        // Create our FBO for the first pass including two textures:
        , firstPassFBO(std::vector<TextureType>({
	        TextureType(GL_RGB16F, GL_RGB, GL_FLOAT),                                   // Color texture type
	        TextureType(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE),    // Depth texture type
        }))
		// Load the post-processing shader:
        , postShader(CMAKE_SOURCE_DIR "/shader/postShader.vert", CMAKE_SOURCE_DIR "/shader/postShader.frag") {
    }

    /**
    * Renders the second pass to the screen.
    */
    void render(const int display_w, const int display_h) {
        // Activate the default FBO to render directly to the screen:
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, display_w, display_h);
        
        // Bind our post-processing DOF shader:
        postShader.bind();

        // Bind the color texture of the FBO to texture slot 0 and tell our shader that
        // "renderedTexture" should use this texture at texture slot 0:
        firstPassFBO.getTexture2D(0).bind(0);
        postShader.setUniform("renderedTexture", 0);

        // Generate mipmap of the color texture (currently bound) and read back the lowest mipmap level (1x1 texture) for average color:
        glGenerateMipmap(GL_TEXTURE_2D);
        Vec4f averageColor;
        glGetTexImage(GL_TEXTURE_2D, static_cast<GLint>(log2(std::max(display_w, display_h))), GL_RGBA, GL_FLOAT, &averageColor);

        // Bind the depth texture of the FBO to texture slot 1 and tell our shader that
        // "renderedDepthTexture" should use this texture at texture slot 1:
        //firstPassFBO.getTexture2D(1).bind(1);
        //postShader.setUniform("renderedDepthTexture", 1);

        // Calculate the average luminance from the average color with weights:
        const float averageLuminance = 0.2126f * averageColor.x + 0.7152f * averageColor.y + 0.0722f * averageColor.z;
        // Calculate the logarithmic average luminance:
        const float logAvgLum = std::clamp(logf(averageLuminance + EPSILON), -3.f, 1.f);
        // Calculate the exposure value:
        Data::instance.exposure = lerp(Data::instance.exposure, KEY_VALUE / expf(logAvgLum), ADJ_SPEED);
        Data::instance.exposure = std::clamp(Data::instance.exposure, 0.1f, 10.0f);

        // Set post-processing values in the shader:
        postShader.setUniform("exposure", Data::instance.exposure);
        postShader.setUniform("saturation", Data::instance.saturation);
        postShader.setUniform("contrast", Data::instance.contrast);
        postShader.setUniform("gamma", Data::instance.gamma);

        // Draw our quad which exactly fills the screen:
        quadMesh.render();
    }

    /**
     * Provides access to the first pass Framebuffer Object(FBO).
     */
    TextureFBO* getFirstPassFBO() {
        return &firstPassFBO;
    }
};
