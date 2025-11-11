// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D currentVertices;
uniform sampler2D previousVertices;

uniform float smoothing = 0.99;

layout (location = 0) out vec4 FragPosition;

void main()
{	
	vec4 current = texture(currentVertices, vScreenPos);
	vec4 previous = texture(previousVertices, vScreenPos);
	
	float newW = previous.w + 0.1;
	if(abs(current.z - previous.z) > 0.005){
		newW = previous.w - 0.1;
	}
	
	if(abs(current.z - previous.z) > 0.1){
		newW = 0;
		
		previous.xyz = current.xyz;
	} else if(current.z == 0 || previous.z == 0){
		previous = previous.z == 0 ? vec4(current.xyz, 0.0) : vec4(0.0);
		FragPosition = previous;
	}

	
	float sm = min(sqrt(previous.w * 0.5 + 0.49), 0.98);
	
	FragPosition = vec4((current * (1.0 - sm) + previous * sm).xyz, clamp(newW, 0, 1));
	//FragPosition = current;
}
