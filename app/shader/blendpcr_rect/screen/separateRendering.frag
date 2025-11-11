// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#version 330 core

in vec4 vPos;
in vec2 vPosAlpha;
in vec3 vNormal;
in float vProjectorWeight;

uniform bool projectOnlyProjectorIDColor = false;
uniform int projectorID;


layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragPosition;
layout (location = 2) out vec4 FragNormal;

void main()
{
	FragColor = vec4(vProjectorWeight,0.0,0.0,1);
    FragPosition = vec4(vPos.xyz, vPosAlpha.x);
    FragNormal = vec4(vNormal, vPosAlpha.y);
}
