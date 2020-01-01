#version 420 core                                           
                                                            
layout (location = 0) in vec3 position;                     
layout (location = 1) in vec2 texcoord;

uniform mat4 um4mvp;

out vec4 clipSpace;

out VS_OUT                                                  
{                                                           
    vec2 texcoord;  
                      
} vs_out;                                                   
                                                            
                                                            
void main(void)                                             
{        
	clipSpace = um4mvp * vec4(position,1.0);
    gl_Position = clipSpace;
    vs_out.texcoord = texcoord;   

	//vs_out.texcoord = vec2(position.x/2.0 + 0.5, position.y/2.0 + 0.5);

}	