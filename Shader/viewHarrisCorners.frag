// GLSL 3.3 Fragment Shader

#version 330

uniform sampler2DRect sSourceFrame;
uniform sampler2DRect sHarrisCorners;

in vec2 vFragmentTexCoord;

out vec4 vOutColor;

void main()
{
	vec4 sourcePix = texture(sSourceFrame, vFragmentTexCoord);
	float p0, p1, p2, p3, p4;
	
	p0 = texture(sHarrisCorners, vFragmentTexCoord + vec2(-1.0, -2.0)).x;
	p1 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 0.0, -2.0)).x;
	p2 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 1.0, -2.0)).x;

	if (p0 > 0.0 || p1 > 0.0 || p2 > 0.0)
	{
		vOutColor = vec4(1.0, 0.8, 0.0, 1.0);
		return;
	}

	p0 = texture(sHarrisCorners, vFragmentTexCoord + vec2(-2.0, -1.0)).x;
	p1 = texture(sHarrisCorners, vFragmentTexCoord + vec2(-1.0, -1.0)).x;
	p2 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 0.0, -1.0)).x;
	p3 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 1.0, -1.0)).x;
	p4 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 2.0, -1.0)).x;

	if (p0 > 0.0 || p1 > 0.0 || p2 > 0.0 || p3 > 0.0 || p4 > 0.0)
	{
		vOutColor = vec4(1.0, 0.8, 0.0, 1.0);
		return;
	}
	
	p0 = texture(sHarrisCorners, vFragmentTexCoord + vec2(-2.0, 0.0)).x;
	p1 = texture(sHarrisCorners, vFragmentTexCoord + vec2(-1.0, 0.0)).x;
	p2 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 0.0, 0.0)).x;
	p3 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 1.0, 0.0)).x;
	p4 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 2.0, 0.0)).x;

	if (p0 > 0.0 || p1 > 0.0 || p2 > 0.0 || p3 > 0.0 || p4 > 0.0)
	{
		vOutColor = vec4(1.0, 0.8, 0.0, 1.0);
		return;
	}	
	
	p0 = texture(sHarrisCorners, vFragmentTexCoord + vec2(-2.0, 1.0)).x;
	p1 = texture(sHarrisCorners, vFragmentTexCoord + vec2(-1.0, 1.0)).x;
	p2 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 0.0, 1.0)).x;
	p3 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 1.0, 1.0)).x;
	p4 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 2.0, 1.0)).x;

	if (p0 > 0.0 || p1 > 0.0 || p2 > 0.0 || p3 > 0.0 || p4 > 0.0)
	{
		vOutColor = vec4(1.0, 0.8, 0.0, 1.0);
		return;
	}
	
	p0 = texture(sHarrisCorners, vFragmentTexCoord + vec2(-1.0, 2.0)).x;
	p1 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 0.0, 2.0)).x;
	p2 = texture(sHarrisCorners, vFragmentTexCoord + vec2( 1.0, 2.0)).x;

	if (p0 > 0.0 || p1 > 0.0 || p2 > 0.0)
	{
		vOutColor = vec4(1.0, 0.8, 0.0, 1.0);
		return;
	}

	vOutColor = 0.65 * sourcePix;
}