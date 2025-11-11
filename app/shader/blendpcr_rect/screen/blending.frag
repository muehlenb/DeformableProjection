// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#version 330 core

#define CAMERA_NUM 3

#define MAX_SHADOW_TILES 50

in vec2 vScreenPos;

struct ShadowTile {
	float projectorWeight[3];
	float minDistance;
	bool useFallback;
};

uniform bool tileBasedShadowAvoidance;
uniform bool ignoreShadowAvoidance;

uniform int shadowTileCount;
uniform ShadowTile shadowTiles[MAX_SHADOW_TILES];

uniform sampler2D color[CAMERA_NUM];
uniform sampler2D vertices[CAMERA_NUM];
uniform sampler2D normals[CAMERA_NUM];
uniform sampler2D depth[CAMERA_NUM];

uniform sampler2D miniWeightsA;

uniform bool isCameraActive[CAMERA_NUM];

uniform usampler2D texture2D_segmentID;
uniform sampler2D texture2D_ui;
uniform mat4 inverseUIMatrix;

uniform mat4 inverseView;
uniform vec4 spectatorPosWS;

uniform float markerHue;
uniform float markerInvSize;

uniform bool projectOnlyProjectorIDColor = false;
uniform int projectorID = -1;

out vec4 FragColor;

vec3 hsv2rgb(vec3 hsv) {
    float h = hsv.x;
    float s = hsv.y;
    float v = hsv.z;
    
    float c = v * s;
    float x = c * (1.0 - abs(mod(h * 6.0, 2.0) - 1.0));
    float m = v - c;
    
    vec3 rgb;
    
    if (h < 1.0 / 6.0) {
        rgb = vec3(c, x, 0.0);
    } else if (h < 2.0 / 6.0) {
        rgb = vec3(x, c, 0.0);
    } else if (h < 3.0 / 6.0) {
        rgb = vec3(0.0, c, x);
    } else if (h < 4.0 / 6.0) {
        rgb = vec3(0.0, x, c);
    } else if (h < 5.0 / 6.0) {
        rgb = vec3(x, 0.0, c);
    } else {
        rgb = vec3(c, 0.0, x);
    }
    
    return rgb + m;
}

float fmod(float x, float y)
{
  return x - y * floor(x/y);
}

