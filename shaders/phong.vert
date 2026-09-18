#version 330 core

in vec3  vertex;
in vec3  normal;

uniform mat4 ProjMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;
uniform float normalSign = 1.0;

out vec3  fvertex;
out vec3  fnormal;


void main()
{	
    mat4 ModelViewMatrix = ViewMatrix * ModelMatrix;
    mat3 NormalMatrix = inverse(transpose(mat3(ModelViewMatrix)));
	
    fvertex = (ModelViewMatrix * vec4(vertex, 1)).xyz;
    fnormal = normalize(NormalMatrix * (normalSign * normal));
	
    gl_Position = ProjMatrix * ModelViewMatrix * vec4 (vertex, 1.0);
}
