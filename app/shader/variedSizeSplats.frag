#version 330

/**
 * This first defined out variable of the fragment shader defines
 * with which color the fragment is rendered
 */
out vec4 fragment_color;

in vec4 world_position;
in vec4 color_from_vs;

uniform bool discardBlackPixels = false;

void main()
{

	if(length(gl_PointCoord.xy - vec2(0.5)) > 0.5)
		discard;

	if(length(color_from_vs.xyz) == 0 && discardBlackPixels)
		discard;

    fragment_color = color_from_vs.bgra;
}
