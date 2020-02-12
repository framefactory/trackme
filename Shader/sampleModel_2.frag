// Fragment Shader
// Phong lighting with 1 directional light source

#version 330



in Fragment
{
	flat int index;
	flat vec2 direction;
	flat int angle;
	smooth vec2 position;
	flat vec4 faceColor0;
	flat vec4 faceColor1;
} frag;


out vec4 vecOutColor;


uniform sampler2DRect sImage;          // current input image (for edge search)
uniform samplerBuffer sFilterKernel;

uniform int filterWidth = 9;

uniform float edgeLumaWeight = 1.0;
uniform float edgeChromaWeight = 1.0;
uniform float edgeThreshold = 0.5;
uniform float colorPeekDistance = 3.0;
uniform float colorFullEdgeThreshold = 0.2;
uniform float colorHalfEdgeThreshold = 0.8;

const vec3 lumaFactors = vec3(0.31, 0.59, 0.1);


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
	// nearby colors on both sides of possible edge
	float edgePeekDistance = 1.0;
	vec3 p1 = sample(sImage, frag.position - frag.direction * edgePeekDistance, frag.angle).xyz;
	vec3 p2 = sample(sImage, frag.position + frag.direction * edgePeekDistance, frag.angle).xyz;
	 
	vec3 c1 = sample(sImage, frag.position - frag.direction * colorPeekDistance, frag.angle).xyz;
	vec3 c2 = sample(sImage, frag.position + frag.direction * colorPeekDistance, frag.angle).xyz;

	vec3 f1 = frag.faceColor0.xyz;
	vec3 f2 = frag.faceColor1.xyz;

	//vec3 dp = abs(p2 - p1);
	//float dLuma = 0.333 * (dp.x + dp.y + dp.z);
	//float dChroma = max(0.0, dot(normalize(p1), normalize(p2)));

	//float dLuma = abs(dot(p1, lumaFactors) - dot(p2, lumaFactors));
	//float dChroma = 0.707 * length(normalize(p1) - normalize(p2));
	//float mag = dLuma * edgeLumaWeight + 0.707 * dChroma * edgeChromaWeight;
	
	vec3 dp = abs(p2 - p1);
	float dLuma = 0.333 * (dp.x + dp.y + dp.z);
	float dChroma = 1.0 - dot(normalize(p1 + 0.1), normalize(p2 + 0.1));
	float mag = dLuma * edgeLumaWeight * 10.0 + dChroma * edgeChromaWeight * 100.0;
	
	if (mag < edgeThreshold)
	{
		// no edge detected
		mag = 0.0;
	}
	else
	{
		float d1 = -1.0;
		float d2 = -1.0;
		
		// edge detected, check if model face is visible on both sides
		if (frag.faceColor0.a > 0.0)
		{
			dp = abs(f1 - c1);
			float d1_luma = 0.333 * (dp.x + dp.y + dp.z);
			float d1_chroma = 1.0 - dot(normalize(c1 + 0.1), normalize(f1 + 0.1));
			d1 = d1_luma * 10.0 + d1_chroma * 100.0;
		}
		if (frag.faceColor1.a > 0.0)
		{
			dp = abs(f2 - c2);
			float d2_luma = 0.333 * (dp.x + dp.y + dp.z);
			float d2_chroma = 1.0 - dot(normalize(c2 + 0.1), normalize(f2 + 0.1));
			d2 = d2_luma * 10.0 + d2_chroma * 100.0;
		}
		
		if (d1 > -1.0)
		{
			if (d2 > -1.0)
			{
				if (d1 > colorFullEdgeThreshold || d2 > colorFullEdgeThreshold)
					mag = -mag;
			}
			else
			{
				if (d1 > colorHalfEdgeThreshold)
					mag = -mag;
			}
		}
		else if (d2 > -1.0)
		{
			if (d2 > colorHalfEdgeThreshold)
				mag = -mag;
		}
	}
	
	// mag: gradient magnitude
	vecOutColor = vec4(mag, float(frag.index), frag.position);
}