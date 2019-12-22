#version 410 core                                                                                                                                                                                       

out vec4 color;                                                               

uniform int drawshadow;
uniform sampler2DShadow tex_shadow;

in VS_OUT                                                                     
{                                                                             
    vec2 texcoord;
    vec4 shadow_coord;                                                           
} fs_in;                                                                      
                                                                            
void main(void)                                                               
{       
    if(drawshadow==0)
    {
        color = vec4(0.64, 0.57, 0.49, 1.0);                                         
    }
    else
    {
        float shadow_factor = textureProj(tex_shadow, fs_in.shadow_coord);
        if(shadow_factor>0.5)
        {
            color = vec4(0.64, 0.57, 0.49, 1.0); 
        }
        else
        {
            color = vec4(0.41, 0.36, 0.37, 1.0); 
        }
    }
}      