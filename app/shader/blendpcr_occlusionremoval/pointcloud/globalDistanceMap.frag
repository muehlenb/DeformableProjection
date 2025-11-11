// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

#define CAMERA_COUNT 3

in vec2 vScreenPos;

uniform sampler2D vertexTextures[CAMERA_COUNT];
uniform sampler2D distanceTextures[CAMERA_COUNT];
uniform sampler2D lookupImageTo3D[CAMERA_COUNT];
uniform sampler2D lookup3DToImage[CAMERA_COUNT];

uniform mat4 model[CAMERA_COUNT];
uniform mat4 view[CAMERA_COUNT];

uniform int cameraID;

uniform int cameraNum = 2;

out vec4 FragColor;

/**
 * Calculates a smoothed vertex for each input vertex / pixel
 */
void main()
{
	// Default distances is the distance texture of current camera:
	FragColor = texture(distanceTextures[cameraID], vScreenPos);
	
	int camCount = min(cameraNum, CAMERA_COUNT);
	for(int otherCamID=0; otherCamID < camCount; ++otherCamID){	
		if(otherCamID == cameraID)
			continue;
	
		vec4 vertexCurrent = vec4(texture(vertexTextures[cameraID], vScreenPos).xyz, 1.0);
		vec4 vertexCurrentWS = model[cameraID] * vertexCurrent;
		
		// Current vertex in other cam space:
		vec4 vertexCurrentInOther = view[otherCamID] * vertexCurrentWS;
		
		// Lookup tables:
		vec2 luCoords = vec2(vertexCurrentInOther.x / vertexCurrentInOther.z, vertexCurrentInOther.y / vertexCurrentInOther.z) * 0.5 + 0.5;
		vec2 otherCamImageCoords = texture(lookup3DToImage[otherCamID], luCoords).xy;
		
		ivec2 otherTextureSize = textureSize(vertexTextures[otherCamID], 0);
		vec2 otherCamRelCoords = otherCamImageCoords / otherTextureSize;
		
		vec4 vertexOther = texture(vertexTextures[otherCamID], otherCamRelCoords);
		vec4 vertexOtherWS = model[otherCamID] * vertexOther;
		
		// If this is not the same surface, ignore:
		if(distance(vertexOtherWS, vertexCurrentWS) > 0.05){
			continue;
		}
		
		vec4 otherDistancesProjectedOnCurrent = texture(distanceTextures[otherCamID], otherCamRelCoords);
		
		// Think about what is the best option here:
		// Adding would sum and average out the distortion, but could prioritize areas
		// which are seen by all cameras, while it's enough when one camera sees it and
		// has a high distance. Furthermore, it exceeds the 8bit range of [0,1] (scale):
		FragColor = min(FragColor, otherDistancesProjectedOnCurrent);
	}
}