void main()
{		
    float distanceTreshold = 0.1;

	float sumProjectorWeight = 0.0;
    vec3 sumVertex = vec3(0.0, 0.0, 0.0);
    vec3 sumNormal = vec3(0.0, 0.0, 0.0);
    float sumDepth = 0.0;
    float sumAlpha = 0.0;

    float lastDistToCam = 10000;
 
    float smoothBlend[4];
    vec4 miniWValueA = texture(miniWeightsA, vScreenPos).rgba;

	//miniWValueA = vec4(0.0, 0.0, 1.0, 0.0);

    for(int i=0; i < CAMERA_NUM; ++i){
        if(isCameraActive[i]){
            vec3 currentVertex = texture(vertices[i], vScreenPos).xyz;
            vec2 blendFactors = vec2(texture(vertices[i], vScreenPos).a, texture(normals[i], vScreenPos).a);

            float currentAlpha = blendFactors.x * 0.1 + blendFactors.y;

            float distToCam = length(currentVertex);

            if(!(distToCam >= 0.01) || !(currentAlpha > 0.0))
                continue;

            if(distToCam + distanceTreshold < lastDistToCam){
                sumProjectorWeight = 0.0;
				sumVertex = vec3(0.0, 0.0, 0.0);
                sumNormal = vec3(0.0, 0.0, 0.0);
                sumDepth = 0.0;
                sumAlpha = 0.0;
                lastDistToCam = distToCam;
            } else if(distToCam - distanceTreshold > lastDistToCam){
                continue;
            }

            vec3 currentNormal = texture(normals[i], vScreenPos).xyz;
            vec3 currentColor = texture(color[i], vScreenPos).rgb;
            float currentDepth = texture(depth[i], vScreenPos).r;

            sumProjectorWeight += currentColor.r * currentAlpha;
			sumVertex += currentVertex * currentAlpha;
            sumNormal += currentNormal * currentAlpha;
            sumDepth += currentDepth * currentAlpha;
            sumAlpha += currentAlpha;
        }
    }

	if(sumAlpha < 0.009){
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        gl_FragDepth = 1.0;
		return;
    }
	
	float projectorWeight = sumProjectorWeight / sumAlpha;
	
	vec4 uiPos = inverseUIMatrix * (inverseView * vec4(sumVertex / sumAlpha, 1.0));
	vec2 texCoord = uiPos.xy;
	
	float distToTarget = (inverseUIMatrix * spectatorPosWS).z;
	//distToTarget = max(1.0, distToTarget);
	
	
	//texCoord.x = (uiPos.x + (uiPos.z * uiPos.x) / distToTarget + 0.5);
	//texCoord.y = (uiPos.y + (uiPos.z * uiPos.y) / distToTarget + 0.5);
	
	texCoord.x = (uiPos.x / (1 - (uiPos.z / distToTarget)) + 0.5);
	texCoord.y = (uiPos.y / (1 - (uiPos.z / distToTarget)) + 0.5); 
	
	FragColor = vec4(0.02,0.02,0.02,1);
		
	
	if(texCoord.x >= 0 && texCoord.x < 1 && texCoord.y >= 0 && texCoord.y < 1){		
		if(tileBasedShadowAvoidance){
			uint segmentID = texture(texture2D_segmentID, vec2(1.0, 1.0) - texCoord).x;
			if(segmentID > 0u && segmentID <= uint(shadowTileCount)){
				ShadowTile tile = shadowTiles[int(segmentID)-1];
				if(tile.minDistance > 0.03){
					projectorWeight = pow(tile.projectorWeight[projectorID], 1.2 / 2.0);
				}
			}
		}
		
		if(!projectOnlyProjectorIDColor){	
			vec3 uiTexel = texture(texture2D_ui, vec2(1.0 - texCoord.x, texCoord.y)).xyz;
			FragColor.xyz = max(vec3(0.02), uiTexel * (ignoreShadowAvoidance ? 0.70 : projectorWeight));
			
			
			vec2 scaledTexCoords = texCoord * vec2(1.77777,1);
			/*
			float d1 = length(scaledTexCoords - vec2((1-0.21354) * 1.7777, 0.94444)) * markerInvSize;
			float d2 = length(scaledTexCoords - vec2((1-0.84375) * 1.7777, 0.0648)) * markerInvSize;
			float d3 = length(scaledTexCoords - vec2((1-0.15625) * 1.7777, 0.0648)) * markerInvSize;
			float d4 = length(scaledTexCoords - vec2((1-0.84375) * 1.7777, 0.94444)) * markerInvSize;
			*/
			
			//FragColor.xyz = vec3(0.0,0.0,0.0);
			
			/*
			int xSize = 12;
			int ySize = 8;
			
			if(projectorID == 0)
			for(int tY = 0; tY < ySize; ++tY){
				for(int tX = 0; tX < xSize; ++tX){
				
					float cTX = (tX+1) / float(xSize+2);
					float cTY = (tY+1) / float(ySize+2);
				
					float d1 = length(scaledTexCoords - vec2((1-cTX) * 1.7777, cTY)) * markerInvSize;
				
					if(d1 < 1)
						FragColor.xyz = max(vec3(0.02), vec3(1.0, 1.0, 1.0));
				}
			}
			*/
			
		} else {
			FragColor.xyz = max(vec3(0.02), vec3(projectorID == 0 ? 1.0 : 0.0, projectorID == 1 ? 1.0 : 0.0, projectorID == 2 ? 1.0 : 0.0) * projectorWeight);
		}
	}
	
	//FragColor.xyz = hsv2rgb(vec3(fmod((inverseView * vec4(sumVertex / sumAlpha, 1.0)),1.0), 1.0, 1.0)); 
	
	
	gl_FragDepth = sumDepth / sumAlpha;
}
