#version 410

layout(location = 0) out vec4 fragColor;

uniform samplerCube tex_cubemap;
uniform sampler2DShadow tex_shadow;

//uniform vec3 diffuse_albedo = vec3(0.35);
//uniform vec3 specular_albedo = vec3(0.7);         
//uniform float specular_power = 200.0;

uniform vec3 Ka; //ambient_albedo
uniform vec3 Kd; //diffuse_albedo
uniform vec3 Ks; //specular_albedo
uniform float Ns; //specular_power

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 V; // eye space halfway vector
    vec3 r_N;
    vec3 r_V;
    vec2 texcoord;
    vec4 shadow_coord;
} vertexData;

void main()
{
    // Normalize the incoming N, L and V vectors                               
    vec3 N = normalize(vertexData.N);                                               
    vec3 L = normalize(vertexData.L);                                               
    vec3 V = normalize(vertexData.V);                                               
    vec3 H = normalize(L + V);                                                 
                                                                               
    // Compute the diffuse and specular components for each fragment          
	vec3 ambient = 0.1f * Ka;
    vec3 diffuse = 2.0f * max(dot(N, L), 0.0) * Kd;
    vec3 specular = pow(max(dot(N, H), 0.0), Ns) * Ks;
                                                                           
    // Write final color to the framebuffer
    float shadow_factor = textureProj(tex_shadow, vertexData.shadow_coord);

    vec4 blinn_phone_color = vec4(ambient + diffuse + specular, 1.0);

    // fragColor = blinn_phone_color;
    // fragColor = vec4(shadow_factor);

    vec3 r = reflect(normalize(vertexData.r_V), normalize(vertexData.r_N));
    vec4 reflect_color = texture(tex_cubemap, r);
    
    fragColor = mix(blinn_phone_color, reflect_color, 0.35) * max(shadow_factor, 0.2);
}