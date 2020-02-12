// Fragment Shader
// Phong lighting with 1 directional light source

#version 330

//const vec4 vecDirection[2] = vec4[2](vec4(0.905, 0.301, -0.301, 0.0), vec4(-0.905, -0.301, -0.301, 0.0));

const vec4 vecDirection[3] = vec4[3](
	vec4(0.905, 0.301, -0.301, 0.0),
	vec4(-0.301, -0.905, -0.301, 0.0),
	vec4(-0.905, 0.301, -0.301, 0.0));
	
//const vec4 vecColor[2] = vec4[2](vec4(1.0, 1.0, 0.0, 1.0), vec4(0.0, 1.0, 1.0, 1.0));
const vec4 vecColor[3] = vec4[3](
	vec4(1.0, 0.99, 0.95, 1.0),
	vec4(0.80, 0.90, 0.97, 1.0),
	vec4(0.30, 0.35, 0.40, 1.0));

in Fragment
{
	vec3 vecNormal;
	vec2 vecTexCoord;
} fragment;

out vec4 vecOutColor;

uniform sampler2D sTextureImage;
uniform int textureEnabled;

void main()
{
	vec4 lightColor = vec4(0.0);
	vec4 matColor = vec4(1.0);
		
	for (int i = 0; i < 3; i++)
	{
		vec4 c = dot(-(vecDirection[i].xyz), fragment.vecNormal) * vecColor[i];
		lightColor = clamp(lightColor + c, 0.0, 1.0);
	}
	
	if (textureEnabled != 0)
		matColor = texture(sTextureImage, fragment.vecTexCoord);
	
	matColor = matColor * lightColor;
	
	vecOutColor = matColor;
	vecOutColor.a = 1.0;
}