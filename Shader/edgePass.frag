// Fragment Shader
// Phong lighting with 1 directional light source

#version 330


in Fragment
{
	vec3 vecTexCoord;
} fragment;

out vec4 vecOutColor;

void main()
{
	vecOutColor = vec4(1.0, 1.0, 1.0, 1.0);
}