// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D image;

out vec4 FragColor;

/**
 * Calculates a smoothed vertex for each input vertex / pixel
 */
void main()
{
	FragColor = texture(image, vec2(vScreenPos.x, 1-vScreenPos.y));	
	
	if(FragColor.a < 0.01)
		discard;
}
