#version 330

/**
 * This first defined out variable of the fragment shader defines
 * with which color the fragment is rendered
 */
layout(location=0) out vec4 fragment_color;

/**
 * The value of this variable comes from the vertex shader (see
 * 'out vec4 normal_from_vs;' there). Note that input variables
 * in the fragment shader, which comes from the vertex shader,
 * are automatically interpolated.
 */
in vec2 texCoord_from_vs;
/* Normal and position in camera space. Suffix _ws for world space. */
in vec3 normal_from_vs;
in vec3 normal_from_vs_ws;
in vec4 position_from_vs;
in vec4 position_from_vs_ws;

/* -- Projectors -- */

// Structure for projector:
struct Projector {
	bool is_active; // 'active' is a reserved word in GLSL
    mat4 model;
	mat4 invModel;
	mat4 projectionMatrix;
};

// Projected images (Unfortunately can not be stored in the struct):
uniform sampler2D images[5];

// ProjectingIntensity (for all projectors the same):
uniform float projectingIntensity;

uniform int proj_num;
uniform Projector projectors[5];

/** The color of the mesh: */
uniform vec4 color = vec4(1.0);

uniform bool useTexture = false;
uniform sampler2D albedoTexture;
uniform float uvMultiplier = 1.0;

uniform bool simulateFolds = false;

// Include utils (such as map):
#include "utils.shader"   
// Include lighting model implementation:
#include "lighting.shader"

// Function to extract the FOV from the projection matrix:
vec2 extractFovs(mat4 projectionMatrix) {
    float v_scale = projectionMatrix[1][1];
    float h_scale = projectionMatrix[0][0];

    // Calculate FOV from the scales:
    float v_fov = 2.0 * degrees(atan(1.0 / v_scale));
    float h_fov = 2.0 * degrees(atan(1.0 / h_scale));

    return vec2(h_fov, v_fov);
}

/**
 * The main function of the fragment shader, which is executed
 * for every fragment.
 */
void main() {

	//fragment_color = vec4(1 - normal_from_vs_ws, 1.0);
	
	//return;

	vec4 albedoColor = color;
	if(useTexture)
		albedoColor = texture(albedoTexture, texCoord_from_vs * uvMultiplier);
		
	if(simulateFolds)
		albedoColor *= map(folds(texCoord_from_vs), 0, 1, 0.93, 1.0);
		
	fragment_color = calculateLight(albedoColor);
	
	// Local projector direction in 2D:
	const vec2 dirX2D = vec2(1.0, 0.0);

	for (int i = 0; i < proj_num; i++) {
		Projector projector = projectors[i];

		if (!projector.is_active) continue;

		vec4 proj_origin = projector.model[3];
		vec4 ray_to_point = normalize(position_from_vs_ws - proj_origin);
		
		vec4 point_proj_space = projector.projectionMatrix * projector.invModel * position_from_vs_ws;

		vec3 point_ndc = point_proj_space.xyz / point_proj_space.w;

		// NDC: [-1,1] to UV: [0,1]):
		vec2 uv = point_ndc.xy * 0.5 + 0.5;

		// Draw the image only if the conditions are right:
		bool out_of_bounds = uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1;
		if (out_of_bounds) continue;
		
		// Describe how much the pixel's surface is facing the projector:
		float dot_facing = dot(-ray_to_point.xyz, normal_from_vs_ws);
		
		fragment_color += texture(images[i], uv) * max(0.000001, dot_facing) * projectingIntensity;
	}
}
