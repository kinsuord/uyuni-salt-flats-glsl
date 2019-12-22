#version 410

layout(location = 0) in vec3 iv3vertex;
layout(location = 1) in vec2 iv2tex_coord;
layout(location = 2) in vec3 iv3normal;

uniform mat4 um4m;
uniform mat4 um4v;
uniform mat4 um4p;
uniform mat4 shadow_matrix; 
uniform vec3 light_pos = vec3(-31.75, 26.05, -97.72);

out VertexData
{
    vec3 N;
    vec3 L;
    vec3 V;
    vec3 r_N;
    vec3 r_V;
    vec2 texcoord;
    vec4 shadow_coord;
} vertexData;

void main()
{
    mat4 um4mv = um4v * um4m;
	gl_Position = um4p * um4mv * vec4(iv3vertex, 1.0);
    vertexData.texcoord = iv2tex_coord;
    vertexData.N = mat3(um4mv) * iv3normal;
    vertexData.L = (um4v * vec4(light_pos, 1.0)).xyz - (um4mv * vec4(iv3vertex, 1.0)).xyz;
    vertexData.V = - (um4mv * vec4(iv3vertex, 1.0)).xyz;

    vec4 pos_vs = um4m * vec4(iv3vertex, 1.0);
    vertexData.r_N = mat3(transpose(inverse(um4m))) * iv3normal;
    vertexData.r_V = pos_vs.xyz;

    vertexData.shadow_coord = shadow_matrix * vec4(iv3vertex, 1.0);
}