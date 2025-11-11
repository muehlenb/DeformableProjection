// © 2024, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)
#version 330 core

#define CAMERA_NUM 3

in vec2 vScreenPos;

uniform sampler2D color[CAMERA_NUM];
uniform sampler2D vertices[CAMERA_NUM];
uniform sampler2D normals[CAMERA_NUM];
uniform sampler2D depth[CAMERA_NUM];

uniform sampler2D miniWeightsA;

uniform bool isCameraActive[CAMERA_NUM];

out vec4 FragColor;

void main()
{		
    float distanceTreshold = 0.05;

    vec3 sumColor = vec3(0.0, 0.0, 0.0);
    vec3 sumNormal = vec3(0.0, 0.0, 0.0);
    float sumDepth = 0.0;
    float sumAlpha = 0.0;

    float lastDistToCam = 10000;

    vec4 miniWValueA = texture(miniWeightsA, vScreenPos).rgba;
	//miniWValueA = vec4(0.0, 1.0, 0.0, 0.0);

    for(int i=0; i < CAMERA_NUM; ++i){
        if(isCameraActive[i]){
            vec4 currentVertex = vec4(texture(vertices[i], vScreenPos).xyz, 1.0);
            vec2 blendFactors = vec2(texture(vertices[i], vScreenPos).a, texture(normals[i], vScreenPos).a);

            float currentAlpha = miniWValueA[i] * blendFactors.y;

            float distToCam = length(currentVertex.xyz);

            if(!(distToCam >= 0.01) || !(currentAlpha > 0.0))
                continue;

            if(distToCam + distanceTreshold < lastDistToCam){
                sumColor = vec3(0.0, 0.0, 0.0);
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

            sumColor += currentColor * currentAlpha;
            sumNormal += currentNormal * currentAlpha;
            sumDepth += currentDepth * currentAlpha;
            sumAlpha += currentAlpha;
        }
    }

    if(sumAlpha > 0.009){
        FragColor = vec4(sumColor / sumAlpha, 1.0);
        gl_FragDepth = sumDepth / sumAlpha;
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        gl_FragDepth = 1.0;
    }
}
