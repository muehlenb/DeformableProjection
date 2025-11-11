// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

layout (location = 0) in vec2 vInPos;   // the position attribute

uniform int screenWidth;
uniform int screenHeight;
uniform int x;
uniform int y;
uniform int width;
uniform int height;

out vec2 vScreenPos;

void main()
{
    vScreenPos = vInPos.xy * 0.5 + 0.5;
	
	vec2 vertexPos = vScreenPos * vec2(float(width) / screenWidth, float(height) / screenHeight);
	vertexPos += vec2(float(x) / screenWidth, float(y) / screenHeight);

    gl_Position = vec4(vertexPos * 2 - 1, 0.5, 1.0);
}
