#version 330 core

// Final color of the fragment
out vec4 fragment_color;

// Input variables from the vertex shader
in vec2 uv;

// The color texture into which the scene was rendered in the previous pass
uniform sampler2D projectorImage;

void main()
{
    fragment_color = texture(projectorImage, uv);
}