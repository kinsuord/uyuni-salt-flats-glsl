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

const float Br = 0.0025;
const float Bm = 0.0003;
const float g =  0.9800;
const vec3 nitrogen = vec3(0.650, 0.570, 0.475);
const vec3 Kr = Br / pow(nitrogen, vec3(4.0));
const vec3 Km = Bm / pow(nitrogen, vec3(0.84));

float hash(float n)
{
    return fract(sin(n) * 43758.5453123);
}

float noise(vec3 x)
{
    vec3 f = fract(x);
    float n = dot(floor(x), vec3(1.0, 157.0, 113.0));
    return mix(mix(mix(hash(n +   0.0), hash(n +   1.0), f.x),
                    mix(hash(n + 157.0), hash(n + 158.0), f.x), f.y),
                mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
                    mix(hash(n + 270.0), hash(n + 271.0), f.x), f.y), f.z);
}

const mat3 m = mat3(0.0, 1.60,  1.20, -1.6, 0.72, -0.96, -1.2, -0.96, 1.28);
float fbm(vec3 p)
{
    float f = 0.0;
    f += noise(p) / 2; p = m * p * 1.1;
    f += noise(p) / 4; p = m * p * 1.2;
    f += noise(p) / 6; p = m * p * 1.3;
    f += noise(p) / 12; p = m * p * 1.4;
    f += noise(p) / 24;
    return f;
}

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
    vec3 ambient = 0.7f * Kd;
    vec3 diffuse = 0.3f * max(dot(N, L), 0.0) * Kd;
    vec3 specular = pow(max(dot(N, H), 0.0), Ns) * Ks;
                                                                           
    // Write final color to the framebuffer
    float shadow_factor = textureProj(tex_shadow, vertexData.shadow_coord);

    vec4 blinn_phone_color = vec4(ambient + diffuse + specular, 1.0);

    // fragColor = blinn_phone_color;
    // fragColor = vec4(shadow_factor);

    vec3 r = reflect(normalize(vertexData.r_V), normalize(vertexData.r_N));
    vec4 reflect_color = texture(tex_cubemap, r);
    
    fragColor = mix(blinn_phone_color, reflect_color, 0.35) * max(shadow_factor, 0.2);
    fragColor = mix(blinn_phone_color, reflect_color, 0) * max(shadow_factor, 0.2);
    // fragColor = mix(blinn_phone_color, reflect_color, 0);
}