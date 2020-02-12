// GLSL 3.3 Fragment Shader
// Canny edge detector
// Stage 2: non-maximum suppression

#version 330

uniform float edgeThresholdLow;
uniform float edgeThresholdHigh;
uniform float edgeThresholdAdaptive;

uniform sampler2DRect sThinEdges;
uniform sampler2DRect sBlurEdges;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

const vec4 color[4] = vec4[4](vec4(1, 0, 0, 1), vec4(1, 1, 0, 1),
	vec4(0, 1, 0, 1), vec4(0, 1, 1, 1));

const vec4 isEdge = vec4(1.0, 0.0, 0.0, 0.0);
const vec4 noEdge = vec4(0.0, 0.0, 0.0, 0.0);
	
void main()
{
	vec4 thinEdge = texture(sThinEdges, vFragmentTexCoord);
	float mag = thinEdge.x;
	float dir = thinEdge.y;
	
	if (mag < edgeThresholdLow)
	{
		vOutColor = noEdge;
		return;
	}
	else if (mag > edgeThresholdHigh)
	{
		vOutColor = isEdge;
		return;
	}


	float m1, m2, m3, m4, m5, m6, m7, m8;
	
	m1 = texture(sThinEdges, vFragmentTexCoord + vec2(-1.0, -1.0)).x;
	m2 = texture(sThinEdges, vFragmentTexCoord + vec2(-1.0,  0.0)).x;
	m3 = texture(sThinEdges, vFragmentTexCoord + vec2(-1.0,  1.0)).x;
	m4 = texture(sThinEdges, vFragmentTexCoord + vec2( 0.0, -1.0)).x;
	m5 = texture(sThinEdges, vFragmentTexCoord + vec2( 0.0,  1.0)).x;
	m6 = texture(sThinEdges, vFragmentTexCoord + vec2( 1.0, -1.0)).x;
	m7 = texture(sThinEdges, vFragmentTexCoord + vec2( 1.0,  0.0)).x;
	m8 = texture(sThinEdges, vFragmentTexCoord + vec2( 1.0,  1.0)).x;
	
	int h1 = int(m1 > edgeThresholdLow);
	int h2 = int(m2 > edgeThresholdLow);
	int h3 = int(m3 > edgeThresholdLow);
	int h4 = int(m4 > edgeThresholdLow);
	int h5 = int(m5 > edgeThresholdLow);
	int h6 = int(m6 > edgeThresholdLow);
	int h7 = int(m7 > edgeThresholdLow);
	int h8 = int(m8 > edgeThresholdLow);
	
	int sum = h1 + h2 + h3 + h4 + h5 + h6 + h7 + h8;
	
	float blurEdge = texture(sBlurEdges, vFragmentTexCoord).x;
	if (sum >= 2 && mag > blurEdge * edgeThresholdAdaptive)
	{
		vOutColor = isEdge;
		return;
	}
	
	vOutColor = noEdge;
}