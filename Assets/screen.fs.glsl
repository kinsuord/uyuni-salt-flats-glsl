#version 420

out vec4 fragColor;

in vec2 TexCoords;

uniform sampler2D tex;

void main()
{
	vec4 texColor = texture(tex, TexCoords);

	fragColor = texColor;

}