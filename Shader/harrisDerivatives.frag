// GLSL 3.3 Fragment Shader
// Precalculates derivatives for harris corner detector

#version 330

uniform sampler2DRect sImage;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

const vec4 lumaFactors = vec4(0.31, 0.59, 0.1, 0.0);

void main()
{
	// coordinates of neighboring pixels
	vec2 posS  = vFragmentTexCoord + vec2( 0, -1);
	vec2 posN  = vFragmentTexCoord + vec2( 0,  1);
	vec2 posW  = vFragmentTexCoord + vec2(-1,  0);
	vec2 posE  = vFragmentTexCoord + vec2( 1,  0);

	// sample center and neighbors in N, E directions
	vec4 colS  = texture(sImage, posS);
	vec4 colN  = texture(sImage, posN);
	vec4 colW  = texture(sImage, posW);
	vec4 colE  = texture(sImage, posE);
	
	// calculate luminance
	float lumS = dot(colS, lumaFactors);
	float lumN = dot(colN, lumaFactors);
	float lumW = dot(colW, lumaFactors);
	float lumE = dot(colE, lumaFactors);

	// calculate luminance derivatives in x and y directions
	float dX = lumE - lumW;
	float dY = lumS - lumN;
	
	// calculate coefficients for harris matrix
	float dXX = dX * dX;
	float dYY = dY * dY;
	float dXY = dX * dY;
	
	// normalize dXY because it can be negative
	dXY = (dXY + 1.0) * 0.5;
	
	vOutColor = vec4(dXX, dYY, dXY, 0.0);
}