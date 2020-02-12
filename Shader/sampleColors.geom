// ----------------------------------------------------------------
//  Shader File		sampleColors.geom
//  Type            GLSL Geometry Shader
//  GLSL Version    3.3
// ----------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-06 22:24:21 +0200 (Di, 06 Sep 2011) $
// ----------------------------------------------------------------

//  Samples an edge model line and sends two points, one on each
//  side of the line, to the fragment shader

#version 330
precision highp float;


#define MAX_SAMPLES_PER_EDGE       24
#define MAX_EMITTED_VERTICES       48

// MAX_SAMPLES_PER_EDGE must be set to the same value as in host application!
// MAX_EMITTED_VERTICES = MAX_SAMPLES_PER_EDGE * 2
// GLSL 3.3 alllows max. 48 vertex emissions from a geometry shader

layout(lines) in;
layout(points, max_vertices = MAX_EMITTED_VERTICES) out;


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
	flat int angle;      // direction of the search line (0...180 degrees)
	flat float residual; // residual from last optimization (0 = good...1 = bad)
	flat vec2 position;  // color sampling position in image pixel coordinates
} frag;


uniform vec2 imageSize;
uniform vec2 transferSize;

uniform float minSampleDistance = 15.0;
uniform float sampleAdaptiveDensity = 1.0;
uniform float searchRange = 20.0;
uniform float colorPeekDistance = 3.0;
uniform float surfaceAngleLimit = 0.17; // radians
uniform float contourAngleLimit = 0.2; // radians

uniform sampler2DRect sDepthModel;     // solid model depth rendering
uniform samplerBuffer sResidualData;   // The residuals of all used samples of the final pose at t-1



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
	else // line is on contour edge
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
	
	// normal vector with magnitude equal to width of 1 screen pixel
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
				

			// -------- SAMPLE POINT POSITION --------
			
			// Sample point position
			vec3 samplePoint = edgeStart + edgeDir * t;
			
			
			// -------- DEPTH COMPARISONS --------
			
			// sample pixel from depth model; discard search line if sample point is hidden
			float modelDepth = texture(sDepthModel, samplePoint.xy).x;
			if (samplePoint.z > modelDepth)
				continue;
				
			// search range			
			vec2 searchStart = samplePoint.xy - searchDir * searchRange;
			vec2 searchEnd = samplePoint.xy + searchDir * searchRange;
			
			// do not search on lines near image borders
			bvec2 t1 = lessThan(searchStart, vec2(0.0));
			bvec2 t2 = greaterThan(searchStart, imageSize);
			bvec2 t3 = lessThan(searchEnd, vec2(0.0));
			bvec2 t4 = greaterThan(searchEnd, imageSize);
			if (any(t1) || any(t2) || any(t3) || any(t4))
				continue;


				
			// color sample points
			vec2 samplePosition0 = samplePoint.xy - searchDir * colorPeekDistance;
			vec2 samplePosition1 = samplePoint.xy + searchDir * colorPeekDistance;

			// read residual of current sample (0 = reliable ... 1 = unreliable)
			float residual = texelFetch(sResidualData, targetID).x;

			
			// -------- EMIT COLOR SAMPLING POINTS --------
			
			
			gl_Position = vec4(tx, 0.5, 0.0, 1.0);
			frag.position = samplePosition0;
			frag.angle = angle;
			frag.residual = residual;
			EmitVertex();

			gl_Position = vec4(tx, -0.5, 0.0, 1.0);
			frag.position = samplePosition1;
			frag.angle = angle;
			frag.residual = residual;
			EmitVertex();

			EndPrimitive();
		}
	
		distance = offset;
		offset = offset * 0.5;
	}
}