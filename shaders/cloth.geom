#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

uniform mat4 ProjMatrix;
uniform mat4 ViewMatrix;
uniform mat3 NormalMatrix;

out vec3  fvertex;
out vec3  fnormal;

void main()
{	
    vec3 p1 = gl_in[0].gl_Position.xyz;
    vec3 p2 = gl_in[1].gl_Position.xyz;
    vec3 p3 = gl_in[2].gl_Position.xyz;
    vec3 triNormal = normalize(cross(p2 - p1, p3 - p1));

    for (int i = 0; i < 3; i++) {
        fvertex  = (ViewMatrix * gl_in[i].gl_Position).xyz;
        fnormal  = NormalMatrix * triNormal;
        gl_Position = ProjMatrix * vec4(fvertex, 1);
        EmitVertex();
    }
    EndPrimitive();
}
