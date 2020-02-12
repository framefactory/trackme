// Fragment Shader
// draw sample lines

#version 330


in Fragment
{
	flat vec2 direction;
	flat int angle;
	flat int iteration;
	flat float samplingDensity;
} frag;


out vec4 vecOutColor;


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
	//vecOutColor = color[frag.iteration % 12];
	
	//float r = frag.angle / 180.0;
	//float g = 1.0 - r;
	//vecOutColor = vec4(r, g, 0.0, 1.0); 
	
	float d = min(1.0, frag.samplingDensity / 3.0 + 0.3);
	vecOutColor = vec4(1.0, 0.8, 0.3, 1.0);
}