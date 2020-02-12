// GLSL 3.3 Fragment Shader
// Overlay texture

#version 330

uniform sampler2DRect sDistanceTransform;

in vec2 vFragmentTexCoord;

out vec4 vOutColor;

void main()
{
	vec4 dt = texture(sDistanceTransform, vFragmentTexCoord);
	float d = 1.0 - sqrt(dt.z) / 100.0;
	if (d > 0.0)
		d = d * d;

	float red = fract(dt.w);
	float green = fract(dt.w / 2.0);
	float blue = fract(dt.w / 4.0);

	vOutColor = vec4(d * 1.0, d * 1.0, d * 1.0, 1.0);
}