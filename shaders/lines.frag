#version 330 core

in vec3  fvertex;
in vec3  fnormal;

uniform vec3  matdiff;
uniform vec3  matspec;
uniform float matshin;
uniform float alpha = 1;
uniform bool shading = true;

const vec3 ambientLight = vec3(0.2, 0.2, 0.2);
const int MAX_LIGHTS = 4;
uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform int  numLights;

out vec4 FragColor;


vec3 Ambient() {
    return ambientLight*matdiff;
}

vec3 Lambert(vec3 NormSCO, vec3 L, int i)
{
    vec3 colRes = vec3(0);
    if (dot(L, NormSCO) > 0)
        colRes = lightColor[i] * matdiff * dot(L, NormSCO);
    return (colRes);
}

vec3 Phong(vec3 NormSCO, vec3 L, vec3 vertSCO, int i)
{
    vec3 colRes = vec3(0);
    if (dot(NormSCO,L) < 0 || matshin == 0)
        return colRes;

    vec3 R = reflect(-L, NormSCO);
    vec3 V = normalize(-vertSCO);
    float shine = pow(max(0.0, dot(R, V)), matshin);
    return (colRes + matspec * lightColor[i] * shine);
}


void main()
{  
    if (shading) {
        vec3 color = Ambient();
        for (int i = 0; i < numLights; ++i) {
            vec3 L = normalize(lightPos[i] - fvertex);
            vec3 N = normalize(fnormal);
            color +=  Lambert(N, L, i) +
                      Phong  (N, L, fvertex, i);
        }
        FragColor = vec4(color, alpha);
    }
    else {
        FragColor = vec4(matdiff.rgb, alpha);
    }
}
