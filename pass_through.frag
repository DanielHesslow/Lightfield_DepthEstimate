#version 440 core
in vec2 uv;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler2D texture;

void main()
{
	color = texture(texture,uv);
}