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
uniform bool lightEnabled[MAX_LIGHTS];
uniform bool globalLightEnabled;
uniform vec3 cameraPos;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 result = vec3(0.0);
    
    if (globalLightEnabled)
    {
        // Adiciona uma luz ambiente global usando Kd (cor real do material)
        vec3 globalAmbient = material.kd * 0.2;
        result += globalAmbient;
        
        int count = clamp(lightCount, 0, MAX_LIGHTS);
        for (int i = 0; i < count; ++i)
        {
            if (!lightEnabled[i]) continue;
            
            Light L = lights[i];
            vec3 lightDir = normalize(L.position - FragPos);
            
            // ambient: usa Kd com fator baixo ao invés de Ka
            vec3 ambient = material.kd * (0.05 * L.color);
            
            // diffuse
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = material.kd * diff * L.color;
            
            // specular (Phong)
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(material.shininess, 1.0));
            vec3 specular = material.ks * spec * L.color;
            
            // attenuation - ajustado para não atenuar tanto de perto
            float distance = length(L.position - FragPos);
            float attenuation = 1.0 / (1.0 + 0.045 * distance + 0.0075 * distance * distance);
            
            result += (ambient + diffuse + specular) * attenuation;
        }
    }
    else
    {
        // Se luzes desligadas, usa Kd (cor difusa) ao invés de Ka
        // porque Ka está branco no arquivo MTL do Blender
        result = material.kd * 0.5;
    }

    if (material.hasTexture)
    {
        vec2 uv = vec2(FragPos.x, FragPos.z);
        vec4 texColor = texture(texSampler, uv);
        result *= texColor.rgb;
    }
    
    result = max(result, vec3(0.0));
    
    FragColor = vec4(result, 1.0);
}