// test.vsh
// Transform solid model including normals

#version 330

in vec3 vecPosition;
in vec3 vecNormal;
in vec2 vecTexCoord;

out Fragment
{
	vec3 vecNormal;
	vec2 vecTexCoord;
} fragment;

layout(std140) uniform Transform
{
	mat4 matModelViewProjection;
	mat4 matModelView;
} transform;

void main()
{
	gl_Position = transform.matModelViewProjection * vec4(vecPosition, 1.0);
	vec4 tn = transform.matModelView * vec4(vecNormal, 0.0);
	fragment.vecNormal = normalize(tn.xyz);
	
	fragment.vecTexCoord = vecTexCoord;
}