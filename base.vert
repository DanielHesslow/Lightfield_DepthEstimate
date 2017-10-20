#version 330 core

layout(location = 0) in vec3 pos_ms;
layout(location = 1) in vec2 uv_in;
out vec2 uv;
uniform mat4 MVP;



void main(){
    gl_Position = vec4(pos_ms,1);
    uv = uv_in;
}





