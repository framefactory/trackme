// GLSL 3.3 Fragment Shader
// Overlay texture

#version 330

uniform sampler2DRect sSourceImage;
uniform sampler2DRect sOverlayImage;
uniform float imageBlendFactor = 0.5;

in vec2 vFragmentTexCoord;

out vec4 vOutColor;

void main()
{
	vec4 op = texture(sOverlayImage, vFragmentTexCoord);
	
	if (op.a > 0.0)
	{
		vOutColor = op;
	}
	else
	{
		vec4 sp = texture(sSourceImage, vFragmentTexCoord);
		vOutColor = sp * imageBlendFactor;
	}
}