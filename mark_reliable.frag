#version 440 core
in vec2 uv;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler3D color_texture;
layout (binding = 1) uniform sampler3D disparity_texture;
layout (location = 0) uniform ivec2 camera;
layout (location = 1) uniform int fractions;
#define num_images 9.0f

vec4 lookup_d(ivec2 cam, vec2 uv){
    float z = (cam.x+(cam.y)*num_images + 0.5) * (1.0/(num_images*num_images));
    return texture(disparity_texture,vec3(uv,z));
}

vec4 lookup_c(ivec2 cam, vec2 uv){
    float z = (cam.x+(cam.y)*num_images + 0.5) * (1.0/(num_images*num_images));
    return texture(color_texture,vec3(uv,z));
}
bool in_unit_range(float f)
{
	return f >= 0.0f && f <= 1.0f;
}
bool in_unit_range(vec2 p)
{
	return in_unit_range(p.x) && in_unit_range(p.y);
}
#define LOAD(x)(((x-0.5)*4.0)/512.0)

void main(){

	float disp = lookup_d(camera,uv).a;
	float real_disp = LOAD(disp);

	float reliability=0.0f; //higher is worse
	float i = 0.0f;
	#if 0
	for(int cam_x = 0; cam_x<num_images;cam_x++)
	for(int cam_y = 0; cam_y<num_images;cam_y++)
	{
		vec2 nuv = real_disp*(camera-ivec2(cam_x,cam_y));
		float tmp = (lookup_d(ivec2(cam_x,cam_y),uv+nuv).a-disp);
		++i;
		reliability += tmp *tmp;
	}
	#else
	for(int cam_x = 0; cam_x<num_images;cam_x++)
	{
		vec2 nuv = real_disp*(camera-ivec2(cam_x,camera.y));
		if(in_unit_range(nuv+uv))
		{
			float tmp = (lookup_d(ivec2(cam_x,camera.y),uv+nuv).a-disp);
			++i;
			reliability += tmp *tmp;
		}
	}
	for(int cam_y = 0; cam_y<num_images;cam_y++)
	{
		vec2 nuv = real_disp*(camera-ivec2(camera.x,cam_y));
		if(in_unit_range(nuv+uv))
		{
			float tmp = (lookup_d(ivec2(camera.x,cam_y),uv+nuv).a-disp);
			++i;
			reliability += tmp *tmp;
		}
	}
	#endif

	color = vec4(disp,reliability/i,disp,disp);
}





