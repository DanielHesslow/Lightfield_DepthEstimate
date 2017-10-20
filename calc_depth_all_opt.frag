#version 440 core
in vec2 uv;
layout (location = 0) out vec4 color;

layout (binding = 0) uniform sampler3D texture;
layout (binding = 1) uniform sampler3D disparity_texture;
layout (location = 0) uniform ivec2 camera;
layout (location = 2) uniform float fractions[22];
layout (location = 1 ) uniform int   num_fractions;

bool in_valid_range(int i)
{
	return i >= 0 && i < 512;
}
bool in_valid_range(ivec2 p)
{
	return in_valid_range(p.x) && in_valid_range(p.y);
}

#define num_images 9.0f

vec4 ilookup(ivec2 cam, ivec2 uv){
    float z = cam.x+cam.y*num_images;
    return texelFetch(texture,ivec3(uv,z),0);
}

vec4 ilookup_d(ivec2 cam, ivec2 uv){
    float z = cam.x+cam.y*num_images;
    return texelFetch(disparity_texture,ivec3(uv,z),0);
}


vec4 lookup(ivec2 cam, vec2 uv){

    float z = (cam.x+cam.y*num_images + 0.5) * (1.0/(num_images*num_images));
    return texture(texture,vec3(uv,z));
}

int num_cams = 9;


float eval(float terms[5], float t)
{
	return pow(t,4)*terms[4] + pow(t,3)*terms[3] +pow(t,2)*terms[2] +pow(t,1)*terms[1] + terms[0];
}
float eval(float terms[4], float t)
{
	return pow(t,3)*terms[3] +pow(t,2)*terms[2] +pow(t,1)*terms[1] + terms[0];
}
float eval(float terms[3], float t)
{
	return pow(t,2)*terms[2] +pow(t,1)*terms[1] + terms[0];
}

void solve_min_0_1(float terms[5], out float cost, out float x_out)
{
	
	#if 0
	cost = eval(terms,1.0);
	x_out = 1.0;
	for(float f = 0.0; f <1.0;f+=0.01){
		float tmp = eval(terms,f);
		if(tmp<cost)
		{
			cost = tmp;
			x_out = f;
		}
	}
	#else
	float f_prime[4] = {terms[1],2*terms[2],3*terms[3],4*terms[4]};
	float f_biss[3] = {f_prime[1],2*f_prime[2],3*f_prime[3]};
	
	// runnning Newton–Raphson a couple of iterations
	// starting from either side of the interval
	// this is clearly not exact
	// but in practice it does seem to work just fine!
	// the minima will be the first extrema we hit since the leading term is possitive
	// either from left, right or both
	// assuming that we do not pass it which I feel like we might but honnestly idk
	 
	int itr = 5;
	{
		float x = 1.;
		for(int i = 0; i<itr;i++) x-= eval(f_prime,x)/eval(f_biss,x);
		x = clamp(x,0,1);
		cost = eval(terms,x);
		x_out = x;
	}
	{
		float x = 0.;
		for(int i = 0; i<itr;i++) x-= eval(f_prime,x)/eval(f_biss,x);
		x = clamp(x,0,1);
		float tmp = eval(terms,x);
		if(tmp<cost)
		{
			cost = tmp;
			x_out = x;
		}
	}
	#endif
}

#define STORE(x)(x+2.0)/4.0
#define LOAD(x)(x-0.5)*4.0

void main(){
	double disparity;
	double min_cost=9999;

	vec3 base = lookup(camera, uv).rgb;

	// variance stuff
	double n =0.0;
	double mean = 0.0f;
	double M2 = 0.0f;
	ivec2 iuv = ivec2(uv * 512.0);
	float disp_low = -2.0;
	for(int pixel_index = -2;pixel_index<2;pixel_index++)
	{
		for (int fraction_index = 0; fraction_index < num_fractions; fraction_index++) {
			float fraction = fractions[fraction_index]; 
			float disp_high = fraction + pixel_index;
			
			{
				float term_t0 = 0.0f;
				float term_t1 = 0.0f;
				float term_t2 = 0.0f;
				float term_t3 = 0.0f;
				float term_t4 = 0.0f;
				int l = 0;
				
				for (int cam_x = 0; cam_x < num_cams; cam_x++) 
				for (int cam_y = 0; cam_y < num_cams; cam_y++) {
					ivec2 cam = ivec2(cam_x,cam_y);
					
					vec2 cam_disp_low = disp_low * (camera-cam);
					vec2 cam_disp_high = disp_high * (camera-cam);
					ivec2 px_off = ivec2(floor((cam_disp_low+cam_disp_high)/2.0));
					ivec2 px = iuv + px_off;
					if (cam != camera && in_valid_range(px) && in_valid_range(px+1))
					{
						vec3 a = ilookup(cam, px + ivec2(0,0)).rgb;
						vec3 b = ilookup(cam, px + ivec2(0,1)).rgb;
						vec3 c = ilookup(cam, px + ivec2(1,0)).rgb;
						vec3 d = ilookup(cam, px + ivec2(1,1)).rgb;

						float x = cam_disp_low.x-px_off.x;  
						float y = cam_disp_low.y-px_off.y;  
						float dx = cam_disp_high.x-cam_disp_low.x;
						float dy = cam_disp_high.y-cam_disp_low.y;

						vec3 tmp_5 = b-a+(d+a-b-c)*x;
						vec3 tmp_6 = (d+a-b-c)*dx;
						vec3 tmp_7 = a+(c-a)*x;
						vec3 tmp_8 = (c-a)*dx;

						// color diference as a function over t err = pa*t^2 + pb*t + pc;
						vec3 pa = tmp_6*dy;			      
						vec3 pb = tmp_8+tmp_6*y+tmp_5*dy; 
						vec3 pc = tmp_7+tmp_5*y-base;         

						// square all things 
						// (cause abs is hell, and mean squared error isn't worse than mean error anyway) gives:
						term_t0 +=   dot(pc,pc);   
						term_t1 += 2*dot(pb,pc);
						term_t2 += 2*dot(pa,pc)+dot(pb,pb); 
						term_t3 += 2*dot(pa,pb);    
						term_t4 +=   dot(pa,pa);      
						++l;
					}
				}
				
				{
					double cost,t;
					float terms[5] = {term_t0, term_t1, term_t2, term_t3, term_t4}; 
					solve_min_0_1(terms, cost, t);
					if(l!=0)
					{
						cost /= l;
						if (cost < min_cost) {
							min_cost = cost;
							disparity = mix(disp_low,disp_high,t);
						}

						// variance stuff
						n+=1.0;
						double delta = cost - mean;
						mean += delta / n;
						double delta2 = cost - mean;
						M2 += delta*delta2;
					}
				}
				disp_low = disp_high;
			}
		}
	}

	double variance = M2/(n-1.0);

	if(min_cost < 0.005||true)
		color = vec4(STORE(disparity));
	else 
		color = vec4(0.3,0.9,0.2,1.0);
}








