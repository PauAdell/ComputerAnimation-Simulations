#version 330 core

in vec3  vertex;

uniform mat4 ProjMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;

void main()
{	
    gl_Position = ProjMatrix * ViewMatrix * ModelMatrix * vec4 (vertex, 1.0);
}
