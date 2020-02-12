// ----------------------------------------------------------------
//  Shader File		updateColors.frag
//  Type            GLSL Fragment Shader
//  GLSL Version    3.3
// ----------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-06 22:24:21 +0200 (Di, 06 Sep 2011) $
// ----------------------------------------------------------------

//  Updates the color statistics of sample point on an
//  edge model line using a simplified gaussian mixture model.

#version 330


in vec2 vFragmentTexCoord;

out vec4 vOutColor0;
out vec4 vOutColor1;
out vec4 vOutColor2;
out vec4 vOutColor3;

uniform sampler2DRect sSampledColors;

uniform sampler2DRect sColorMemory0;
uniform sampler2DRect sColorMemory1;
uniform sampler2DRect sColorMemory2;
uniform sampler2DRect sColorMemory3;

// x: threshold, y: adaptation factor
uniform vec2 updateColorParams = vec2(0.1, 0.25);
uniform float maxFreqDiff = 300.0;



void main()
{
	float threshold   = updateColorParams.x;
	float adaptFactor = updateColorParams.y;
	
	vec2 pos = gl_FragCoord.xy;
	vec4 memColor[4];
	
	// Get stored colors in memory texture 1 to 4
	memColor[0] = texture(sColorMemory0, pos);
	memColor[1] = texture(sColorMemory1, pos);
	memColor[2] = texture(sColorMemory2, pos);
	memColor[3] = texture(sColorMemory3, pos);
	
	// Sample color from current image
	vec4 sampleColor = texture(sSampledColors, pos);
	
	if (sampleColor.a == 0.0) // the sample was not present or not accurate
	{
		vOutColor0 = memColor[0];
		vOutColor1 = memColor[1];
		vOutColor2 = memColor[2];
		vOutColor3 = memColor[3];
		
		vOutColor0.a = -abs(vOutColor0.a); // mark invalid sample for viewing purposes
		return;
	}
	else if (sampleColor.a == -1.0) // sample is a background color, adapt quickly
	{
		threshold = 0.6;
		adaptFactor = 0.8;
	}
	
	// Calculate distance of pixel color to stored colors
	float d0 = distance(sampleColor.rgb, memColor[0].rgb);
	float d1 = distance(sampleColor.rgb, memColor[1].rgb);
	float d2 = distance(sampleColor.rgb, memColor[2].rgb);
	float d3 = distance(sampleColor.rgb, memColor[3].rgb);
	
	memColor[0].a = abs(memColor[0].a);
	memColor[1].a = abs(memColor[1].a);
	memColor[2].a = abs(memColor[2].a);
	memColor[3].a = abs(memColor[3].a);
	
	// If pixel is close to one of stored colors,
	// update color and frequency counter
	if (d0 < threshold)
	{
		memColor[0].rgb = mix(memColor[0].rgb, sampleColor.rgb, adaptFactor);
		memColor[0].a = memColor[0].a + 1.0;
		//float cnt = abs(memColor[0].a);
		//vOutColor0.a = (cnt - memColor[1].a < maxFreqDiff) ? cnt + 1 : cnt;
		
		vOutColor0 = memColor[0];
		vOutColor1 = memColor[1];
		vOutColor2 = memColor[2];
		vOutColor3 = memColor[3];
	}
	
	else if (d1 < threshold)
	{
		memColor[1].rgb = mix(memColor[1].rgb, sampleColor.rgb, adaptFactor);
		memColor[1].a = memColor[1].a + 1.0;
		//float cnt = memColor[1].a;
		//memColor[1].a = (cnt - memColor[2].a < maxFreqDiff) ? cnt + 1 : cnt;
		
		if (memColor[0].a < memColor[1].a)
		{
			vOutColor0 = memColor[1];
			vOutColor1 = memColor[0];
			
			vOutColor0.a = -vOutColor0.a;
		}
		else
		{
			vOutColor0 = memColor[0];
			vOutColor1 = memColor[1];
		}
		
		vOutColor2 = memColor[2];
		vOutColor3 = memColor[3];
	}
	
	else if (d2 < threshold)
	{
		memColor[2].rgb = mix(memColor[2].rgb, sampleColor.rgb, adaptFactor);
		memColor[2].a = memColor[2].a + 1.0;
		//float cnt = memColor[2].a;
		//memColor[2].a = (cnt - memColor[3].a < maxFreqDiff) ? cnt + 1 : cnt;
		
		if (memColor[0].a < memColor[2].a)
		{
			vOutColor0 = memColor[2];
			vOutColor1 = memColor[0];
			vOutColor2 = memColor[1];
			
			vOutColor0.a = -vOutColor0.a;
		}
		else
		{
			vOutColor0 = memColor[0];
			
			if (memColor[1].a < memColor[2].a)
			{
				vOutColor1 = memColor[2];
				vOutColor2 = memColor[1];
			}
			else
			{
				vOutColor1 = memColor[1];
				vOutColor2 = memColor[2];
			}
		}

		vOutColor3 = memColor[3];
	}
	else if (d3 < threshold)
	{
		memColor[3].rgb = mix(memColor[3].rgb, sampleColor.rgb, adaptFactor);
		memColor[3].a = memColor[3].a + 1.0;
		
		if (memColor[0].a < memColor[3].a)
		{
			vOutColor0 = memColor[3];
			vOutColor1 = memColor[0];
			vOutColor2 = memColor[1];
			vOutColor3 = memColor[2];
			
			vOutColor0.a = -vOutColor0.a;
		}
		else
		{
			vOutColor0 = memColor[0];
			
			if (memColor[1].a < memColor[3].a)
			{
				vOutColor1 = memColor[3];
				vOutColor2 = memColor[1];
				vOutColor3 = memColor[2];
			}
			else
			{
				vOutColor1 = memColor[1];
				
				if (memColor[2].a < memColor[3].a)
				{
					vOutColor2 = memColor[3];
					vOutColor3 = memColor[2];
				}
				else
				{
					vOutColor2 = memColor[2];
					vOutColor3 = memColor[3];
				}
			}
		}
	}
	else
	{
		vOutColor3.rgb = sampleColor.rgb;
		vOutColor3.a = 0.0;
		
		vOutColor0 = memColor[0];
		vOutColor1 = memColor[1];
		vOutColor2 = memColor[2];
	}
}