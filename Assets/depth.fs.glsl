#version 410 core                               
                                                
out vec4 fragColor;                             
                                                
void main()                                     
{                                               
    fragColor = vec4(vec3(gl_FragCoord.z), 1.0);
}                                               