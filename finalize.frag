#version 440 core
in vec2 uv;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler3D color_texture;
layout (binding = 1) uniform sampler3D disp_texture;
layout (location = 0) uniform ivec2 camera;
const float num_images = 9.0f;
vec4 lookup_c(ivec2 cam, vec2 uv){
    float z = (cam.x+cam.y*num_images + 0.5) * (1.0/(num_images*num_images));
    return texture(color_texture,vec3(uv,z));
}
vec4 lookup_d(ivec2 cam, vec2 uv){
    float z = (cam.x+cam.y*num_images + 0.5) * (1.0/(num_images*num_images));
    return texture(disp_texture,vec3(uv,z));
}

void main()
{
	vec3 c = lookup_c(camera,uv).rgb;
	float d = lookup_d(camera,uv).r;
	color = vec4(c,d);
}