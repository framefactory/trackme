// ----------------------------------------------------------------
//  Shader File		sampleModel.vert
//  Type            GLSL Vertex Shader
//  GLSL Version    3.3
// ----------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-09 13:00:10 +0200 (Fr, 09 Sep 2011) $
// ----------------------------------------------------------------

//  Vertex transformation where each vertex
//  is either the start or end point of an edge model line.

#version 330

in vec3 vecPosition;
in vec3 vecNormal;
in float sampleDensity;

out Geometry
{
	int vertexID;
	float sampleDensity;
	vec3 linePoint;
	vec3 lineNormal;
	vec3 prevPosition;
} geometry;


layout(std140) uniform Transform
{
	mat4 matModelViewProjection;
	mat4 matModelView;
	mat4 matMVPPrevious;
} transform;

void main()
{
	geometry.vertexID = gl_VertexID;
	geometry.sampleDensity = sampleDensity;

	vec4 p = vec4(vecPosition, 1.0);
	vec4 n = vec4(vecNormal, 0.0);

	gl_Position = transform.matModelViewProjection * p;
	vec4 prevPos = transform.matMVPPrevious * p;
	geometry.prevPosition = prevPos.xyz / prevPos.w;

	geometry.linePoint = (transform.matModelView * p).xyz;
	geometry.lineNormal = (transform.matModelView * n).xyz;
}