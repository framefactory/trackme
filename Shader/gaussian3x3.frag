// GLSL 3.3 Fragment Shader
// Small 3 x 3 gaussian kernel

#version 330

uniform sampler2DRect sImage;

in vec2 vFragmentTexCoord;

out vec4 vOutColor;

void main()
{
	vec4 c = texture(sImage, vFragmentTexCoord + vec2(-1.0, -1.0)) * 0.0625;
	c +=     texture(sImage, vFragmentTexCoord + vec2( 0.0, -1.0)) * 0.125;
	c +=     texture(sImage, vFragmentTexCoord + vec2( 1.0, -1.0)) * 0.0625;
	c +=     texture(sImage, vFragmentTexCoord + vec2(-1.0,  0.0)) * 0.125;
	c +=     texture(sImage, vFragmentTexCoord + vec2( 0.0,  0.0)) * 0.25;
	c +=     texture(sImage, vFragmentTexCoord + vec2( 1.0,  0.0)) * 0.125;
	c +=     texture(sImage, vFragmentTexCoord + vec2(-1.0,  1.0)) * 0.0625;
	c +=     texture(sImage, vFragmentTexCoord + vec2( 0.0,  1.0)) * 0.125;
	c +=     texture(sImage, vFragmentTexCoord + vec2( 1.0,  1.0)) * 0.0625;
	
	c.w = 1.0;
	vOutColor = c;
}