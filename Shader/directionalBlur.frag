// Fragment Shader
// Applies filter kernel

#version 330

uniform sampler2DRect sImage;


uniform samplerBuffer sFilterKernel;

uniform int filterWidth;
uniform int filterDirection;

in	vec2 vFragmentTexCoord;
out vec4 vecOutColor;

void main()
{
	
	vec4 sum = vec4(0.0);
	
	int fi = filterDirection * filterWidth * filterWidth;
	int fw2 = filterWidth / 2;
	
	for (int y = -fw2; y <= fw2; y++)
	{
		for (int x = -fw2; x <= fw2; x++)
		{
			float fv = texelFetch(sFilterKernel, fi).x;
			vec2 texCoord = vFragmentTexCoord + vec2(x, y);
			sum += texture(sImage, texCoord) * fv;
			fi++;
		}
	}
	
	//sum = vec4(1.0, 0.5, 0.0, 1.0);
	
	vecOutColor = sum;
	vecOutColor.a = 1.0;
}