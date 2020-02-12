// GLSL 3.3 Fragment Shader
// Gaussian blur, horizontal separation

#version 330

uniform sampler2DRect sImage;

in vec2 vFragmentTexCoord;

out vec4 vOutColor;

void main()
{
	vec4 c = vec4(0);

	c += texture(sImage, vFragmentTexCoord + vec2(-3.0, 0.0)) * 0.015625;
	c += texture(sImage, vFragmentTexCoord + vec2(-2.0, 0.0)) * 0.09375;
	c += texture(sImage, vFragmentTexCoord + vec2(-1.0, 0.0)) * 0.234375;
	c += texture(sImage, vFragmentTexCoord)                   * 0.3125;
	c += texture(sImage, vFragmentTexCoord + vec2( 1.0, 0.0)) * 0.234375;
	c += texture(sImage, vFragmentTexCoord + vec2( 2.0, 0.0)) * 0.09375;
	c += texture(sImage, vFragmentTexCoord + vec2( 3.0, 0.0)) * 0.015625;

	/*	
	c += texture(sImage, vFragmentTexCoord + vec2(-2.0, 0.0)) * 0.0625;
	c += texture(sImage, vFragmentTexCoord + vec2(-1.0, 0.0)) * 0.25;
	c += texture(sImage, vFragmentTexCoord)                   * 0.375;
	c += texture(sImage, vFragmentTexCoord + vec2( 1.0, 0.0)) * 0.25;
	c += texture(sImage, vFragmentTexCoord + vec2( 2.0, 0.0)) * 0.0625;
	*/	
	
	/*
	c += texture(sImage, vFragmentTexCoord + vec2(-1.0, 0.0)) * 0.25;
	c += texture(sImage, vFragmentTexCoord)                   * 0.5;
	c += texture(sImage, vFragmentTexCoord + vec2( 1.0, 0.0)) * 0.25;
	*/
	c.w = 1.0;
	vOutColor = c;
}