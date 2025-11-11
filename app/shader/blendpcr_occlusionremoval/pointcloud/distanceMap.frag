// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

#define PROJECTOR_COUNT 3

in vec2 vScreenPos;

struct Projector {
	vec4 direction;
};

uniform Projector projectors[PROJECTOR_COUNT];

uniform sampler2D vertexTexture;
uniform usampler2D nearestIndicesTexture;

uniform mat4 model;

out vec4 FragColor;

/**
 * Calculates a smoothed vertex for each input vertex / pixel
 */
void main()
{
	ivec2 uScreenPos = ivec2(round(vScreenPos * vec2(511))); 
	uvec3 nIT = texelFetch(nearestIndicesTexture, uScreenPos,0).xyz;
	
	uvec2 coords0 = uvec2(nIT.x & 65535u, (nIT.x >> 16u) & 65535u);
	uvec2 coords1 = uvec2(nIT.y & 65535u, (nIT.y >> 16u) & 65535u);
	uvec2 coords2 = uvec2(nIT.z & 65535u, (nIT.z >> 16u) & 65535u);
	
	
	
	FragColor = vec4(
		nIT.x == 0u ? 1.0 : distance(vec2(uScreenPos), vec2(coords0))/64.0, 
		nIT.y == 0u ? 1.0 : distance(vec2(uScreenPos), vec2(coords1))/64.0, 
		nIT.z == 0u ? 1.0 : distance(vec2(uScreenPos), vec2(coords2))/64.0, 
		1.0
	);
	
/*
    // Relative size of one pixel:
    vec2 texelSize = 1.0 / textureSize(shadowTexture, 0); 
    vec4 vertex = model * vec4(texture(vertexTexture, vScreenPos).xyz, 1.0);
    vec4 shadow = vec4(texture(shadowTexture, vScreenPos).xyz, 1.0);
	
	vec4 nearestShadows = vec4(0.0, 0.0, 0.0, 0.0);
	vec4 counter = vec4(0.0, 0.0, 0.0, 0.0);
	
	for(int dY = -10; dY <= 10; dY += 1){
		for(int dX = -10; dX <= 10; dX += 1){
			vec2 thisCoord = vec2(texelSize.x * dX + vScreenPos.x, texelSize.y * dY + vScreenPos.y);
			
			if(thisCoord.x >= 0 && thisCoord.x <= 1 && thisCoord.y >= 0 && thisCoord.y <= 1){
				vec4 thisShadow = vec4(texture(shadowTexture, thisCoord).xyz, 1.0);
				vec4 thisVertex = model * vec4(texture(vertexTexture, thisCoord).xyz, 1.0);
					
				vec4 thisDistances = vec4(0.0, 0.0, 0.0, 0.0);
				for(int i = 0; i < PROJECTOR_COUNT; ++i){
					float projectorXYDist = length(cross((thisVertex - vertex).xyz, projectors[i].direction.xyz));
					
					if(thisShadow[i] > 0.0){
						thisDistances[i] = clamp(1 - projectorXYDist * 5, 0.0, 1.0);
						++counter[i];
					}
				}
				
				nearestShadows = max(nearestShadows, thisDistances);
			}
		}
	}
	
	// Mini noise filtering:
	for(int i = 0; i < PROJECTOR_COUNT; ++i){
		if(counter[i] < 3){
			nearestShadows[i] = 0.0;
		}
	}	
	
	FragColor = nearestShadows;	
	*/
}
