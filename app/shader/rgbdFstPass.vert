#version 330

/** Input variable (the position, which is vertex attribute 0) */
layout(location = 0) in vec4 vertex_position;

/** Input variable (the normal, which is vertex attribute 1) */
layout(location = 1) in vec4 vertex_normal;

/** Input variable (the color, which is vertex attribute 2) (NOT USED HERE) */
layout(location = 2) in vec4 vertex_color;

/** Input variable (the tex coord, which is vertex attribute 3) */
layout(location = 3) in vec4 vertex_texCoord;

/** Outputs the position of the vertex in camera space */
out vec3 position_from_vs;

/** Outputs the position of the vertex in camera space */
out vec3 normal_from_vs;

/** Outputs the texture coordinate */
out vec2 texCoord_from_vs;

/**
 * View matrix which transforms the vertices from world space
 * into camera space.
 */
uniform mat4 view;

/**
 * Model matrix which transforms the vertices from model space
 * into world space.
 */
uniform mat4 model;

/**
 * Since this is the vertex shader, the following program is
 * executed for each vertex (i.e. triangle point) individually.
 */
void main()
{
    // Set the normal in camera space:
    normal_from_vs = normalize(mat3(view * model) * vertex_normal.xyz);
}
