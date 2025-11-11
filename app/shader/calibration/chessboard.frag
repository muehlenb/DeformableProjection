#version 330

layout(location=0) out vec4 fragment_color;

in vec2 uv;

uniform int margin;
uniform int fieldSize;
uniform int xCount;
uniform int yCount;

uniform vec4 fieldColor;

/**
 * The main function of the fragment shader, which is executed
 * for every fragment.
 */
void main()
{
	float x = ((uv.x - margin) / fieldSize);
	float y = ((uv.y - margin) / fieldSize);

	if(x < 0 || y < 0 || x >= xCount || y >= yCount){
		fragment_color = fieldColor;
		return;
	}
	
	if((int(x) % 2 == 0) == (int(y) % 2 == 0))
		discard;


    fragment_color = fieldColor;
}

/*
	int xPos = int(uv.x * xSize);
	int yPos = int(uv.y * ySize);

	if(xPos != 0 && yPos != 0 && xPos + 1 != xSize && yPos + 1 != ySize)
		if((xPos % 2 == 0) == (yPos % 2 == 0))
			discard;

    fragment_color = color;
*/