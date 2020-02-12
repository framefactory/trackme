// GLSL 3.3 Fragment Shader

#version 330

uniform sampler2DRect sSourceFrame;
uniform sampler2DRect sHiddenLineModel;
uniform sampler2DRect sSearchLines;

in vec2 vFragmentTexCoord;

out vec4 vOutColor;

void main()
{
	vec4 sourcePix = texture(sSourceFrame, vFragmentTexCoord);
	vec4 modelPix = texture(sHiddenLineModel, vFragmentTexCoord);
	
	vec4 e0 = texture(sSearchLines, vFragmentTexCoord);
	vec4 e1 = texture(sSearchLines, vFragmentTexCoord + vec2(1.0, 0.0));
	vec4 e2 = texture(sSearchLines, vFragmentTexCoord + vec2(0.0, 1.0));
	vec4 e3 = texture(sSearchLines, vFragmentTexCoord + vec2(1.0, 1.0));
	
	vec4 e = e0.x > 0.0 ? e0 : (e1.x > 0.0 ? e1
		: (e2.x > 0.0 ? e2 : (e3.x > 0.0 ? e3 : e0)));
	
	vec4 edgePix;
	if (e.x > 0.0)
		edgePix = vec4(1.0, 1.0, 0.3, 1.0);
	else
		edgePix = vec4(0.2, 0.6, 1.0, 1.0);
	
	vec4 result = sourcePix * 0.5;
	
	if (e.w > 0.0)
		result = edgePix;
	
	else if (modelPix.w > 0.0)
		result = vec4(0.4, 0.7, 0.2, 1.0);
	
	vOutColor = vec4(result.xyz, 1.0);
}