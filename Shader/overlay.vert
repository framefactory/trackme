// GLSL 3.3 Vertex Shader
// Overlay rectangle

#version 330

in vec2 vVertexPosition;
in vec2 vVertexTexCoord;

out	vec2 vFragmentTexCoord;

void main()
{
	gl_Position = vec4(vVertexPosition, 0.0, 1.0);
	vFragmentTexCoord = vVertexTexCoord;
}