// ----------------------------------------------------------------
//  Shader File		drawSampleLines.geom
//  Type            GLSL Geometry Shader
//  GLSL Version    3.3
// ----------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-09 13:00:10 +0200 (Fr, 09 Sep 2011) $
// ----------------------------------------------------------------

//  Samples the received lines in regular intervals and emits
//  search lines normal to the lines, for display purposes only!

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
	flat vec2 direction;
	flat int angle;
	flat int iteration;
	flat float samplingDensity;
} frag;

uniform vec2 imageSize;

uniform float minSampleDistance = 15.0;
uniform float sampleAdaptiveDensity = 1.0;
uniform float searchRange = 20.0;
uniform float motionCompensation = 0.5;

uniform sampler2DRect sDepthModel; // solid model depth rendering


void main()
{
	// test angle between view direction and normal directions of adjacent faces
	vec3 pt = normalize(geometry[0].linePoint);
	float va0 = dot(-pt, geometry[0].lineNormal);
	float va1 = dot(-pt, geometry[1].lineNormal);

	if (va0 == va1)
	{
		if (va0 < 0.17)
			return;
	}
	else
	{
		if ((va0 < 0.0 && va1 < 0.02) || (va1 < 0.0 && va0 < 0.02))
			return;
	}
	
	int edgeID = geometry[1].vertexID / 2;
	float sampDens = (geometry[0].sampleDensity - 1.0) * sampleAdaptiveDensity + 1.0;
	float minSampDist = minSampleDistance / sampDens;
	
	// offset and scale factor: device coords -> screen coords
	vec3 dev2ImOffset = vec3(1.0, 1.0, 1.0);
	vec3 dev2ImFactor = vec3(imageSize.x * 0.5, imageSize.y * 0.5, 0.5);
	vec2 im2DevOffset = vec2(-1.0, -1.0);
	vec2 im2DevFactor = vec2(2.0 / imageSize.x, 2.0 / imageSize.y);
	
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
	int iter = 0;
	
	while (distance >= minSampDist)
	{
		for (float t = offset; t < edgeLength; t += distance)
		{
			if (sampleID >= MAX_SAMPLES_PER_EDGE)
				return;
			sampleID++;

	
			// -------- SAMPLE POINT POSITION, MOTION BLUR COMPENSATION --------
			
			// Sample point position
			vec3 samplePoint = edgeStart + edgeDir * t;
			
			// Motion-blur compenation
			vec2 prevSamplePoint = prevStart + prevDir * t;
			vec2 motion = samplePoint.xy - prevSamplePoint;
			float normalMotion = length(searchDir * dot(searchDir, motion));
			float rangeFactor = normalMotion * motionCompensation;
			
			// -------- DEPTH COMPARISONS --------

			// Sample pixel from hidden line model; discard search line if edge is hidden
			float modelDepth = texture(sDepthModel, samplePoint.xy).x;
			if (samplePoint.z > modelDepth)
				continue;
				
			// search range			
			vec2 searchStart = samplePoint.xy - searchDir * (searchRange + rangeFactor);
			vec2 searchEnd = samplePoint.xy + searchDir * (searchRange + rangeFactor);
			
			// do not search on lines near image borders
			bvec2 t1 = lessThan(searchStart, vec2(0.0));
			bvec2 t2 = greaterThan(searchStart, imageSize);
			bvec2 t3 = lessThan(searchEnd, vec2(0.0));
			bvec2 t4 = greaterThan(searchEnd, imageSize);
			if (any(t1) || any(t2) || any(t3) || any(t4))
				continue;
			
			// Emit start and end point of search line
			gl_Position = vec4(searchStart * im2DevFactor + im2DevOffset, 0.0, 1.0);
			frag.direction = searchDir;
			frag.angle = angle;
			frag.iteration = iter;
			frag.samplingDensity = geometry[0].sampleDensity;
			EmitVertex();

			gl_Position = vec4(searchEnd * im2DevFactor + im2DevOffset, 0.0, 1.0);
			frag.direction = searchDir;
			frag.angle = angle;
			frag.iteration = iter;
			frag.samplingDensity = geometry[1].sampleDensity;
			EmitVertex();

			EndPrimitive();
		}
	
		distance = offset;
		offset = offset * 0.5;
		iter++;
	}
}