#version 450 core
layout(location=0) in  vec3 pos;
layout(location=1) in  vec3 nor;
layout(location=2) in  vec2 uv;
layout(location=3) in  vec4 color;

void main() {
	gl_Position = vec4(pos, 1.0);
}
