// GLSL 3.3 Fragment Shader
// Overlay texture

#version 330

uniform sampler2DRect sImage;

in vec2 vFragmentTexCoord;

out vec4 vOutColor;

void main()
{
	vOutColor = abs(texture(sImage, vFragmentTexCoord));
}