#version 330

/**
 * This first defined out variable of the fragment shader defines
 * with which color the fragment is rendered
 */
layout(location=0) out vec4 fragment_color;

/**
 * The value of this variable comes from the vertex shader (see
 * 'out vec4 normal_from_vs;' there). Note that input variables
 * in the fragment shader, which comes from the vertex shader,
 * are automatically interpolated.
 */
in vec2 texCoord_from_vs;
in vec3 normal_from_vs;
in vec4 position_from_vs;

uniform vec4 color = vec4(1.0);

// Include lighting model implementation:
#include "lighting.shader"

/**
 * The main function of the fragment shader, which is executed
 * for every fragment.
 */
void main()
{
    // Calculate a fragment color using the texture:
    fragment_color = calculateLight(color);
}
