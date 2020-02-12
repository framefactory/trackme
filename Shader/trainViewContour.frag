// GLSL 3.3 Fragment Shader
// Overlay texture

#version 330

uniform sampler2DRect sContourData;
in vec2 vFragmentTexCoord;
out vec4 vOutColor;

const vec4 color[10] = vec4[10]
(
	vec4(1.0, 0.0, 0.0, 1.0),
	vec4(1.0, 0.5, 0.0, 1.0),
	vec4(1.0, 1.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.0, 1.0, 1.0, 1.0),
	vec4(0.0, 0.5, 1.0, 1.0),
	vec4(0.0, 0.0, 1.0, 1.0),
	vec4(0.5, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 1.0, 1.0),
	vec4(1.0, 0.0, 0.5, 1.0)
);

void main()
{
	vec2 pos = vFragmentTexCoord;
	int index0 = int(texture(sContourData, pos                 ).x * 255.0 + 0.5);
	int index1 = int(texture(sContourData, pos + vec2(1.0, 0.0)).x * 255.0 + 0.5);
	int index2 = int(texture(sContourData, pos + vec2(0.0, 1.0)).x * 255.0 + 0.5);
	int index3 = int(texture(sContourData, pos + vec2(1.0, 1.0)).x * 255.0 + 0.5);
	
	int index = index0;
	if (index == 0)
	{
		if (index1 > 0)
			index = index1;
		else if (index2 > 0)
			index = index2;
		else if (index3 > 0)
			index = index3;
	}
	
	if (index > 0)
		vOutColor = color[(index - 1) % 10];
	else
		vOutColor = vec4(0.0);
}