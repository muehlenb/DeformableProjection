#version 330

layout(location = 0) in vec4 vertex_position;


uniform int margin;
uniform int fieldSize;
uniform int xCount;
uniform int yCount;
uniform int screenWidth;
uniform int screenHeight;
uniform float positionX = 0;
uniform float positionY = 0;

out vec2 uv;

float getChessboardWidth(){
	return 2 * margin + fieldSize * xCount;
}

float getChessboardHeight(){
	return 2 * margin + fieldSize * yCount;
}

void main()
{

	uv = (vertex_position.xy + 1.0) / 2.0 * vec2(getChessboardWidth(), getChessboardHeight());

	vec2 scale = vec2(getChessboardWidth() / screenWidth, getChessboardHeight() / screenHeight);
	vec2 movingRange = vec2(1 - scale.x, 1 - scale.y);
	vec2 translation = movingRange * vec2(positionX * 2 - 1, positionY * 2 - 1);

    gl_Position = vec4(vertex_position.xy * scale + translation, 0.0, 1.0);
}
