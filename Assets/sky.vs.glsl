  #version 410 core
  
  out vec3 pos;
  uniform mat4 P;
  uniform mat4 V;
  
  uniform float sun_time = 0.0;

  const vec2 data[4] = vec2[](
    vec2(-1.0,  1.0), vec2(-1.0, -1.0),
    vec2( 1.0,  1.0), vec2( 1.0, -1.0));

  void main()
  {
    gl_Position = vec4(data[gl_VertexID], 1.0, 1.0);
    // pos = transpose(mat3(V)) * (inverse(P) * gl_Position).xyz;
    vec4 p = inverse(P*V) * gl_Position;
    // p /= p.w;
    pos = p.xyz;

    // vec4[4] vertices = vec4[4](vec4(-1.0, -1.0, 1.0, 1.0), 
    //                         vec4( 1.0, -1.0, 1.0, 1.0), 
    //                         vec4(-1.0,  1.0, 1.0, 1.0), 
    //                         vec4( 1.0,  1.0, 1.0, 1.0));
                                                
    // vec4 p = inv_vp_matrix * vertices[gl_VertexID];        
    // p /= p.w;                                              
    // vs_out.tc = normalize(p.xyz - eye);                    

    // gl_Position = vertices[gl_VertexID]; 
  }