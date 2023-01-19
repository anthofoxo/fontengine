#version 330 core

in vec2 frag_tex;
in vec4 frag_col;

layout(location = 0) out vec4 out_col;

uniform sampler2D u_sampler;

void main(void)
{
	out_col = frag_col;
	float dist = texture(u_sampler, frag_tex).r;
	if(dist < 0.8) discard;
}