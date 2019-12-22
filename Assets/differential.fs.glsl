#version 410 core                                                             
                                                                            
uniform sampler2D sb_tex;                                                 
uniform sampler2D snoobj_tex;
uniform sampler2D sobj_tex;                                                   

uniform int showmode;

out vec4 color;                                                               
                                                                            
in VS_OUT                                                                     
{                                                                             
    vec2 texcoord;                                                            
} fs_in;                                                                      
                                                                            
void main(void)                                                               
{                 
    switch(showmode)
    {
        case(0):
            color = texture(sb_tex, fs_in.texcoord);
        break;
        case(1):
            color = texture(snoobj_tex, fs_in.texcoord);
        break;
        case(2):				
            color = texture(sobj_tex, fs_in.texcoord);
        break;
        case(3):
            color = texture(sb_tex, fs_in.texcoord) + texture(sobj_tex, fs_in.texcoord) - texture(snoobj_tex, fs_in.texcoord);
        break;
    }                                                           
}      