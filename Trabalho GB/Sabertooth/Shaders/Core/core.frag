#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

#define MAX_LIGHTS 8

struct Material {
    vec3 ka;
    vec3 kd;
    vec3 ks;
    float shininess;
    bool  hasTexture;
};

struct Light {
    vec3 position;
    vec3 color;
};

uniform Material material;
uniform sampler2D texSampler;

// lights
uniform int lightCount;
uniform Light lights[MAX_LIGHTS];
uniform bool lightEnabled[MAX_LIGHTS]; // habilita/desabilita por luz
uniform bool globalLightEnabled;        // desligar todas as luzes de uma vez

uniform vec3 cameraPos;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - FragPos);

    vec3 result = vec3(0.0);

    if (globalLightEnabled)
    {
        int count = clamp(lightCount, 0, MAX_LIGHTS);
        for (int i = 0; i < count; ++i)
        {
            if (!lightEnabled[i]) continue;

            Light L = lights[i];
            vec3 lightDir = normalize(L.position - FragPos);

            // ambient: use a small ambient factor of light color
            vec3 ambient = material.ka * (0.2 * L.color);

            // diffuse
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = material.kd * diff * L.color;

            // specular (Phong)
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(material.shininess, 1.0));
            vec3 specular = material.ks * spec * L.color;

            // attenuation (optional simple)
            float distance = length(L.position - FragPos);
            float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance); // constants for nicer falloff

            result += (ambient + diffuse + specular) * attenuation;
        }
    }
    else
    {
        // if global off, use only ambient from material (or background)
        result = material.ka * 0.1;
    }

    // textura (note: seu mesh não tem texcoords; se tiver, altere para usar texcoords)
    if (material.hasTexture)
    {
        // fallback texture coords: proj on XZ (temporary)
        vec2 uv = vec2(FragPos.x, FragPos.z);
        vec4 texColor = texture(texSampler, uv);
        result *= texColor.rgb;
    }

    // gamma/tonemap could be added here
    FragColor = vec4(result, 1.0);
}