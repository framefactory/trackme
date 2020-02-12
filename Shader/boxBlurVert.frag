// GLSL 3.3 Fragment Shader
// Gaussian blur, vertical separation

#version 330

uniform sampler2DRect sImage;

in vec2 vFragmentTexCoord;

out vec4 vOutColor;

void main()
{
	vec4 c = vec4(0);
	
	c += texture(sImage, vFragmentTexCoord + vec2(0.0, -3.0)) * 0.142857143;
	c += texture(sImage, vFragmentTexCoord + vec2(0.0, -2.0)) * 0.142857143;
	c += texture(sImage, vFragmentTexCoord + vec2(0.0, -1.0)) * 0.142857143;
	c += texture(sImage, vFragmentTexCoord)                   * 0.142857143;
	c += texture(sImage, vFragmentTexCoord + vec2(0.0,  1.0)) * 0.142857143;
	c += texture(sImage, vFragmentTexCoord + vec2(0.0,  2.0)) * 0.142857143;
	c += texture(sImage, vFragmentTexCoord + vec2(0.0,  3.0)) * 0.142857143;

	c.w = 1.0;
	vOutColor = c;
}