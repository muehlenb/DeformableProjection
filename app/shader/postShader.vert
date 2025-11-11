#version 330

// Input variable (the position, which is vertex attribute 0):
layout(location = 0) in vec4 vertex_position;

// Input variable (the normal, which is vertex attribute 1):
layout(location = 1) in vec4 vertex_normal;

// Input variable (the color, which is vertex attribute 2):
layout(location = 2) in vec4 vertex_color;

/** Input variable (the uv, which is vertex attribute 3) */
layout(location = 3) in vec2 vertex_texCoord;

/** Output variable which can be used by the fragment shader */
out vec2 texCoord;

// Since this is the vertex shader, the following program is
// executed for each vertex (i.e. triangle point) individually:
void main()
{
	//compute UV coordinates (zero to one) from the quad's vertex positions (minus one to one)
    texCoord = (vertex_position * 0.5 + 0.5).xy;

    // gl_Position is a predefined variable in every vertex
    // which has to be defined by the vertex shader:
    gl_Position = vertex_position;
}
