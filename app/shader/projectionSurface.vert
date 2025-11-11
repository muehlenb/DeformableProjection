#version 330

/** Input variable (the position, which is vertex attribute 0) */
layout(location = 0) in vec4 vertex_position;

/** Input variable (the normal, which is vertex attribute 1) */
layout(location = 1) in vec4 vertex_normal;

/** Input variable (the color, which is vertex attribute 2) */
layout(location = 2) in vec4 vertex_color;

/** Input variable (the tex coord, which is vertex attribute 3) */
layout(location = 3) in vec2 vertex_texCoord;

/** Output variable which can be used by the fragment shader */
out vec2 texCoord_from_vs;

/* Normal and position in camera space. Suffix _ws for world space. */
out vec3 normal_from_vs;
out vec3 normal_from_vs_ws;
out vec4 position_from_vs;
out vec4 position_from_vs_ws;

/**
 * Transformation matrix which transforms the vertices from the
 * object coordinate system into the world coordinate system.
 */
uniform mat4 model;

/**
 * Transformation matrix which transforms the vertices from the
 * world coordinate system into the camera coordinate system.
 */
uniform mat4 view;

/**
 * Projection matrix which performs a perspective transformation.
 */
uniform mat4 projection;

uniform bool simulateFolds;
uniform bool simulateWind;
uniform float time;

uniform float bumpIntensity = 0.0;

// Include utils (such as map):
#include "utils.shader"
 

/**
 * Since this is the vertex shader, the following program is
 * executed for each vertex (i.e. triangle point) individually.
 */
void main()
{
    texCoord_from_vs = vertex_texCoord;

    position_from_vs_ws = model * vertex_position;
	normal_from_vs_ws = normalize(mat3(model) * vertex_normal.xyz);
	
	// Impact
	{
		float impact = clamp((0.15 - distance(vec3(0.0, 1.2, 0.0), position_from_vs_ws.xyz)) / 0.15, 0.0, 1.0);
		position_from_vs_ws.y += impact * bumpIntensity;
		
		vec2 normOff = position_from_vs_ws.xz;
		
		normal_from_vs_ws += vec3(normOff.x, 0.0, normOff.y) * impact * bumpIntensity * 20;
		normal_from_vs_ws = normalize(normal_from_vs_ws);
	}
	normal_from_vs = normalize(mat3(view) * normal_from_vs_ws);

	if(simulateFolds){
		position_from_vs_ws += vec4(normal_from_vs_ws, 0.0) *  (1-folds(texCoord_from_vs * 1)) * 0.005;
	}
	
	if(simulateWind){ 
		position_from_vs_ws += wind(position_from_vs_ws, time * 2);
	}
	
	
    position_from_vs = view * position_from_vs_ws;

    // Calculate gl_Position using the projection and view matrix.
    // It is assumed that vertex_position already is in world space.
    gl_Position = projection * position_from_vs;
}
