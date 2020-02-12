// GLSL 3.3 Fragment Shader
// Overlay texture

#version 330

flat in float fragIndex;
out vec4 vOutColor;


void main()
{
	vOutColor = vec4(fragIndex / 255.0);
}