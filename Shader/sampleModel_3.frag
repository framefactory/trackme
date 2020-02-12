// ----------------------------------------------------------------
//  Shader File		sampleModel_3.frag
//  Type            GLSL Fragment Shader
//  GLSL Version    3.3
// ----------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------

//  Samples the gradient magnitude at a single point on a
//  edge model search line.

#version 330



in Fragment
{
	flat vec2 direction;
	flat int angle;
	smooth vec2 position;
	flat vec4 faceColor0;
	flat vec4 faceColor1;
	flat float motionFactor;
} frag;


out vec4 vecOutColor;


uniform sampler2DRect sImage;          // current input image (for edge search)
uniform samplerBuffer sFilterKernel;

uniform vec2 transferSize;
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
	
	int filterSet = min(4, int(frag.motionFactor) / 2);
	int fi = (filterSet * 180 + angle) * filterWidth * filterWidth;
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
	// -------- WRITE COLOR COMPARISON DATA ON FIRST/LAST ROW --------
	
	
	if (gl_FragCoord.y == 0.5) // center of first pixel is at (0.5, 0.5)
	{
		vecOutColor = frag.faceColor0;
		return;
	}
	if (gl_FragCoord.y == transferSize.y - 0.5)
	{
		vecOutColor = frag.faceColor1;
		return;
	}
	
	// -------- EDGE DETECTION --------
	
	
	// nearby colors on both sides of possible edge
	float edgePeekDistance = 1.0;
	float epd = edgePeekDistance + frag.motionFactor;
	float cpd = colorPeekDistance + frag.motionFactor;
	
	vec3 p1 = sample(sImage, frag.position - frag.direction * epd, frag.angle).xyz;
	vec3 p2 = sample(sImage, frag.position + frag.direction * epd, frag.angle).xyz;
	 
	vec3 c1 = sample(sImage, frag.position - frag.direction * cpd, frag.angle).xyz;
	vec3 c2 = sample(sImage, frag.position + frag.direction * cpd, frag.angle).xyz;

	vec3 f1 = frag.faceColor0.xyz;
	vec3 f2 = frag.faceColor1.xyz;

	//vec3 dp = abs(p2 - p1);
	//float dLuma = 0.333 * (dp.x + dp.y + dp.z);
	//float dChroma = max(0.0, dot(normalize(p1), normalize(p2)));

	//float dLuma = abs(dot(p1, lumaFactors) - dot(p2, lumaFactors));
	//float dChroma = 0.707 * length(normalize(p1) - normalize(p2));
	//float mag = dLuma * edgeLumaWeight + 0.707 * dChroma * edgeChromaWeight;
	
	vec3 dp = abs(p2 - p1);
	//float dLuma = 0.333 * (dp.x + dp.y + dp.z);
	float dLuma = abs(dot(p1, lumaFactors) - dot(p2, lumaFactors));
	float dChroma = 1.0 - dot(normalize(p1 + 0.1), normalize(p2 + 0.1));
	float mag = dLuma * edgeLumaWeight * 10.0 + dChroma * edgeChromaWeight * 50.0;
	//mag = mag * frag.motionFactor;
	
	
	// -------- COLOR COMPARISON --------
	
	
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
		if (frag.faceColor0.a == 1.0)
		{
			dp = abs(f1 - c1);
			float d1_luma = 0.333 * (dp.x + dp.y + dp.z);
			//float d1_chroma = 1.0 - dot(normalize(c1 + 0.1), normalize(f1 + 0.1));
			d1 = d1_luma * 10.0; // + d1_chroma * 100.0;
		}
		if (frag.faceColor1.a == 1.0)
		{
			dp = abs(f2 - c2);
			float d2_luma = 0.333 * (dp.x + dp.y + dp.z);
			//float d2_chroma = 1.0 - dot(normalize(c2 + 0.1), normalize(f2 + 0.1));
			d2 = d2_luma * 10.0; // + d2_chroma * 100.0;
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
	
	
	// -------- WRITE OUT --------
	
	
	// mag: gradient magnitude
	vecOutColor = vec4(mag, 0.0, frag.position);
}