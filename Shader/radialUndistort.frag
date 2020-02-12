// GLSL 3.3 Fragment Shader
// Undo radial distortion

#version 330

uniform sampler2DRect sImage;
uniform vec2 undistFactor;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

void main()
{
	vec2 pos = gl_FragCoord.xy;
	vec2 imSize = vec2(textureSize(sImage)); 

	vec2 ctr = imSize * 0.5;
	vec2 dir = pos - ctr;
	float len = length(dir);
	dir = dir / len;
	
	float r = len / length(imSize) * 2.0;
	
	float rr = r * r;
	float radialOffset = undistFactor.x * rr + undistFactor.y * rr * rr;
	vec2 lookupPos = pos + dir * radialOffset;

	vOutColor = texture(sImage, lookupPos);
}