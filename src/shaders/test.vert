#version 330 core

layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_color;
layout (location = 2) in vec2 vertex_texture_coord;

out vec4 color;
out vec2 texture_coord;

void main() {
    gl_Position = vec4(vertex_pos.x, vertex_pos.y, vertex_pos.z, 1.0);
    color = vec4(vertex_color, 1.0);
    texture_coord = vertex_texture_coord;
}