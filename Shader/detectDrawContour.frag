// GLSL 3.3 Fragment Shader
// Overlay texture

#version 330

flat in float fragIndex;
out vec4 vOutColor;

const vec4 color[10] = vec4[10]
(
	vec4(1.0, 0.0, 0.0, 1.0),
	vec4(1.0, 0.5, 0.0, 1.0),
	vec4(1.0, 1.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.0, 1.0, 1.0, 1.0),
	vec4(0.0, 0.5, 1.0, 1.0),
	vec4(0.0, 0.0, 1.0, 1.0),
	vec4(0.5, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 0.5, 1.0)
);


void main()
{
	int i = int(fragIndex);
	vOutColor = color[i % 10];
}