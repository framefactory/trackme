// Fragment Shader
// Phong lighting with 1 directional light source

#version 330



in Fragment
{
	flat ivec2 id;			// x: lineID, y: sampleID
	flat vec2 direction;
	flat int angle;
	smooth vec2 position;
	flat vec4 faceColor0;
	flat vec4 faceColor1;
} frag;


out vec4 vecOutColor;


uniform sampler2DRect sImage;
uniform samplerBuffer sFilterKernel;

uniform int filterWidth = 9;

uniform float edgeLumaWeight = 1.0;
uniform float edgeChromaWeight = 1.0;
uniform float edgeThreshold = 0.5;
uniform float colorPeekDistance = 2.0;
uniform float colorFullEdgeThreshold = 0.2;
uniform float colorHalfEdgeThreshold = 0.8;

const vec3 lumaFactors = vec3(0.31, 0.59, 0.1);

vec4 sample(vec2 coords)
{
	vec4 sum = vec4(0.0);
	
	int fi = frag.angle * filterWidth * filterWidth;
	int fw2 = filterWidth / 2;
	
	for (int y = -fw2; y <= fw2; y++)
	{
		for (int x = -fw2; x <= fw2; x++)
		{
			float coeff = texelFetch(sFilterKernel, fi).x;
			vec2 texCoord = coords + vec2(x, y);
			sum += texture(sImage, texCoord) * coeff;
			fi++;
		}
	}
	
	return sum;
}


void main()
{
	vec3 p1 = sample(frag.position - frag.direction).xyz;
	vec3 p2 = sample(frag.position + frag.direction).xyz;
	
	float v1 = dot(p1, lumaFactors);
	float v2 = dot(p2, lumaFactors);
	float dLuma = abs(v2 - v1);
	
	//vec3 dp = abs(p2 - p1);
	//float dLuma = 0.333 * (dp.x + dp.y + dp.z);
	float dChroma = dot(normalize(p1), normalize(p2));
	float d = dLuma * edgeLumaWeight * 10.0 + dChroma * edgeChromaWeight * 0.1;
	
	float s = 1.0;

	
	if (d < edgeThreshold)
	{
		// no edge detected
		d = 0.0;
	}
	else
	{
		// sample colors on both side of edge
		vec3 c0 = sample(frag.position - frag.direction * colorPeekDistance).xyz;
		vec3 c1 = sample(frag.position + frag.direction * colorPeekDistance).xyz;

		// edge detected, check if model face is visible on both sides
		if (frag.faceColor0.a > 0.0 && frag.faceColor1.a > 0.0)
		{
			vec3 sampleColDir = normalize(c1 - c0);
			vec3 faceColDir = normalize(frag.faceColor1.xyz - frag.faceColor0.xyz);
			s = dot(sampleColDir, faceColDir);

			if (s < colorFullEdgeThreshold)
				s = 0.0;
		}
		else // only compare color if model face is visible
		{
			if (frag.faceColor0.a > 0.0)
				s = dot(normalize(frag.faceColor0.xyz), normalize(c0));
			else
				s = dot(normalize(frag.faceColor1.xyz), normalize(c1));
			
			if (s < colorHalfEdgeThreshold)
				s = -1.0;
		}

	}
	
	vecOutColor = vec4(d, s, frag.id.x, frag.id.y);
}