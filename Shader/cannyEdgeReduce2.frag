// GLSL 3.3 Fragment Shader
// Canny edge detector
// Stage 2: non-maximum suppression

#version 330

uniform sampler2DRect sEdges;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

void main()
{
	float m1 = texture(sEdges, vFragmentTexCoord + vec2( 1.0,  0.0)).x;
	float m2 = texture(sEdges, vFragmentTexCoord + vec2( 0.0, -1.0)).x;
	float m3 = texture(sEdges, vFragmentTexCoord + vec2(-1.0,  0.0)).x;
	
	if ((m1 > 0 && m2 > 0) || (m2 > 0 && m3 > 0))
	{
		vOutColor = vec4(0.0);
		return;
	}
	
	vOutColor = texture(sEdges, vFragmentTexCoord);
}