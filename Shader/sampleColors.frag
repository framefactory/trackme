// ----------------------------------------------------------------
//  Shader File		sampleColors.frag
//  Type            GLSL Geometry Shader
//  GLSL Version    3.3
// ----------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-06 22:24:21 +0200 (Di, 06 Sep 2011) $
// ----------------------------------------------------------------

//  Samples colors on both sides of model edge, writes
//  result to a 2 x N texture that serves as input for
//  the statistics update

#version 330


in Fragment
{
	flat int angle;      // direction of the search line (0...180 degrees)
	flat float residual; // residual from last optimization (0 = good...1 = bad)
	flat vec2 position;  // color sampling position in image pixel coordinates
} frag;

out vec4 vOutColor;

uniform int filterWidth = 9;
uniform sampler2DRect sImage;
uniform sampler2DRect sDepthModel;
uniform samplerBuffer sFilterKernel;


vec4 sample(sampler2DRect sampler, vec2 coords, int angle)
{
	vec4 sum = vec4(0.0);
	
	int filterSet = 0;
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
	vec4 color = sample(sImage, frag.position, frag.angle);
	float depth = texture(sDepthModel, frag.position).x;

	if (depth < 1.0)
		color.a = max(0.0, 1.0 - frag.residual);
	else
		color.a = -1.0;
	
	vOutColor = color;
}