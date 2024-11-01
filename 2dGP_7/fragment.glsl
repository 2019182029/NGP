#version 330 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec4 objectColor;
uniform vec3 viewPos;
uniform vec3 amb;

void main() {
    vec3 ambientLight = amb; // Ambient light intensity
    vec3 ambient = ambientLight * lightColor;

    vec3 normalVector = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diffuseLight = max(dot(normalVector, lightDir), 0.0);
    vec3 diffuse = diffuseLight * lightColor;

    int shininess = 256;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normalVector);
    float specularLight = max(dot(viewDir, reflectDir), 0.0);
    specularLight = pow(specularLight, float(shininess));
    vec3 specular = specularLight * lightColor;
    
    vec4 result = vec4(ambient + (diffuse + specular) ,1.0) * objectColor;

    FragColor = result;
}
