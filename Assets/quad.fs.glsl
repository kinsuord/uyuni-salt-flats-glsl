#version 420 core                                                                                                                                                                                       

out vec4 color;                                                               

uniform int drawshadow;
layout(binding=0) uniform sampler2DShadow tex_shadow;
layout(binding=1) uniform sampler2D tex;


in VS_OUT                                                                     
{                                                                             
    vec2 texcoord;
    vec4 shadow_coord;                                                           
} fs_in;                                                                      
                                                                            
void main(void)                                                               
{       
    if(drawshadow==0)
    {
//        color = vec4(0.64, 0.57, 0.49, 1.0);                                        
		color = vec4(1.0, 0.0, 0.0, 1.0);  
    }
    else
    {
        float shadow_factor = textureProj(tex_shadow, fs_in.shadow_coord);
        if(shadow_factor>0.5)
        {
			color = texture(tex, fs_in.texcoord);  
        }
        else
        {
            //shadow
			color = texture(tex, fs_in.texcoord) * 0.8f;  
        }
    }
}      