#version 330 core

layout(location = 0) in vec2 vert_pos;
layout(location = 1) in vec2 vert_tex;
layout(location = 2) in vec4 vert_col;

out vec2 frag_tex;
out vec4 frag_col;

float map(float value, float min1, float max1, float min2, float max2)
{
	return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

void main(void)
{
	gl_Position = vec4(map(vert_pos.x, 0.0, 1280.0, -1.0, 1.0), map(vert_pos.y, 0.0, 720.0, -1.0, 1.0), 0.0, 1.0);
	frag_tex = vert_tex;
	frag_col = vert_col;
}