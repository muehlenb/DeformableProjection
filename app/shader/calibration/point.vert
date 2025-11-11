#version 330

layout(location = 0) in vec4 vertex_position;

uniform int screenWidth;
uniform int screenHeight;
uniform float positionX;
uniform float positionY;
uniform float pointSize = 10;

out vec2 uv;

void main()
{
	uv = vertex_position.xy;

	vec2 scale = vec2(pointSize / screenWidth, pointSize / screenHeight);
	vec2 translation = vec2(positionX * 2 - 1, positionY * 2 - 1);

    gl_Position = vec4(vertex_position.xy * scale.xy + translation, 0.0, 1.0);
}
