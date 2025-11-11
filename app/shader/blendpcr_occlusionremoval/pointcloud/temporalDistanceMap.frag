// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D currentDistanceMap;
uniform sampler2D previousDistanceMap;

out vec4 FragColor;

/**
 * Calculates a smoothed vertex for each input vertex / pixel
 */
void main()
{
	vec4 current = texture(currentDistanceMap, vScreenPos);
	vec4 previous = texture(previousDistanceMap, vScreenPos);

	//current = min(current, previous + 0.1);
	
	FragColor = current * 0.25 + previous * 0.75;
}
