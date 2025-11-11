#version 330

in float distance_from_vs;
in float x;
in float y;

in float deleted;

uniform sampler2D rawUV;

uniform float minBrightness = 0.02;
uniform float targetDistance;

uniform int projectorID;

/**
 * This first defined out variable of the fragment shader defines
 * with which color the fragment is rendered
 */
layout(location=0) out vec4 fragment_color;

void main()
{
	if(deleted > 0.0){
		fragment_color = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}
		
	float distToTarget = targetDistance - distance_from_vs;

	float uvX = 1 - (x + (distance_from_vs * x) / distToTarget + 0.5);
	float uvY = 1 - (y + (distance_from_vs * y) / distToTarget + 0.5);


	vec4 minBrightnessV4 = vec4(vec3(minBrightness), 1.0);
	if(uvX > 0 && uvY > 0 && uvX < 1 && uvY < 1)
		fragment_color = max(minBrightnessV4, texture(rawUV, vec2(uvX, uvY)));
	else 
		fragment_color = minBrightnessV4;
		
	//fragment_color = vec4(1, 1, 1, 1);
		
	/*
	if(projectorID == 0)
		fragment_color *= vec4(0, 1, 1, 1);
		
	if(projectorID == 1)
		fragment_color *= vec4(1, 0, 1, 1);
		
	if(projectorID == 2)
		fragment_color *= vec4(1, 1, 0, 1);
	*/
}
