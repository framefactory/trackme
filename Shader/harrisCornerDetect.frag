// GLSL 3.3 Fragment Shader
// Harris corner detector

#version 330

uniform sampler2DRect sImage;

in vec2 vFragmentTexCoord;
out vec4 vOutColor;

const vec4 color[4] = vec4[4](vec4(1, 0, 0, 1), vec4(1, 1, 0, 1),
	vec4(0, 1, 0, 1), vec4(0, 1, 1, 1));

const float k = 1.0;

void main()
{
	// get harris coefficients for current pixel
	// x: dX * dX, y: dY * dY, z: dX * dY
	vec4 hc = texture(sImage, vFragmentTexCoord);
	
	// denormalize z: dX * dY
	hc.z = hc.z * 2.0 - 1.0;
	
	// calculate harris measure
	float xy = hc.x + hc.y;
	
	// Original meaure
	
	/*
	float harris = (hc.x * hc.y - hc.z * hc.z) - 0.04 * xy * xy;
	if (harris < 0.00001)
		harris = 0.0;
	*/

	// Copyright (c) 2002-2010 Peter Kovesi
	// Centre for Exploration Targeting
	// The University of Western Australia
	// http://www.csse.uwa.edu.au/~pk/research/matlabfns/
	
	// Alison Noble, "Descriptions of Image Surfaces", PhD thesis, Department
	// of Engineering Science, Oxford University 1989, p45.
	
	float harris = (hc.x * hc.y - hc.z * hc.z) / (xy + 0.0000001);
	if (harris < 0.001)
		harris = 0.0;
		
	vOutColor = vec4(harris, 0.0, 0.0, 0.0);
}