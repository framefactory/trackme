// GLSL 3.3 Fragment Shader
// Overlay texture

#version 330

uniform sampler2DRect sContourData;

in vec2 vFragmentTexCoord;

out vec4 vOutColor;

const vec4 color[12] = vec4[12]
(
	vec4(1.0, 0.0, 0.0, 1.0),
	vec4(1.0, 0.5, 0.0, 1.0),
	vec4(1.0, 1.0, 0.0, 1.0),
	vec4(0.5, 1.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.5, 1.0),
	vec4(0.0, 1.0, 1.0, 1.0),
	vec4(0.0, 0.5, 1.0, 1.0),
	vec4(0.0, 0.0, 1.0, 1.0),
	vec4(0.5, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 0.5, 1.0)
);

void main()
{
	vec2 pos = gl_FragCoord.xy;
	vec4 pix = texture(sContourData, pos);
	
	vec4 c = (pix.w != -1.0) ? color[int(pix.w) % 12] : vec4(1.0);
	
	float val = pix.z;
	float d = sqrt(val);
	if (d >= 1.0) d += 4.0;
	d = max(0.0, 1.0 - d * 0.01);
	d = max(0.0, d * d * d);
	
	vOutColor = c * d;
	vOutColor.a = 1.0;
	
	
	/*
	if (pix.w != -1.0 && pix.z == 0.0)
	{
		vOutColor = color[int(pix.w) % 12];
		if (pix.z > 0.0)
			vOutColor = 0.5 * vOutColor;
	}
	else if (pix.z == 0.0)
	{
		vOutColor = vec4(1.0);
	}
	else
	{
		vOutColor = vec4(0.0);
	}
	*/
}