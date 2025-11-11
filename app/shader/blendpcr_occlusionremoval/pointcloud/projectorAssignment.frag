// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

#define PROJECTOR_COUNT 3
in vec2 vScreenPos;

struct Projector {
	mat4 projection;
	mat4 view;
	vec4 position;
	vec4 direction;
};

uniform Projector projectors[PROJECTOR_COUNT];

uniform sampler2D vertexTexture;
uniform sampler2D distanceTexture;

uniform mat4 model;

out vec4 FragColor;

/**
 * Calculates a smoothed vertex for each input vertex / pixel
 */
void main()
{
    // Relative size of one pixel:
    vec2 texelSize = 1.0 / textureSize(vertexTexture, 0);
		
	
	vec4 totalColor = vec4(0.0);
	int sum = 0;
	
	for(int dY = -3; dY <= 3; dY += 1){
		for(int dX = -3; dX <= 3; dX += 1){		
			vec2 relOffset = vec2(dX, dY) * texelSize;
		
			vec4 vertexCS = vec4(texture(vertexTexture, vScreenPos + relOffset).xyz, 1.0);
			vec4 vertex = model * vertexCS;
			vec4 shadowDistance = vec4(texture(distanceTexture, vScreenPos + relOffset).xyz, 1.0);
	
			int bestProjector = -1;
			float bestValue = 0.0;
	
			for(int projectorID = 0; projectorID < PROJECTOR_COUNT; ++projectorID){
				vec4 vertexPS = projectors[projectorID].view * vertex;
				vec4 vertexProjected = projectors[projectorID].projection * vertexPS;
				vec2 vertexPImageS = (vertexProjected).xy / vertexProjected.w * 0.5 + 0.5;
				
				if(vertexCS.z < 0.01)
					continue;

				// If outer frustum, ignore this projector:
				if(vertexPImageS.x < 0.0 || vertexPImageS.x >= 1.0 || vertexPImageS.y < 0.0 || vertexPImageS.y >= 1.0){
					continue;
				}
				
				if(vertex.y > 1.8)
					continue;
				
				float limitedShadowDistance = min(0.5, shadowDistance[projectorID] - projectorID * 0.05);
				
				if(limitedShadowDistance > bestValue){
					bestProjector = projectorID;
					bestValue = limitedShadowDistance;
				}
			}
			
			vec4 thisResult = vec4(0.0,0.0,0.0,0.0);
			
			if(bestProjector >= 0)
				thisResult[bestProjector] = 1.0;
				
			totalColor += thisResult;
			sum++;
		}
	}
	
	FragColor = totalColor / sum;
}
