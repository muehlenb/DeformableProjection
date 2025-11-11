#version 330

/** Input variable (the position, which is vertex attribute 0) */
layout(location = 0) in float dist;

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

uniform int width;
uniform int height;

uniform vec4 targetPosition;

out float distance_from_vs;
out float x;
out float y;

out float deleted;


/**
 * Since this is the vertex shader, the following program is
 * executed for each vertex (i.e. triangle point) individually.
 */
void main()
{
	x = float(gl_VertexID % width) / float(width - 1) - 0.5;
	y = float(gl_VertexID / width) / float(height - 1) - 0.5;

	distance_from_vs = dist;
	vec4 vertex = vec4(x, y, dist, 1.0);
	
	if(dist == 0.0)
		deleted = 1.0;
	else
		deleted = 0.0;

    // Calculate gl_Position using the projection and view matrix.
    // It is assumed that vertex_position already is in world space.
    gl_Position = projection * view * model * vertex;
}
