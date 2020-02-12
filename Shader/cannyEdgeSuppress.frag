// GLSL 3.3 Fragment Shader
// Canny edge detector
// Stage 2: non-maximum suppression

#version 330

uniform sampler2DRect sImage;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

void main()
{
	vec4 center = texture(sImage, vFragmentTexCoord);
	float mag = center.x;
	
	vec2 p[4];
	
	switch(int(center.y))
	{
	case 0: // Direction N
		p[0] = vec2( 0,  1); p[1] = vec2( 0,  2);
		p[2] = vec2( 0, -1); p[3] = vec2( 0, -2);
		break;
	case 1: // Direction NE
		p[0] = vec2( 1,  1); p[1] = vec2( 2,  2);
		p[2] = vec2(-1, -1); p[3] = vec2(-2, -2);
		break;
	case 2: // Direction E
		p[0] = vec2(-2,  0); p[1] = vec2(-1,  0);
		p[2] = vec2( 1,  0); p[3] = vec2( 2,  0);
		break;
	default: // Direction SE
		p[0] = vec2(-2,  2); p[1] = vec2(-1,  1);
		p[2] = vec2( 1, -1); p[3] = vec2( 2, -2);
		break;
	}
	
	for (int i = 0; i < 4; ++i)
	{
		float m = texture(sImage, vFragmentTexCoord + p[i]).x;
		if (m > mag)
		{
			center.x = 0.0;
			break;
		}
	}
	
	vOutColor = center;
}