#version 420 core                              

in vec4 clipSpace;

out vec4 out_Color;

layout(binding=0) uniform sampler2D reflectionTexture; // above the water
layout(binding=1) uniform sampler2D refractionTexture; // under the water

in VS_OUT                                                                     
{                                                                             
    vec2 texcoord;
                                                        
} fs_in;  

void main(void) {

	vec2 ndc = (clipSpace.xy/clipSpace.w)/2.0f + 0.5f; 
	vec2 reflectTexCoords = vec2(ndc.x, 1.0f-ndc.y);
	vec2 refractTexCoords = vec2(ndc.x, ndc.y);

	vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);
	vec4 refractColor = texture(refractionTexture, refractTexCoords);
	
	out_Color = mix(reflectColor, refractColor, 0.5f);


}