// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#version 330 core

in vec4 vPos;

layout (location = 0) out vec4 FragColor;

void main()
{
	FragColor = vec4(length(vPos.xyz) / 4.0, 0.0, 1.0, 1.0);
}
