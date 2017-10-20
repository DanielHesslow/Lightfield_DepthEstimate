#version 440 core


#if 0
const vec2 screen_quad_verts[] = vec2[](
	vec2(-1.0,1.0),
	vec2(-1.0,-1.0),
	vec2(1.0,1.0),
	vec2(1.0,-1.0)
);

out vec2 uv;

void main()
{
	gl_Position = vec4(screen_quad_verts[gl_VertexID],0.0,1.0);
	uv = screen_quad_verts[gl_VertexID]*0.5+0.5;
}

#else 
// so this appearently saves some computation along the diagonal.
// should save roughly 1/512 of the time hehe. so yea. micro optimization because I think it is fun ;)


const vec2 screen_quad_verts[] = vec2[](
	vec2(-1.0,-1.0),
	vec2(3.0,-1.0),
	vec2(-1.0,3)
);

out vec2 uv;

void main()
{
	gl_Position = vec4(screen_quad_verts[gl_VertexID],0.0,1.0);
	uv = screen_quad_verts[gl_VertexID]*0.5+0.5;
}
#endif 
