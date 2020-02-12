// GLSL 3.3 Fragment Shader
// View Depth Pass

#version 330

uniform sampler2DRect sDepthImage;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

void main()
{
	float depth = texture(sDepthImage, vFragmentTexCoord).x;
	
	if (depth == 1.0)
		vOutColor = vec4(0.0, 0.0, 0.0, 1.0);
	else
	{
		float n = 1.0; // camera z near
		float f = 100.0; // camera z far
		float v = 1.0 - (2.0 * n) / (f + n - depth * (f - n));
		vOutColor = vec4(v, v, v, 1.0);
	}
}