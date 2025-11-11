#version 330

layout(location=0) out vec4 fragment_color;

in vec2 uv;

uniform vec4 color;

/**
 * The main function of the fragment shader, which is executed
 * for every fragment.
 */
void main()
{
	if(length(uv) > 1)
		discard;


    fragment_color = color;
}
