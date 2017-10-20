#version 440 core
in vec2 uv;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler3D texture;
layout (binding = 1) uniform sampler3D disparity_texture;
layout (location = 0) uniform ivec2 camera;


bool in_unit_range(float f)
{
	return f >= 1/512.0 && f <= 1.0f-1.0/512.0;
}

bool in_unit_range(vec2 p)
{
	return in_unit_range(p.x) && in_unit_range(p.y);
}


#define num_images 9.0f
vec4 lookup(ivec2 cam, vec2 uv){

    float z = (cam.x+cam.y*num_images + 0.5) * (1.0/(num_images*num_images));
    return texture(texture,vec3(uv,z));
}
vec4 lookup_d(ivec2 cam, vec2 uv){

    float z = (cam.x+cam.y*num_images + 0.5) * (1.0/(num_images*num_images));
    return texture(disparity_texture,vec3(uv,z));
}

int num_cams = 9;

void main(){
	color = lookup(camera,uv);
	vec4 disp = lookup_d(camera,uv);
	if(disp.r != 0.0) 
	{
		color = disp;
		return;		
	}


	float disparity;
	float min_cost=999999;
	float inv_512 = 1.0/512.0;


	vec3 v_base = lookup(camera, uv).rgb;

	// variance stuff
	float n =0.0;
	float mean = 0.0f;
	float M2 = 0.0f;

	for (float fraction = -2.0;fraction<=2.0;fraction += 0.01f) {
		float disp = (fraction)*inv_512;
		
		vec3 base = lookup(camera, uv).rgb;
		
		{
			int l = 0;
			float cost =0.0;
			for (int cam_x = 0; cam_x < num_cams; cam_x++) 
			for (int cam_y = 0; cam_y < num_cams; cam_y++){
				ivec2 cam = ivec2(cam_x,cam_y);
				vec2 cam_disp = disp * (camera-cam);
				if (cam != camera && in_unit_range(uv+cam_disp))
				{
					vec3 d = base- lookup(ivec2(cam), vec2(uv + cam_disp)).rgb;
					cost += min(dot(d,d),0.0001);
					++l;
				}
			}
		
			if(l!=0)
			{
				cost /= l;
				if (cost < min_cost) {
					min_cost = cost;
					disparity = disp;
				}

				// variance stuff
				n+=1.0;
				float delta = cost - mean;
				mean += delta / n;
				float delta2 = cost - mean;
				M2 += delta*delta2;
			}
		}
		
	}

	float variance = M2/(n-1.0);
	#define STORE(x)(x*512.0+2.0)/4.0

	if(variance > 0.00001 && min_cost < 0.01||true)
		color = vec4(STORE(disparity));
	else 
		color = vec4(0.3,0.9,0.2,1.0);
}








