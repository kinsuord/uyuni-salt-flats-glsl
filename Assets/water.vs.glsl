#version 420 core                                           
                                                            
layout (location = 0) in vec3 position;                     
layout (location = 1) in vec2 texcoord;

uniform mat4 um4m;
uniform mat4 um4v;
uniform mat4 um4p;
uniform vec3 cameraPosition;
uniform vec3 lightPosition;

out vec4 clipSpace;
out vec2 textureCoords;
out vec3 toCameraVector;
out vec3 fromLightVector;

const float tiling = 0.8f;
                                                            
void main(void)                                             
{       
	vec4 worldPosition = um4m *  vec4(position,1.0);

	clipSpace =um4p *um4v * um4m * vec4(position,1.0);
    gl_Position = clipSpace;
    textureCoords = texcoord *  tiling ;   

	toCameraVector = cameraPosition - worldPosition.xyz;
	fromLightVector = worldPosition.xyz - lightPosition;
}	