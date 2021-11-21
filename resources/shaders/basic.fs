#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

struct DirLight {
   vec3 direction;

   vec3 specular;
   vec3 diffuse;
   vec3 ambient;
};

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D depthMap;
    sampler2D normalMap;

    float shininess;
};

uniform Material material;
uniform PointLight pointLight;
uniform DirLight dirLight;
uniform bool flag; // if there be no normal map, then use standard Blinn-Phong lighting calculations

uniform vec3 viewPosition;

uniform float heightScale;
uniform bool parallax; // if we successfully loaded the height map, then we proceed with Parallax Mapping

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
  // ambient
    vec3 ambient = light.ambient * texture(material.texture_diffuse1, fs_in.TexCoords).rgb;

    // diffuse
    // vec3 lightDir = normalize(light.position - FragPos);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.texture_diffuse1, fs_in.TexCoords).rgb;

    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.texture_specular1, fs_in.TexCoords).rgb;

    vec3 result = ambient + diffuse + specular;
    return result;
}
// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, bool flag) {
    vec3 lightDir;
    if(!flag) {
        lightDir = normalize(light.position - fragPos);
    } else {
        lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    }
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, fs_in.TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, fs_in.TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, fs_in.TexCoords).xxx);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(material.depthMap, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(material.depthMap, currentTexCoords).r;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(material.depthMap, prevTexCoords).r - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}


void main() {
    vec3 normal;
    if(flag) {
        normal = texture(material.normalMap, fs_in.TexCoords).rgb;
        normal = normalize(normal * 2.0 - 1.0);
    } else {
        normal = normalize(fs_in.Normal);
    }
    vec3 viewDir;
    if(!flag) {
        viewDir = normalize(viewPosition - fs_in.FragPos);
    } else {
        viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    }
    vec2 texCoords = fs_in.TexCoords;
        if(parallax) {
            texCoords = ParallaxMapping(fs_in.TexCoords, viewDir);
            if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
                discard;
        }
    vec3 result = CalcPointLight(pointLight, normal, fs_in.FragPos, viewDir, flag);
    result += CalcDirLight(dirLight, normal, viewDir);
   //vec3 result = texture(material.texture_diffuse1, TexCoords).rgb;
    FragColor = vec4(result, 1.0);
}