// GLSL 3.3 Fragment Shader
// Jump Flooding (Distance Transform) - Initial step

#version 330

// input format: x > 0 -> seed, x == 0 -> uninitialized
// the value of x is used as seedId if it's a seed

// output format: xy = closest seed offset position, z = squared distance, w = unused
// a seed has (0, 0, 0, seedId), an uninitialized pixel has (?, ?, NOT_INITIALIZED, ?)

uniform sampler2DRect sImage;
uniform float stepSize;

in vec2 vFragmentTexCoord;
out vec4 vResult;

const float NOT_INITIALIZED = 100000.0;

void main()
{
	vec2 pos = gl_FragCoord.xy; //vFragmentTexCoord;
	vec4 v0 = texture(sImage, pos);
	
	// check if current pixel is already a seed pixel
	if (v0.x > 0.0)
	{
		vResult = vec4(0.0, 0.0, 0.0, v0.x);
		return;
	}

	vec2 v;
	vec2 offset;
	vec2 distVec;
	float dist0 = NOT_INITIALIZED;
	float dist;
	
	// 1 - left, top
	offset = vec2(-stepSize, -stepSize);
	v = texture(sImage, pos + offset).xy;
	if (v.x > 0.0)
	{
		dist = offset.x * offset.x + offset.y * offset.y;
		if (dist < dist0)
		{
			v0.xy = offset;
			v0.w = v.x;
			dist0 = dist;
		}
	}
	
	// 2 - center, top
	offset = vec2(0, -stepSize);
	v = texture(sImage, pos + offset).xy;
	if (v.x > 0.0)
	{
		dist = offset.x * offset.x + offset.y * offset.y;
		if (dist < dist0)
		{
			v0.xy = offset;
			v0.w = v.x;
			dist0 = dist;
		}
	}
	
	// 3 - right, top
	offset = vec2(stepSize, -stepSize);
	v = texture(sImage, pos + offset).xy;
	if (v.x > 0.0)
	{
		dist = offset.x * offset.x + offset.y * offset.y;
		if (dist < dist0)
		{
			v0.xy = offset;
			v0.w = v.x;
			dist0 = dist;
		}
	}
	
	// 4 - left, center
	offset = vec2(-stepSize, 0);
	v = texture(sImage, pos + offset).xy;
	if (v.x > 0.0)
	{
		dist = offset.x * offset.x + offset.y * offset.y;
		if (dist < dist0)
		{
			v0.xy = offset;
			v0.w = v.x;
			dist0 = dist;
		}
	}
	
	// 5 - right, center
	offset = vec2(stepSize, 0);
	v = texture(sImage, pos + offset).xy;
	if (v.x > 0.0)
	{
		dist = offset.x * offset.x + offset.y * offset.y;
		if (dist < dist0)
		{
			v0.xy = offset;
			v0.w = v.x;
			dist0 = dist;
		}
	}
	
	// 6 - left, bottom
	offset = vec2(-stepSize, stepSize);
	v = texture(sImage, pos + offset).xy;
	if (v.x > 0.0)
	{
		dist = offset.x * offset.x + offset.y * offset.y;
		if (dist < dist0)
		{
			v0.xy = offset;
			v0.w = v.x;
			dist0 = dist;
		}
	}
	
	// 7 - center, bottom
	offset = vec2(0, stepSize);
	v = texture(sImage, pos + offset).xy;
	if (v.x > 0.0)
	{
		dist = offset.x * offset.x + offset.y * offset.y;
		if (dist < dist0)
		{
			v0.xy = offset;
			v0.w = v.x;
			dist0 = dist;
		}
	}
	
	// 8 - right, bottom
	offset = vec2(stepSize, stepSize);
	v = texture(sImage, pos + offset).xy;
	if (v.x > 0.0)
	{
		dist = offset.x * offset.x + offset.y * offset.y;
		if (dist < dist0)
		{
			v0.xy = offset;
			v0.w = v.x;
			dist0 = dist;
		}
	}
	
	if (dist0 == NOT_INITIALIZED)
	{
		vResult = vec4(0.0, 0.0, NOT_INITIALIZED, 0.0);
		return;
	}
	
	v0.z = dist0;
	vResult = v0;
}