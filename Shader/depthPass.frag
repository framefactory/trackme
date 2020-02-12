// Fragment Shader
// Phong lighting with 1 directional light source

#version 330

const vec4 vecDirection[2] = vec4[2](vec4(0.905, 0.301, 0.301, 0.0), vec4(-0.905, 0.301, 0.301, 0.0));
const vec4 vecColor[2] = vec4[2](vec4(1.0, 1.0, 0.0, 1.0), vec4(0.0, 1.0, 1.0, 1.0));

in Fragment
{
	vec3 vecNormal;
	vec2 vecTexCoord;
} fragment;

out vec4 vecOutColor;

void main()
{
	vec4 colorSum = vec4(0.0);
		
	for (int i = 0; i < 2; i++)
	{
		vec4 c = dot(-(vecDirection[i].xyz), fragment.vecNormal) * vecColor[i];
		colorSum = clamp(colorSum + c, 0.0, 1.0);
	}
	
	vecOutColor = colorSum;
	vecOutColor.a = 1.0;
}