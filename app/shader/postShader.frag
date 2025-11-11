#version 330 core

// Input variables from the vertex shader
in vec2 texCoord;

// Final color of the fragment
out vec3 color;

// The color texture into which the scene was rendered in the previous pass
uniform sampler2D renderedTexture;

// The depth texture into which the scene was rendered in the previous pass
//uniform sampler2D renderedDepthTexture;

uniform float exposure = 0.5;
uniform float saturation = 1.0;
uniform float contrast = 1.0;
uniform float gamma = 2.2;

// Function to adjust contrast:
vec3 adjustContrast(vec3 _color, float _contrast) {
    return (_color - 0.5) * _contrast + 0.5;
}

// Function to convert RGB to HSV:
vec3 RGBtoHSV(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

// Function to convert HSV to RGB:
vec3 HSVtoRGB(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    // HDR color with exposure applied:
    vec3 hdrColor = texture(renderedTexture, texCoord).rgb * exposure;
  
    // Reinhard tone mapping:
    vec3 mappedColor = hdrColor / (hdrColor + vec3(1.0));

    // Gamma correction:
    vec3 correctedColor = pow(mappedColor, vec3(1.0 / gamma));

    // Adjust saturation:
    vec3 hsv = RGBtoHSV(correctedColor);
    hsv.y *= saturation;
    color = HSVtoRGB(hsv);
    
    // Adjust contrast:
    color = adjustContrast(color, contrast);
}