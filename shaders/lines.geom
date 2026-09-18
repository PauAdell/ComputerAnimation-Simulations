#version 330 core
layout (lines) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 ProjMatrix;
uniform mat4 ViewMatrix;
uniform float radius;

out vec3  fvertex;
out vec3  fnormal;

#define PI 3.1415926538

const vec3 cylinderVerts[18] = vec3[18](
    vec3( 0.000000, 1,  1.000000), vec3( 0.000000,  -1,  1.000000),
    vec3( 0.707107, 1,  0.707107), vec3( 0.707107,  -1,  0.707107),
    vec3( 1.000000, 1,  0.000000), vec3(  1.000000, -1,  0.000000),
    vec3( 0.707107, 1, -0.707107), vec3( 0.707107,  -1, -0.707107),
    vec3( 0.000000, 1, -1.000000), vec3( 0.000000,  -1, -1.000000),
    vec3(-0.707107, 1, -0.707107), vec3(-0.707107,  -1, -0.707107),
    vec3(-1.000000, 1, -0.000000), vec3(-1.000000,  -1, -0.000000),
    vec3(-0.707107, 1,  0.707107), vec3(-0.707107,  -1,  0.707107),
    vec3( 0.000000, 1,  1.000000), vec3( 0.000000,  -1,  1.000000)
);


mat4 rotationMatrix(vec3 axis, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

mat4 translationMatrix(vec3 t)
{
    return mat4(vec4(1.0, 0.0, 0.0, 0.0),
                vec4(0.0, 1.0, 0.0, 0.0),
                vec4(0.0, 0.0, 1.0, 0.0),
                vec4(t, 1.0));
}

mat4 scalingMatrix(vec3 s)
{
    return mat4(s.x, 0.0, 0.0, 0.0,
                0.0, s.y, 0.0, 0.0,
                0.0, 0.0, s.z, 0.0,
                0.0, 0.0, 0.0, 1.0);
}


mat4 getModelMatrix(vec3 dist, vec3 mid)
{
    float d  = length(dist);
    vec3 dir = dist/d;
    float ax = 0.5*PI - asin(dir.y);
    float ay = atan(dir.x, dir.z);

    mat4 m = mat4(1.0);
    m *= translationMatrix(mid);
    m *= rotationMatrix(vec3(0,1,0), -ay);
    m *= rotationMatrix(vec3(1,0,0), -ax);
    m *= scalingMatrix(vec3(radius, 0.5*d, radius));
    return m;
}

void main()
{	
    vec3 p1 = gl_in[0].gl_Position.xyz;
    vec3 p2 = gl_in[1].gl_Position.xyz;
    vec3 dir = p2 - p1;
    vec3 mid = 0.5*(p1 + p2);

    mat4 ModelMatrix = getModelMatrix(dir, mid);
    mat4 ModelViewMatrix = ViewMatrix * ModelMatrix;
    mat3 NormalMatrix = inverse(transpose(mat3(ModelViewMatrix)));	

    for (int i = 0; i < 18; i++) {
        fvertex  = (ModelViewMatrix * vec4(cylinderVerts[i], 1)).xyz;
        fnormal  = normalize(NormalMatrix * (cylinderVerts[i]*vec3(1,0,1)));
        gl_Position = ProjMatrix * vec4(fvertex, 1);
        EmitVertex();
    }
    EndPrimitive();
}
