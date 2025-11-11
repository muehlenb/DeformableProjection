// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

#define PROJECTOR_COUNT 3

in vec2 vScreenPos;

struct Projector {
	sampler2D distanceMap;
	mat4 projection;
	mat4 view;
};

uniform Projector projectors[PROJECTOR_COUNT];

uniform sampler2D vertexTexture;

uniform mat4 model;

out vec4 FragColor;

/**
 * Calculates a smoothed vertex for each input vertex / pixel
 */
void main()
{
    // Relative size of one pixel:
    vec2 texelSize = 1.0 / textureSize(vertexTexture, 0);
	vec4 vertexCS = vec4(texture(vertexTexture, vScreenPos).xyz, 1.0);
    vec4 vertex = model * vertexCS;
	
	if(isnan(vertexCS.z) || vertexCS.z < 0.1){
		FragColor = vec4(0.0);
		return;
	}
	
	FragColor = vec4(0.0, 0.0, 0.0, 0.0);
	for(int projectorID = 0; projectorID < PROJECTOR_COUNT; ++projectorID){
		int shadowed = 0;
		int total = 0;
	
		for(int dY = -2; dY <= 2; ++dY){
			for(int dX = -2; dX <= 2; ++dX){
				++total;
				vec4 sampleCS = vec4(texture(vertexTexture, vScreenPos + vec2(dX, dY) * texelSize).xyz, 1.0);
				vec4 sampleWS = model * sampleCS;
				
				if(sampleWS.y < 1.0 || isnan(sampleWS.y) || sampleWS.y > 3.0)
					continue;
				
				vec4 samplePS = projectors[projectorID].view * sampleWS;
				vec4 sampleProjected = projectors[projectorID].projection * samplePS;
				vec2 samplePImageS = (sampleProjected).xy / sampleProjected.w * 0.5 + 0.5;
				
				if(samplePImageS.x < 0.0 || samplePImageS.x >= 1.0 || samplePImageS.y < 0.0 || samplePImageS.y >= 1.0){
					continue;
				}
				
				float dist = texture(projectors[projectorID].distanceMap, samplePImageS).x * 4;
				
				if(dist < length(samplePS.xyz) - 0.1)
					++shadowed;
			}
		}
			
		if(total == 0)
			FragColor[projectorID] = 0.0;
		else
			FragColor[projectorID] = shadowed / float(total) > 0.90 ? 1.0 : 0.0;
	}

}
