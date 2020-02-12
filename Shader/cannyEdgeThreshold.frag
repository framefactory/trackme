// GLSL 3.3 Fragment Shader
// Canny edge detector
// Stage 3: thresholding with hysteresis

#version 330

uniform float edgeThresholdLow;
uniform float edgeThresholdHigh;
uniform float edgeThresholdAdaptive;

uniform sampler2DRect sEdges;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

const vec4 isEdge = vec4(1.0, 0.0, 0.0, 0.0);
const vec4 noEdge = vec4(0.0, 0.0, 0.0, 0.0);

const int confirmed = 1;
const int undecided = 0;
const int rejected = -1;

const int PIXEL_FOLLOW_COUNT = 20;

const vec2 dPos[8] = vec2[8]
(
	vec2( 0.0,  1.0),
	vec2( 1.0,  1.0),
	vec2( 1.0,  0.0),
	vec2( 1.0, -1.0),
	vec2( 0.0, -1.0),
	vec2(-1.0, -1.0),
	vec2(-1.0,  0.0),
	vec2(-1.0,  1.0)
);



int follow(in int prevDir, in vec2 pos, in vec4 pix,
           out int dir, out vec2 nextPos, out vec4 nextPix)
{
	int gradDir = int(pix.y);
	
	int d1 = (gradDir + 2) % 8;
	int d2 = (gradDir + 6) % 8;
	vec2 p1 = pos + dPos[d1];
	vec2 p2 = pos + dPos[d2];
	vec4 n1 = (prevDir == d2) ? vec4(0.0) : texture(sEdges, p1);
	vec4 n2 = (prevDir == d1) ? vec4(0.0) : texture(sEdges, p2);
	if (n1.x > edgeThresholdHigh || n2.x > edgeThresholdHigh)
		return confirmed;
	if (n1.x < edgeThresholdLow && n2.x < edgeThresholdLow)
		return rejected;

	if (n1.x > n2.x)
	{
		dir = d1;
		nextPos = p1;
		nextPix = n1;
	}
	else
	{
		dir = d2;
		nextPos = p2;
		nextPix = n2;
	}

	return undecided;
}


int check(int prevDir, in vec2 prevPos)
{
	vec2 pos = prevPos + dPos[prevDir];
	vec4 pix = texture(sEdges, pos);
	
	if (pix.x > edgeThresholdHigh)
		return confirmed;
	if (pix.x < edgeThresholdLow)
		return rejected;
		
	int dir;
	vec2 nextPos;
	vec4 nextPix;
	int r;

	for (int i = 0; i < PIXEL_FOLLOW_COUNT; i++)
	{
		r = follow(prevDir, pos, pix, dir, nextPos, nextPix);
		if (r == rejected || r == confirmed)
			return r;

		prevDir = dir;
		pos = nextPos;
		pix = nextPix;
	}
		
	r = follow(prevDir, pos, pix, dir, nextPos, nextPix);
	return r;
}


void main()
{
	vec2 pos = vFragmentTexCoord;
	vec4 pix = texture(sEdges, pos);
	
	if (pix.x < edgeThresholdLow)
	{
		vOutColor = noEdge;
		return;
	}
	else if (pix.x > edgeThresholdHigh)
	{
		vOutColor = vec4(1.0, 1.0, 0.0, 0.0); /* isEdge */
		return;
	}
	
	int gradDir = int(pix.y);
	int searchDir1 = (gradDir + 2) % 8;
	int searchDir2 = (gradDir + 6) % 8;

	int r = check(searchDir1, pos);
	if (r == confirmed)
	{
		vOutColor = isEdge;
		return;
	}
	
	r = check(searchDir2, pos);
	if (r == confirmed)
	{
		vOutColor = isEdge;
		return;
	}
	
	vOutColor = vec4(0.0, 0.5, 1.0, 0.0); /* noEdge */
	return;
}