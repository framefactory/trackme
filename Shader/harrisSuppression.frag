// GLSL 3.3 Fragment Shader
// Harris non-maximum suppression vertical

#version 330

uniform sampler2DRect sImage;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

void main()
{
	float c  = texture(sImage, vFragmentTexCoord + vec2( 0.0,  0.0)).x;
	if (c == 0.0)
	{
		vOutColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	float v0 = texture(sImage, vFragmentTexCoord + vec2(-1.0, -1.0)).x;
	float v1 = texture(sImage, vFragmentTexCoord + vec2( 0.0, -1.0)).x;
	float v2 = texture(sImage, vFragmentTexCoord + vec2( 1.0, -1.0)).x;
	float v3 = texture(sImage, vFragmentTexCoord + vec2(-1.0,  0.0)).x;
	float v4 = texture(sImage, vFragmentTexCoord + vec2( 1.0,  0.0)).x;
	float v5 = texture(sImage, vFragmentTexCoord + vec2(-1.0,  1.0)).x;
	float v6 = texture(sImage, vFragmentTexCoord + vec2( 0.0,  1.0)).x;
	float v7 = texture(sImage, vFragmentTexCoord + vec2( 1.0,  1.0)).x;
	
	if (v0 > c || v1 > c || v2 > c || v3 > c || v4 > c || v5 > c || v6 > c || v7 > c)
		c = 0.0;
		
	vOutColor = vec4(c, 0.0, 0.0, 1.0);
}