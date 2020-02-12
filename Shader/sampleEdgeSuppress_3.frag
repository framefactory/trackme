// GLSL 3.3 Fragment Shader
// Harris non-maximum suppression vertical

#version 330

uniform sampler2DRect sImage;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

void main()
{
	vec2 pos = gl_FragCoord.xy;
	float ny = textureSize(sImage).y;
	
	vec4 p = texture(sImage, pos);
	
	if (pos.y > 0.5 && pos.y < ny - 0.5)
	{
		if (p.x == 0)
		{
			if (p.w > 0)
				vOutColor = vec4(0.0, 0.0, 0.0, 1.0);
			else
				vOutColor = vec4(0.0);
			return;
		}

		float v0 = (pos.y < 3.5) ? 0.0 : abs(texture(sImage, gl_FragCoord.xy + vec2( 0.0, -2.0)).x);
		float v1 = (pos.y < 2.5) ? 0.0 : abs(texture(sImage, gl_FragCoord.xy + vec2( 0.0, -1.0)).x);
		float v2 = (pos.y > ny - 3.5) ? 0.0 : abs(texture(sImage, gl_FragCoord.xy + vec2( 0.0,  1.0)).x);
		float v3 = (pos.y > ny - 2.5) ? 0.0 : abs(texture(sImage, gl_FragCoord.xy + vec2( 0.0,  2.0)).x);

		float mag = abs(p.x);
		if (v0 > mag || v1 > mag || v2 > mag || v3 > mag)
		{
			vOutColor = vec4(0.0, 0.0, 0.0, 1.0);
			return;
		}
	}
	
	vOutColor = p;
}