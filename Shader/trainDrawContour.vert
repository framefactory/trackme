// test.vsh
// Draw Contour Model for Training

#version 330

in vec3 vertPosition;
in float vertIndex;

flat out float fragIndex;

layout(std140) uniform Transform
{
	mat4 matModelViewProjection;
} transform;

void main()
{
	gl_Position = transform.matModelViewProjection * vec4(vertPosition, 1.0);
	fragIndex = vertIndex;
}