#version 420 core                              

in vec4 clipSpace;
in vec2 textureCoords;
in vec3 toCameraVector;
in vec3 fromLightVector;

out vec4 out_Color;

layout(binding=0) uniform sampler2D reflectionTexture; // above the water
layout(binding=1) uniform sampler2D refractionTexture; // under the water
layout(binding=2) uniform sampler2D dudvMap; 
layout(binding=3) uniform sampler2D normalMap; 

uniform float moveFactor;
uniform vec3 cameraPosition;
uniform vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

const float waveStrength = 0.01f;
const float shineDamper = 30.0f;
const float reflectivity = 0.6f;

void main(void) {

	vec2 ndc = (clipSpace.xy/clipSpace.w)/2.0f + 0.5f; 
	vec2 reflectTexCoords = vec2(ndc.x, 1.0f-ndc.y);
	vec2 refractTexCoords = vec2(ndc.x, ndc.y);

//	vec2 distortion1 = (texture(dudvMap, vec2(textureCoords.x + moveFactor, textureCoords.y)).rg * 2.0f - 1.0f)*waveStrength;
//	vec2 distortion2 = (texture(dudvMap, vec2(-textureCoords.x, textureCoords.y + moveFactor)).rg * 2.0f - 1.0f)*waveStrength;
//	vec2 totalDistortion = distortion1 + distortion2;

	vec2 distortedTexCoords = texture(dudvMap, vec2(textureCoords.x + moveFactor, textureCoords.y)).rg*0.1;
	distortedTexCoords = textureCoords + vec2(distortedTexCoords.x, distortedTexCoords.y+moveFactor);
	vec2 totalDistortion = (texture(dudvMap, distortedTexCoords).rg * 2.0 - 1.0) * waveStrength;

	reflectTexCoords += totalDistortion;
	refractTexCoords += totalDistortion;

	vec4 reflectColor = texture(reflectionTexture, reflectTexCoords);
	vec4 refractColor = texture(refractionTexture, refractTexCoords);

	vec3 viewVector = normalize(toCameraVector);
	float refractiveFactor = dot(viewVector, vec3(0.0f, 1.0f, 0.0f));
	
	vec4 normalMapColor = texture(normalMap, distortedTexCoords);
	vec3 normal = vec3(normalMapColor.r * 2.0f - 1.0f, normalMapColor.b, normalMapColor.g * 2.0f - 1.0f);
	normal = normalize(normal);

	vec3 reflectedLight = reflect(normalize(fromLightVector), normal);
	float specular = max(dot(reflectedLight, viewVector), 0.0);
	specular = pow(specular, shineDamper);
	vec3 specularHighlights = lightColor * specular * reflectivity;

	out_Color = mix(reflectColor, refractColor, refractiveFactor);
	out_Color = out_Color + vec4(specularHighlights, 0.0f); 
}