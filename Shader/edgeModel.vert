// test.vsh
// Dummy vertex shader

#version 330

in vec4 vecPosition;
in vec4 vecColor;

out Fragment
{
	vec3 vecTexCoord;
} fragment;

layout(std140) uniform Transform
{
	mat4 matModelViewProjection;
} transform;

void main()
{
	gl_Position = transform.matModelViewProjection * vecPosition;
	fragment.vecTexCoord = vecPosition.xyz;
}