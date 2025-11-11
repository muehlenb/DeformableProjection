#version 330

layout(location = 0) in vec4 vertex_position;

out vec2 uv;

void main()
{
	uv = (vertex_position.xy + 1.0) / 2.0;

    gl_Position = vec4(vertex_position.xy, 0.0, 1.0);
}
