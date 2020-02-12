// ----------------------------------------------------------------
//  Shader File		sampleModel_3.geom
//  Type            GLSL Geometry Shader
//  GLSL Version    3.3
// ----------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------

//  Samples the received lines in regular intervals and emits
//  search lines normal to the lines.

#version 330
precision highp float;


#define MAX_SAMPLES_PER_EDGE       24
#define MAX_EMITTED_VERTICES       48

// MAX_SAMPLES_PER_EDGE must be set to the same value as in host application!
// MAX_EMITTED_VERTICES = MAX_SAMPLES_PER_EDGE * 2
// GLSL 3.3 alllows max. 48 vertex emissions from a geometry shader

layout(lines) in;
layout(line_strip, max_vertices = MAX_EMITTED_VERTICES) out;


in Geometry
{
	int vertexID;
	float sampleDensity;
	vec3 linePoint;
	vec3 lineNormal;
	vec3 prevPosition;
} geometry[];

out Fragment
{
	flat vec2 direction;     // unit vector indicating direction of search line
	flat int angle;          // direction of the search line (0...180 degrees)
	smooth vec2 position;    // sampling position in image pixel coordinates
	flat vec4 faceColor0;
	flat vec4 faceColor1;
	flat float motionFactor;
} frag;

uniform vec2 imageSize;
uniform vec2 transferSize;

uniform float minSampleDistance = 15.0;
uniform float sampleAdaptiveDensity = 1.0;
uniform float searchRange = 20.0;
uniform float colorPeekDistance = 3.0;
uniform float motionCompensation = 0.5;
uniform float surfaceAngleLimit = 0.17; // radians
uniform float contourAngleLimit = 0.2; // radians
uniform int filterWidth = 9;
uniform float colorAdaptability = 0.2; 

uniform sampler2DRect sDepthModel;    // solid model depth rendering
uniform sampler2DRect sPrevImage;     // previous image for color comparison
uniform samplerBuffer sFilterKernel;  // directional blur filter for color sampling
uniform sampler2DRect sColorBuffer1;  // buffer of samples x search where y = 0 and y = ny-1 = colors at t-1
uniform sampler2DRect sColorBuffer2;  // color memory buffer of matching implementation 2
uniform samplerBuffer sResidualData;  // The residuals of all used samples of the final pose at t-1


vec4 sample(sampler2DRect sampler, vec2 coords, int angle)
{
	vec4 sum = vec4(0.0);
	
	int fi = angle * filterWidth * filterWidth;
	int fw2 = filterWidth / 2;
	
	for (int y = -fw2; y <= fw2; y++)
	{
		for (int x = -fw2; x <= fw2; x++)
		{
			float coeff = texelFetch(sFilterKernel, fi).x;
			vec2 texCoord = coords + vec2(x, y);
			sum += texture(sampler, texCoord) * coeff;
			fi++;
		}
	}
	
	return sum;
}


