#version 410 core                                           
                                                            
layout (location = 0) in vec2 position;                     
layout (location = 1) in vec2 texcoord;

uniform mat4 um4mvp;
uniform mat4 shadow_matrix;

out VS_OUT                                                  
{                                                           
    vec2 texcoord;  
    vec4 shadow_coord;                                        
} vs_out;                                                   
                                                            
                                                            
void main(void)                                             
{                                                           
    gl_Position = um4mvp * vec4(position.x,0.0,position.y,1.0);
    vs_out.texcoord = texcoord;   
    vs_out.shadow_coord = shadow_matrix * vec4(position.x,0.0,position.y,1.0);                          
}	