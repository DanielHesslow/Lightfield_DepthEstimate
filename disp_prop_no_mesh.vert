#version 440 core

// this creates a single triangle strip at
// y = [instance_id,instnace_id+1]
// x = [0,512]
// this is sort of pseudo geometry shader things... I like it :p

// should call this with 1024 verts. 

layout (binding = 0) uniform sampler3D color_texture;
layout (binding = 1) uniform sampler3D disparity_texture;
layout (location = 0) uniform ivec2 camera;
layout (location = 1) uniform ivec2 camera_active;
int num_images = 9;

const float inv_512 = 1.0f/512.0f;

vec4 lookup_d(ivec2 cam, ivec2 uv){
    float z = cam.x+cam.y*num_images;
    return texelFetch(disparity_texture,ivec3(uv,z),0);
}

noperspective out float disparity;
noperspective out float reliability;
noperspective out vec4 vert_disparity;
noperspective out vec4 vert_weight;
noperspective out vec2 uv;

#define LOAD(x)(((x-0.5)*4.0)*inv_512)

void set_nth_component(inout vec4 v, int comp, float f)
{
	// Driver bug madness:
	// seems like it doesn't like dynamic indices(?)
	// I mean it might not be allowed (haven't read the spec carefully) 
	// but then you should atleast issue a warning...
	#if 1
	// what we do to get around it:
	if(comp == 0) v[0] = f;
	else if(comp == 1)v[1] = f;
	else if(comp == 2) v[2] = f;
	else if(comp == 3) v[3] = f;
	#elif 0
	// what we want to do:
	v[comp] = f
	#else
	// what also doesn't work.. has got to be a bug yeah?
	if(comp == 0) v[comp] = f;
	else if(comp == 1)v[comp] = f;
	else if(comp == 2) v[comp] = f;
	else if(comp == 3) v[comp] = f;
	#endif
}

void main()
{
	int x = (gl_VertexID>>1);
	int y = gl_InstanceID + (gl_VertexID&1);
	uv = (vec2(x, y)+0.5)*inv_512;
	vec2 disp = lookup_d(camera_active, ivec2(x,y)).rg;
	disparity = disp.r;
	reliability = disp.g;
	vec2 d = LOAD(disparity)*vec2(camera_active-camera);
	gl_Position = vec4((uv+d)*2.0-1.0, -disparity,1.0);

	vert_disparity = vec4(0);
	vert_weight = vec4(0);

	int idx = gl_VertexID&3;
	#if 0
	// intel seem to have a driver bug, causing this to fail...
	vert_disparity[idx] = disparity;
	vert_weight[idx] = 1.0; 
	#else
	// this seems workaround the problem :/
	set_nth_component(vert_disparity, gl_VertexID&3, disparity);
	set_nth_component(vert_weight, gl_VertexID&3, 1.0);
	#endif
}
