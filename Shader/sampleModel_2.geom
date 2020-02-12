// GLSL Geometry Shader

#version 330

#define MAX_SAMPLES_PER_EDGE       128
#define MAX_EMITTED_PRIMITIVES     24
#define MAX_EMITTED_VERTICES       48

precision highp float;

layout(lines) in;
layout(line_strip, max_vertices = MAX_EMITTED_VERTICES) out;

in Geometry
{
	int vertexID;
	vec4 faceColor;
} geometry[];

out Fragment
{
	flat int index;
	flat vec2 direction;
	flat int angle;
	smooth vec2 position;
	flat vec4 faceColor0;
	flat vec4 faceColor1;
} frag;

uniform vec2 imageSize;
uniform vec2 transferSize;

uniform float minSampleDistance = 15.0;
uniform float searchRange = 20.0;
uniform float colorPeekDistance = 3.0;
uniform int filterWidth = 9;

uniform sampler2DRect sDepthModel;         // solid model depth rendering
uniform sampler2DRect sPrevImage;          // previous image for color comparison
uniform samplerBuffer sFilterKernel;       // directional blur filter for color sampling


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
	int edgeID = geometry[1].vertexID / 2;

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

	// normal vector with magnitude equal to 1 screen pixel
	vec2 searchDir = vec2(-edgeDir.y, edgeDir.x);

	// angle of search direction
	int angle = int(atan(searchDir.y, searchDir.x) / 3.14159265 * 180.0 + 180.0);
	angle = (angle + 90) % 360;
	
	// Sample edge by subdividing sampling distance
	// until minimum distance is reached
	float offset = edgeLength * 0.5;
	float distance = edgeLength;
	int sampleID = 0;
	int primitiveID = 0;
	
	while (distance >= minSampleDistance)
	{
		for (float t = offset; t < edgeLength; t += distance)
		{
			if (primitiveID >= MAX_EMITTED_PRIMITIVES)
				break;

			sampleID++;
			if (sampleID >= MAX_SAMPLES_PER_EDGE)
				break;
				
			int index = edgeID * MAX_SAMPLES_PER_EDGE + sampleID;
			
			// Sample point position
			vec3 samplePoint = edgeStart + edgeDir * t;
			
			// Sample pixel from hidden line model; discard search line if edge is hidden
			float modelDepth = texture(sDepthModel, samplePoint.xy).x;
			if (samplePoint.z > modelDepth)
				continue;
				
			float md1 = texture(sDepthModel, samplePoint.xy + vec2(-1.0, -1.0)).x;
			float md2 = texture(sDepthModel, samplePoint.xy + vec2( 1.0, -1.0)).x;
			float md3 = texture(sDepthModel, samplePoint.xy + vec2(-1.0,  1.0)).x;
			float md4 = texture(sDepthModel, samplePoint.xy + vec2( 1.0,  1.0)).x;
			
			float td = samplePoint.z - 0.01;
			if (md1 < td || md2 < td ||md3 < td || md4 < td)
				continue;
			
			vec2 searchStart = samplePoint.xy - searchDir * searchRange;
			vec2 searchEnd = samplePoint.xy + searchDir * searchRange;
			
			// do not search on lines near image borders
			bvec2 t1 = lessThan(searchStart, vec2(0.0));
			bvec2 t2 = greaterThan(searchStart, imageSize);
			bvec2 t3 = lessThan(searchEnd, vec2(0.0));
			bvec2 t4 = greaterThan(searchEnd, imageSize);
			if (any(t1) || any(t2) || any(t3) || any(t4))
				continue;

			// sample colors at each side of edge positions in previous image				
			vec2 colorPos0 = samplePoint.xy - searchDir * colorPeekDistance;
			vec2 colorPos1 = samplePoint.xy + searchDir * colorPeekDistance;
			vec4 faceColor0 = sample(sPrevImage, colorPos0, angle);
			vec4 faceColor1 = sample(sPrevImage, colorPos1, angle);
			
			// Sample depth at both sides of edge line
			float faceDepth0 = texture(sDepthModel, colorPos0).x;
			float faceDepth1 = texture(sDepthModel, colorPos1).x;

			// If either side of the edge shows background, set alpha of face color to zero.
			faceColor0.w = faceDepth0 < 1.0 ? 1.0 : 0.0;
			faceColor1.w = faceDepth1 < 1.0 ? 1.0 : 0.0;
			
			// vertical line slot in target
			int targetID = edgeID * MAX_EMITTED_PRIMITIVES + primitiveID;

			// scale from transfer image scale to unit rect
			float tx = float(targetID) / (transferSize.x * 0.5) - 1.0;
		
			// Emit start and end point of search line
			gl_Position = vec4(tx, 1.0, 0.0, 1.0);
			frag.position = searchStart;
			frag.index = index;
			frag.direction = searchDir;
			frag.angle = angle;
			frag.faceColor0 = faceColor0;
			frag.faceColor1 = faceColor1;
			EmitVertex();

			gl_Position = vec4(tx, -1.0, 0.0, 1.0);
			frag.position = searchEnd;
			frag.index = index;
			frag.direction = searchDir;
			frag.angle = angle;
			frag.faceColor0 = faceColor0;
			frag.faceColor1 = faceColor1;
			EmitVertex();

			EndPrimitive();
			primitiveID++;
		}
	
		distance = offset;
		offset = offset * 0.5;
	}
}