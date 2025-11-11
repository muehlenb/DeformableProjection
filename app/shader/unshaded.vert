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

out vec4 worldpos;

/**
 * Since this is the vertex shader, the following program is
 * executed for each vertex (i.e. triangle point) individually.
 */
void main()
{
    texCoord_from_vs = vertex_texCoord;

	worldpos = model* vertex_position;

    // Calculate gl_Position using the projection and view matrix.
    // It is assumed that vertex_position already is in world space.
    gl_Position = projection * view * model* vertex_position;
}
