// GLSL 3.3 Fragment Shader
// Overlay texture

#version 330

uniform sampler2DRect sDistanceTransform;
in vec2 vFragmentTexCoord;
out vec4 vOutColor;

const vec4 color[10] = vec4[10]
(
	vec4(1.0, 0.0, 0.5, 1.0),
	vec4(1.0, 0.0, 0.0, 1.0),
	vec4(1.0, 0.5, 0.0, 1.0),
	vec4(1.0, 1.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.0, 1.0, 1.0, 1.0),
	vec4(0.0, 0.5, 1.0, 1.0),
	vec4(0.0, 0.0, 1.0, 1.0),
	vec4(0.5, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 1.0, 1.0)
);

void main()
{
	vec2 pos = vFragmentTexCoord;
	vec4 p = texture(sDistanceTransform, pos);
	
	float d = sqrt(p.z);
	if (d > 0) d += 5;
	d = max(0.0, 1.0 - d * 0.004);
	d = max(0.1, d * d * d);
	
	if (p.w > 0.0)
	{
		vOutColor = color[int(p.w * 255.0 + 0.5) % 10] * d;
		vOutColor.w = 1.0;
		//vOutColor = vec4(d, d, d, 1.0);
	}
	else
	{
		vOutColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}