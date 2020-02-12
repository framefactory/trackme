// GLSL 3.3 Fragment Shader
// Canny edge detector
// Stage 1: Detection

#version 330

uniform sampler2DRect sImage;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

const vec4 color[4] = vec4[4](vec4(1, 0, 0, 1), vec4(1, 1, 0, 1),
	vec4(0, 1, 0, 1), vec4(0, 1, 1, 1));

void main()
{
	// coordinates of neighboring pixels
	float d = 1.0;
	vec2 posN  = vec2(vFragmentTexCoord.x, vFragmentTexCoord.y + d);
	vec2 posNE = vec2(vFragmentTexCoord.x + d, vFragmentTexCoord.y + d);
	vec2 posE  = vec2(vFragmentTexCoord.x + d, vFragmentTexCoord.y);
	vec2 posSE = vec2(vFragmentTexCoord.x + d, vFragmentTexCoord.y - d);
	vec2 posS  = vec2(vFragmentTexCoord.x, vFragmentTexCoord.y - d);
	vec2 posSW = vec2(vFragmentTexCoord.x - d, vFragmentTexCoord.y - d);
	vec2 posW  = vec2(vFragmentTexCoord.x - d, vFragmentTexCoord.y);
	vec2 posNW = vec2(vFragmentTexCoord.x - d, vFragmentTexCoord.y + d);

	// sample center and neighbors in N, NE, E, SE directions
	vec4 colC  = texture(sImage, vFragmentTexCoord);
	vec4 colN  = texture(sImage, posN);
	vec4 colNE = texture(sImage, posNE);
	vec4 colE  = texture(sImage, posE);
	vec4 colSE = texture(sImage, posSE);
	vec4 colS  = texture(sImage, posS);
	vec4 colSW = texture(sImage, posSW);
	vec4 colW  = texture(sImage, posW);
	vec4 colNW = texture(sImage, posNW);

	// calculate first order derivatives in four directions	
	
	vec4 dN  = abs(colN  - colS) * 0.333334;
	vec4 dNE = abs(colNE - colSW) * (0.707107 * 0.333334);
	vec4 dE  = abs(colE  - colW) * 0.333334;
	vec4 dSE = abs(colSE - colNW) * (0.707107 * 0.333334);
	/*
	vec4 dN  = abs(colN  - colS);
	vec4 dNE = abs(colNE - colSW) * 0.707107;
	vec4 dE  = abs(colE  - colW);
	vec4 dSE = abs(colSE - colNW) * 0.707107;
	*/
	
	// Sum up values of individual color channels
	
	float gN  = dN.x  + dN.y  + dN.z;
	float gNE = dNE.x + dNE.y + dNE.z;
	float gE  = dE.x  + dE.y  + dE.z;
	float gSE = dSE.x + dSE.y + dSE.z;
	/*
	float gN  = max(dN.x, max(dN.y, dN.z));
	float gNE = max(dNE.x, max(dNE.y,dNE.z));
	float gE  = max(dE.x, max(dE.y, dE.z));
	float gSE = max(dSE.x, max(dSE.y, dSE.z));
	*/
	
	// Find direction with maximum gradient	
	int dir;
	float mag; 
	
	if (gNE > gN)
	{
		if (gE > gNE)
		{
			if (gSE > gE)
			{ dir = 3; mag = gSE; }
			else
			{ dir = 2; mag = gE; }
		}
		else
		{
			if (gSE > gNE)
			{ dir = 3; mag = gSE; }
			else
			{ dir = 1; mag = gNE; }
		}
	}
	else
	{
		if (gE > gN)
		{
			if (gSE > gE)
			{ dir = 3; mag = gSE; }
			else
			{ dir = 2; mag = gE; }
		}
		else
		{
			if (gSE > gN)
			{ dir = 3; mag = gSE; }
			else
			{ dir = 0; mag = gN; }
		}
	}
	
	vOutColor = vec4(mag, dir, 0, 0);
	
	//float v = sqrt(mag);
	//vOutColor = v * color[dir];
}