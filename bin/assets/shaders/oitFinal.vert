#version 420 core

uniform mat4 MVP;

layout(location = 0) in vec2 vertex;

void main()
{
  gl_Position =  MVP * vec4(vertex,0,1);
}