void main()
{
	// test angle between view direction and normal directions of adjacent faces
	vec3 pt = normalize(geometry[0].linePoint);
	float va0 = dot(-pt, geometry[0].lineNormal);
	float va1 = dot(-pt, geometry[1].lineNormal);

	if (va0 == va1) // line is on plain surface
	{
		if (va0 < surfaceAngleLimit)
			return;
	}
	else // line is part of the model's shape
	{
		if ((va0 < 0.0 && va1 < contourAngleLimit) || (va1 < 0.0 && va0 < contourAngleLimit))
			return;
	}
			
	int edgeID = geometry[1].vertexID / 2;
	float sampDens = (geometry[0].sampleDensity - 1.0) * sampleAdaptiveDensity + 1.0;
	float minSampDist = minSampleDistance / sampDens;

	// offset and scale factor: device coords -> screen coords
	vec3 dev2ImOffset = vec3(1.0, 1.0, 1.0);
	vec3 dev2ImFactor = vec3(imageSize.x * 0.5, imageSize.y * 0.5, 0.5);
	
	// Get start and end point of edge line and do perspective division
	vec4 edgeStart_hd = gl_in[0].gl_Position;
	vec4 edgeEnd_hd   = gl_in[1].gl_Position;
	vec3 edgeStart_d  = edgeStart_hd.xyz / edgeStart_hd.w;
	vec3 edgeEnd_d    = edgeEnd_hd.xyz / edgeEnd_hd.w;
	vec3 edgeStart    = (edgeStart_d + dev2ImOffset) * dev2ImFactor;
	vec3 edgeEnd      = (edgeEnd_d + dev2ImOffset) * dev2ImFactor;
	
	// If any part of edge is behind camera, discard it
	if (edgeStart_d.z >= 1.0 || edgeEnd_d.z >= 1.0)
		return;
	
	// Edge length, edge unit vector and edge normal unit vector
	vec3 edgeLine = edgeEnd - edgeStart;
	float edgeLength = length(edgeLine.xy);
	vec3 edgeDir = edgeLine / edgeLength;
	
	vec3 prevStart_d = geometry[0].prevPosition;
	vec3 prevEnd_d = geometry[1].prevPosition;
	vec2 prevStart = ((prevStart_d + dev2ImOffset) * dev2ImFactor).xy;
	vec2 prevEnd = ((prevStart_d + dev2ImOffset) * dev2ImFactor).xy;
	vec2 prevLine = prevEnd - prevStart;
	//float prevLength = length(prevLine);
	vec2 prevDir = prevLine / edgeLength;
	
	// normal vector with magnitude equal to 1 screen pixel
	vec2 searchDir = vec2(-edgeDir.y, edgeDir.x);

	// angle of search direction
	int angle = int(atan(searchDir.y, searchDir.x) / 3.14159265 * 180.0 + 180.0);
	angle = (angle + 90) % 180;
	
	// Sample edge by subdividing sampling distance
	// until minimum distance is reached
	float offset = edgeLength * 0.5;
	float distance = edgeLength;
	int sampleID = 0;
	
	while (distance >= minSampDist)
	{
		for (float t = offset; t < edgeLength; t += distance)
		{
			if (sampleID >= MAX_SAMPLES_PER_EDGE)
				return;
				
			// -------- TARGET COLUMN OF SEARCH LINE RESULT --------	

			// vertical line slot in target
			int targetID = edgeID * MAX_SAMPLES_PER_EDGE + sampleID;
			sampleID++;

			// scale from transfer image scale to unit rect
			float tx = (float(targetID) + 0.5) / (transferSize.x * 0.5) - 1.0;


			// -------- SAMPLE POINT POSITION, MOTION BLUR COMPENSATION --------
			
			// Sample point position
			vec3 samplePoint = edgeStart + edgeDir * t;
			
			// Motion-blur compenation
			vec2 prevSamplePoint = prevStart + prevDir * t;
			vec2 motion = samplePoint.xy - prevSamplePoint;
			float normalMotion = length(searchDir * dot(searchDir, motion));
			float rangeOffset = normalMotion * motionCompensation;
			float motionFactor = normalMotion * motionCompensation;
			
			
			// -------- DEPTH COMPARISONS --------
			
			// sample pixel from depth model; discard search line if sample point is hidden
			float modelDepth = texture(sDepthModel, samplePoint.xy).x;
			if (samplePoint.z > modelDepth)
				continue;
				
			// search range			
			vec2 searchStart = samplePoint.xy - searchDir * (searchRange + rangeOffset);
			vec2 searchEnd = samplePoint.xy + searchDir * (searchRange + rangeOffset);
			
			// do not search on lines near image borders
			bvec2 t1 = lessThan(searchStart, vec2(0.0));
			bvec2 t2 = greaterThan(searchStart, imageSize);
			bvec2 t3 = lessThan(searchEnd, vec2(0.0));
			bvec2 t4 = greaterThan(searchEnd, imageSize);
			if (any(t1) || any(t2) || any(t3) || any(t4))
				continue;


			// -------- ADAPTIVE COLOR MATCHING --------

			// sample colors at each side of edge positions in previous image				
			vec2 pcp0 = samplePoint.xy - searchDir * colorPeekDistance;
			vec2 pcp1 = samplePoint.xy + searchDir * colorPeekDistance;
			vec4 prevColor0 = sample(sPrevImage, pcp0, angle);
			vec4 prevColor1 = sample(sPrevImage, pcp1, angle);
			prevColor0.a = 1.0;
			prevColor1.a = 1.0;
			
			// read color history, first and last pixel of column
			vec2 hcp0 = vec2(targetID, 0.0);
			vec2 hcp1 = vec2(targetID, transferSize.y - 1.0);
			vec4 historyColor0 = texture(sColorBuffer1, hcp0);
			vec4 historyColor1 = texture(sColorBuffer1, hcp1);

			// read residual of current sample (0 = reliable ... 1 = unreliable)
			float residual = texelFetch(sResidualData, targetID).x;
			
			float trust0 = max(0.05, 1.0 - residual * historyColor0.a) * colorAdaptability;
			float trust1 = max(0.05, 1.0 - residual * historyColor0.a) * colorAdaptability;
			vec4 faceColor0 = (1.0 - trust0) * historyColor0 + trust0 * prevColor0;
			vec4 faceColor1 = (1.0 - trust1) * historyColor1 + trust1 * prevColor1;
			
			// ACM V2
			vec4 matchColor0 = texture(sColorBuffer2, vec2(targetID, 0.0));
			vec4 matchColor1 = texture(sColorBuffer2, vec2(targetID, 1.0));

			// Sample depth at both sides of edge line
			float faceDepth0 = texture(sDepthModel, pcp0).x;
			float faceDepth1 = texture(sDepthModel, pcp1).x;

			// Use the color's alpha channel to indicate if it's a model or a background sample
			faceColor0.a = faceDepth0 < 1.0 ? 1.0 : 0.0;
			faceColor1.a = faceDepth1 < 1.0 ? 1.0 : 0.0;
			
			//faceColor0.rgb = matchColor0.rgb;
			//faceColor1.rgb = matchColor1.rgb;
			

			
			// -------- EMIT SEARCH LINE --------
						
			// Emit start and end point of search line
			gl_Position = vec4(tx, 1.0, 0.0, 1.0);
			frag.position = searchStart;
			frag.direction = searchDir;
			frag.angle = angle;
			frag.faceColor0 = faceColor0;
			frag.faceColor1 = faceColor1;
			frag.motionFactor = motionFactor;
			EmitVertex();

			gl_Position = vec4(tx, -1.0, 0.0, 1.0);
			frag.position = searchEnd;
			frag.direction = searchDir;
			frag.angle = angle;
			frag.faceColor0 = faceColor0;
			frag.faceColor1 = faceColor1;
			frag.motionFactor = motionFactor;
			EmitVertex();

			EndPrimitive();
		}
	
		distance = offset;
		offset = offset * 0.5;
	}
}