// GLSL 3.3 Fragment Shader
// Jump Flooding (Distance Transform) - Incremental step

#version 330

// input/output format:
// xy = closest seed offset position, z = squared distance, w = seedId
// a seed has (0, 0, 0, seedId), an uninitialized pixel has (?, ?, NOT_INITIALIZED, ?)
// all other pixels have (offsetX, offsetY, distance, seedId)

uniform sampler2DRect sImage;
uniform float stepSize;

in vec2 vFragmentTexCoord;
out vec4 vResult;

const float NOT_INITIALIZED = 100000.0;

void main()
{
	ivec2 pos = ivec2(gl_FragCoord.xy);
	vec4 v0 = texture(sImage, pos);
	
	/*
	if (fract(pos.x / 2.0) == 0.0)
		vResult = vec4(200.0);
	else
		vResult = vec4(0.0);
	return;
	*/
	
	// check if current pixel is already a seed pixel
	if (v0.z == 0.0)
	{
		vResult = v0;
		return;
	}

	vec2 distVec0 = v0.xy;
	float dist0 = v0.z;
	
	vec4 v;
	vec2 offset;
	vec2 distVec;
	float dist;
	
	// 1 - left, top
	offset = vec2(-stepSize, -stepSize);
	v = texture(sImage, pos + offset);
	if (v.z < NOT_INITIALIZED)
	{
		distVec = v.xy + offset;
		dist = distVec.x * distVec.x + distVec.y * distVec.y;
		if (dist < dist0)
		{
			v0.xy = distVec;
			v0.w = v.w;
			dist0 = dist;
		}
	}
	
	// 2 - center, top
	offset = vec2(0, -stepSize);
	v = texture(sImage, pos + offset);
	if (v.z < NOT_INITIALIZED)
	{
		distVec = v.xy + offset;
		dist = distVec.x * distVec.x + distVec.y * distVec.y;
		if (dist < dist0)
		{
			v0.xy = distVec;
			v0.w = v.w;
			dist0 = dist;
		}
	}
	
	// 3 - right, top
	offset = vec2(stepSize, -stepSize);
	v = texture(sImage, pos + offset);
	if (v.z < NOT_INITIALIZED)
	{
		distVec = v.xy + offset;
		dist = distVec.x * distVec.x + distVec.y * distVec.y;
		if (dist < dist0)
		{
			v0.xy = distVec;
			v0.w = v.w;
			dist0 = dist;
		}
	}
	
	// 4 - left, center
	offset = vec2(-stepSize, 0);
	v = texture(sImage, pos + offset);
	if (v.z < NOT_INITIALIZED)
	{
		distVec = v.xy + offset;
		dist = distVec.x * distVec.x + distVec.y * distVec.y;
		if (dist < dist0)
		{
			v0.xy = distVec;
			v0.w = v.w;
			dist0 = dist;
		}
	}
	
	// 5 - right, center
	offset = vec2(stepSize, 0);
	v = texture(sImage, pos + offset);
	if (v.z < NOT_INITIALIZED)
	{
		distVec = v.xy + offset;
		dist = distVec.x * distVec.x + distVec.y * distVec.y;
		if (dist < dist0)
		{
			v0.xy = distVec;
			v0.w = v.w;
			dist0 = dist;
		}
	}
	
	// 6 - left, bottom
	offset = vec2(-stepSize, stepSize);
	v = texture(sImage, pos + offset);
	if (v.z < NOT_INITIALIZED)
	{
		distVec = v.xy + offset;
		dist = distVec.x * distVec.x + distVec.y * distVec.y;
		if (dist < dist0)
		{
			v0.xy = distVec;
			v0.w = v.w;
			dist0 = dist;
		}
	}
	
	// 7 - center, bottom
	offset = vec2(0, stepSize);
	v = texture(sImage, pos + offset);
	if (v.z < NOT_INITIALIZED)
	{
		distVec = v.xy + offset;
		dist = distVec.x * distVec.x + distVec.y * distVec.y;
		if (dist < dist0)
		{
			v0.xy = distVec;
			v0.w = v.w;
			dist0 = dist;
		}
	}
	
	// 8 - right, bottom
	offset = vec2(stepSize, stepSize);
	v = texture(sImage, pos + offset);
	if (v.z < NOT_INITIALIZED)
	{
		distVec = v.xy + offset;
		dist = distVec.x * distVec.x + distVec.y * distVec.y;
		if (dist < dist0)
		{
			v0.xy = distVec;
			v0.w = v.w;
			dist0 = dist;
		}
	}
	
	v0.z = dist0;
	vResult = v0;
}