// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D vertexTexture;
uniform usampler2D segmentationTexture;

uniform mat4 camModel; 
uniform mat4 inverseUIMatrix;
uniform vec4 spectatorPosWS; 

layout (location = 0) out uint segmentID;
layout (location = 1) out vec3 projectorDistance;

void main()
{
	vec4 vertexWS = camModel * vec4(texture(vertexTexture, vScreenPos).xyz, 1.0);

	vec4 uiPos = inverseUIMatrix * vertexWS;
	vec2 texCoord = uiPos.xy;
	
	float distToTarget = (inverseUIMatrix * spectatorPosWS).z;
	
	//distToTarget = max(1.0, distToTarget);
	
	texCoord.x = 1 - (uiPos.x + (uiPos.z * uiPos.x) / distToTarget + 0.5);
	texCoord.y = 1 - (uiPos.y + (uiPos.z * uiPos.y) / distToTarget + 0.5);
	
	if(texCoord.x < 0 || texCoord.x > 1 || texCoord.y < 0 || texCoord.y > 1){
		projectorDistance = vec3(0.0, 0.0, 0.0);
		return;
	}
 
	uint tidx = texture(segmentationTexture, texCoord).x;
 
	if(vertexWS.y > 1.4 || vertexWS.y < 0.9)
		tidx = 0u;
		
	segmentID = tidx;
 
	projectorDistance = vec3(0.0, 0.0, 0.0);
		
	//if(tidx == 0u) 
	//	projectorDistance = vec3(texCoord.x, texCoord.y, 1.0);
	//else 
	if(tidx == 1u) 
		projectorDistance = vec3(1.0, 0.0, 0.0);
	else if(tidx == 2u) 
		projectorDistance = vec3(1.0, 1.0, 0.0);
	else if(tidx == 3u) 
		projectorDistance = vec3(0.0, 1.0, 0.0);
	else if(tidx == 4u) 
		projectorDistance = vec3(0.0, 1.0, 1.0);
	else if(tidx == 5u) 
		projectorDistance = vec3(0.0, 0.0, 1.0);
	else if(tidx == 6u) 
		projectorDistance = vec3(1.0, 0.0, 1.0);
	else if(tidx == 7u) 
		projectorDistance = vec3(0.5, 0.5, 0.5);
	else if(tidx == 8u) 
		projectorDistance = vec3(1.0, 0.0, 0.0);
	else if(tidx == 9u) 
		projectorDistance = vec3(1.0, 1.0, 0.0);
	else if(tidx == 10u) 
		projectorDistance = vec3(0.0, 1.0, 0.0);
	else if(tidx == 11u) 
		projectorDistance = vec3(0.0, 1.0, 1.0);
	else if(tidx == 12u) 
		projectorDistance = vec3(0.0, 0.0, 1.0);
	else if(tidx == 13u) 
		projectorDistance = vec3(1.0, 0.0, 1.0);
	else if(tidx == 14u) 
		projectorDistance = vec3(0.5, 0.5, 0.5);
}
 