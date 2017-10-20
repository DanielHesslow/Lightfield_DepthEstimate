#version 440 core

noperspective in float disparity;
noperspective in float reliability;
noperspective in vec3 cc;
noperspective in vec4 vert_disparity;
noperspective in vec4 vert_weight;
noperspective in vec2 uv;
layout (location = 0) out vec4 color;
layout (binding = 0) uniform sampler3D color_texture;

const int num_images = 9;

vec4 lookup_c(ivec2 cam, vec2 uv){
    float z = (cam.x+cam.y*num_images + 0.5) * (1.0/(num_images*num_images));
    return texture(color_texture,vec3(uv,z));
}

layout (location = 0) uniform ivec2 camera;
layout (location = 1) uniform ivec2 camera_active;

void main()
{
	vec3 c_diff = lookup_c(camera,gl_FragCoord.xy).xyz-lookup_c(camera,uv).xyz; 
	float r = dot(c_diff,c_diff); 
	float alpha = .5f;
	r = reliability*(1.-alpha) + r*(alpha);
	vec4 vd = vert_disparity;
	if(vert_weight.x != 0.0) vd.x /= vert_weight.x; // can we speed this up??
	if(vert_weight.y != 0.0) vd.y /= vert_weight.y; // can we speed this up??
	if(vert_weight.z != 0.0) vd.z /= vert_weight.z; // can we speed this up??
	if(vert_weight.w != 0.0) vd.w /= vert_weight.w; // can we speed this up??

	float avg_disp = (vd.x+ vd.y+vd.z+vd.w)*0.3333; // one is zero. cause we've got a triangle.
	vec4 delta = avg_disp-vd;
	
	// avg_disp^2 larger cause of the zero. 
	// 1/3 is just a constant factor so we just change the if-statement instead
	float var_disp = (dot(delta,delta)-avg_disp*avg_disp);  
	

	float rel = exp(-0.001*(var_disp * (1./0.000005) + reliability * (1./0.001)));
	//if(var_disp > 0.000005) discard;
	//if(reliability > 0.001) discard;

	float disp = disparity;
	float abs_rel = rel*(1./81.);
	color = vec4(disp*abs_rel,r*abs_rel,abs_rel,disp);

}