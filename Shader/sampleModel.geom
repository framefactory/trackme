// GLSL Geometry Shader

#version 330

precision highp float;

layout(lines) in;
layout(line_strip, max_vertices = 48) out;

in Geometry
{
	int vertexID;
	vec4 faceColor;
} geometry[];

out Fragment
{
	flat ivec2 id;
	flat vec2 direction;
	flat int angle;
	smooth vec2 position;
	flat vec4 faceColor0;
	flat vec4 faceColor1;
} frag;

uniform vec2 imageSize;
uniform float minSampleDistance = 0.15; // in units
uniform float searchDistance = 20; // in pixels

uniform sampler2DRect sDepthModel; // solid model depth rendering


void main()
{
	vec2 imScale = vec2(2.0 / imageSize.x, 2.0 / imageSize.y);
	float aspect = imageSize.x / imageSize.y;

	// Get start and end point of edge line and do perspective division
	vec4 start4 = gl_in[0].gl_Position;
	vec4 end4 = gl_in[1].gl_Position;
	vec3 start = start4.xyz / start4.w;
	vec3 end = end4.xyz / end4.w;
	
	// Line length, line unit vector and line normal unit vector
	vec3 line = end - start;
	float len = length(line.xy);

	line = line / len;
	//vec2 norm = vec2(-line.y * imScale.x, line.x * imScale.y);
	vec2 norm = vec2(-line.y * imScale.x, line.x * aspect * imScale.y);

	// Per fragment data
	int lineID = min(geometry[0].vertexID, geometry[1].vertexID) / 2;
	vec2 direction = vec2(-line.y, line.x);
	int angle = int(atan(direction.y, direction.x) / 3.14159265 * 180.0 + 180.0);
	angle = (angle + 90) % 360;
	
	vec4 faceColor0 = geometry[0].faceColor;
	vec4 faceColor1 = geometry[1].faceColor;
	
	// Sample edge by subdividing sampling distance
	// until minimum distance is reached
	float offset = len * 0.5;
	float distance = len;
	int sampleID = 0;
	
	while (distance >= minSampleDistance)
	{
		for (float t = offset; t < len; t += distance)
		{
			// Sample point position
			vec3 samplePoint = start + line * t;
			
			// Sample pixel from hidden line model.
			// Discard search line if edge is hidden
			
			vec2 samplePointCoord = (samplePoint.xy + 1.0) * imageSize * 0.5;
			float modelDepth = texture(sDepthModel, samplePointCoord).x;
			float samplePointDepth = (samplePoint.z + 1.0) * 0.5;
			if (samplePointDepth - 0.01 >= modelDepth)
				continue;
				
			// Sample depth at both sides of edge line
			vec2 faceCoord0 = ((samplePoint.xy - norm * 1.5) + 1.0) * imageSize * 0.5;
			vec2 faceCoord1 = ((samplePoint.xy + norm * 1.5) + 1.0) * imageSize * 0.5;
			float faceDepth0 = texture(sDepthModel, faceCoord0).x;
			float faceDepth1 = texture(sDepthModel, faceCoord1).x;
			
			// If either side of the edge shows background, set alpha of face color to zero.
			if (faceDepth0 == 1.0)
				faceColor0.w = 0.0;
			if (faceDepth1 == 1.0)
				faceColor1.w = 0.0;
			
			vec2 searchStart = samplePoint.xy - norm * searchDistance;
			vec2 searchEnd = samplePoint.xy + norm * searchDistance;

			ivec2 id = ivec2(lineID, sampleID);
			sampleID++;
			
			// Emit start and end point of search line
			gl_Position = vec4(searchStart, 0.0, 1.0);
			frag.position = (searchStart + 1.0) * imageSize * 0.5;
			frag.id = id;
			frag.direction = direction;
			frag.angle = angle;
			frag.faceColor0 = faceColor0;
			frag.faceColor1 = faceColor1;
			EmitVertex();

			gl_Position = vec4(searchEnd, 0.0, 1.0);
			frag.position = (searchEnd + 1.0) * imageSize * 0.5;
			frag.id = id;
			frag.direction = direction;
			frag.angle = angle;
			frag.faceColor0 = faceColor0;
			frag.faceColor1 = faceColor1;
			EmitVertex();

			EndPrimitive();
		}
	
		distance = offset;
		offset = offset * 0.5;
	}
}