#version 330 core

// Input variables from the vertex shader:
in vec2 texCoord;

// Position relative to the camera:
out uint outDepth;

// The depth texture into which the scene was renderen in the previous pass:
uniform sampler2D renderedDepthTexture;

// The noise texture:
uniform sampler2D noiseTexture;
uniform bool simulateNoise;

// Time:
uniform float time;

float fallOffStart = 50.0; // Angle in degrees
float fallOffEnd = 100.0;  // 100, so 90 (maximum) degrees still has a chance to be valid
float noiseMagnitude = 0.02;

// Camera settings:
uniform int width = 640;
uniform int height = 576;
uniform float fovX = 75.0;
uniform float fovY = 65.0;
// Near and far planes of the camera:
uniform float nearPlane =  0.01;
uniform float farPlane = 1000.0;

float halfFovX = radians(fovX / 2.0);
float halfFovY = radians(fovY / 2.0);

// Include utils (such as map):
#include "utils.shader"

// Function to convert depth to view space coordinates:
vec3 depthToViewSpace(vec2 localTexCoord, float depth) {
    float ndcDepth = depth * 2.0 - 1.0;
    float viewSpaceDepth = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - ndcDepth * (farPlane - nearPlane));

    vec3 viewSpacePos;
    viewSpacePos.x = tan(halfFovX) * (localTexCoord.x * 2.0 - 1.0) * viewSpaceDepth;
    viewSpacePos.y = tan(halfFovY) * (localTexCoord.y * 2.0 - 1.0) * viewSpaceDepth;
    viewSpacePos.z = viewSpaceDepth;

    return viewSpacePos;
}

// Function to compute normal based on depth texture:
vec3 computeNormal() {
    // Normalized offset:
    vec2 offset = vec2(1.0 / width, 1.0 / height);

    // Sample depths at neighboring pixels:
    float depth = texture(renderedDepthTexture, texCoord).r;
    float depthRight = texture(renderedDepthTexture, texCoord + vec2(offset.x, 0.0)).r;
    float depthUp = texture(renderedDepthTexture, texCoord + vec2(0.0, offset.y)).r;

    // Convert depths to view space position:
    vec3 pos = depthToViewSpace(texCoord, depth);
    vec3 posRight = depthToViewSpace(texCoord + vec2(offset.x, 0.0), depthRight);
    vec3 posUp = depthToViewSpace(texCoord + vec2(0.0, offset.y), depthUp);

    // Compute vectors:
    vec3 dX = posRight - pos;
    vec3 dY = posUp - pos;

    // Compute normal:
    vec3 normal = normalize(cross(dX, dY));
    
    return normal;
}

// Random number between 0 and 1:
float rand(vec2 uv) {
    return texture(noiseTexture, uv).r;
}

void main()
{
    // Read the depth value from the depth texture:
    float depth = texture(renderedDepthTexture, texCoord).r;
    
    // Convert depth to view space position:
    vec3 viewSpacePos = depthToViewSpace(texCoord, depth);

    // If the distance is too big, make the pixel invalid:
    if(viewSpacePos.z > 10.0) {
        outDepth = 0u;
        return;
    }

    // Calculate noise offset and random invalidation:
    if (simulateNoise) {
        float noise = rand(texCoord + vec2(time, sin(time * 0.9))) * 2.0 - 1.0; // -1 to 1
        vec3 normal = -computeNormal();
        vec3 posToCam = normalize(-viewSpacePos);
        float angle = dot(normal, posToCam); // Normalized because both components are normalized
        float angleDeg = degrees((acos(angle)));
        
        // Angle in degrees mapped between cutoff start and end:
        float angleDegMapped = max(0, map(angleDeg, fallOffStart, fallOffEnd, 0, 1));
        float randomValue = rand(texCoord + vec2(sin(time * 0.8), time));

        // The bigger the angle the bigger the chance to be invalidated:
        //if (angleDegMapped > randomValue) {
        //    outPosition = vec4(0.0, 0.0, -0.05, 0.0);
        //    return;
        //}
        
        // More noise offset with bigger angle:
        vec3 zOffset = posToCam * noise * noiseMagnitude * (1.0 - angle) * 0.5;
        viewSpacePos += zOffset;
    }

    // Set output position:
    outDepth = uint(ceil(viewSpacePos.z * 1000) +0); // Offset in mm!
}