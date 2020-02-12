// GLSL 3.3 Fragment Shader
// Overlay texture

#version 330

uniform sampler2DRect sPatchData;
in vec2 vFragmentTexCoord;
out vec4 vOutColor;

void main()
{
	vec2 pos = vFragmentTexCoord;
	float val = texture(sPatchData, pos).x;
	
	if (val == -1.0)
		vOutColor = vec4(0.8, 0.2, 0.1, 1.0);
	else if (val == -2.0)
		vOutColor = vec4(0.7, 0.5, 0.2, 1.0);
	else if (val == -3.0)
		vOutColor = vec4(0.1, 0.4, 0.7, 1.0);
	else
	{
		float d = sqrt(val);
		if (d >= 1.0) d += 2.0;
		d = max(0.0, 1.0 - d * 0.015);
		d = max(0.1, d * d * d);
		vOutColor = vec4(d, d, d, 1.0);
	}
}