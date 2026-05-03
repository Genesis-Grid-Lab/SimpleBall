#version 330

#define MAX_LIGHTS 16

struct Light
{
    int enabled;
    int type;

    vec3 position;
    vec3 direction;

    vec4 color;

    float intensity;
    float range;
    float spotAngle;
};

uniform int uLightCount;
uniform Light uLights[MAX_LIGHTS];

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform sampler2D shadowMap;
uniform int useShadows;

in vec4 fragPosLightSpace;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.001;

    if (currentDepth - bias > closestDepth)
        return 1.0;

    return 0.0;
}

void main()
{
    vec4 texelColor =
        texture(texture0, fragTexCoord) *
        colDiffuse *
        fragColor;

    vec3 normal = normalize(fragNormal);

    vec3 lighting = vec3(0.1);

    for (int i = 0; i < uLightCount; i++)
    {
        Light light = uLights[i];

        if (light.enabled == 0)
            continue;

        vec3 lightDir;
	float attenuation = 1.0;

        // Directional
        if (light.type == 0)
        {
            lightDir = normalize(-light.direction);
        }
        // Point
        else
        {
            lightDir =
                normalize(light.position - fragPosition);

		float dist = distance(light.position, fragPosition);

		attenuation = clamp(1.0 - (dist / light.range), 0.0, 1.0);
   		 attenuation *= attenuation; // Optionnel: chute plus douce

    if (light.type == 2) { // Spot
        float theta = dot(lightDir, normalize(-light.direction));
	if (theta > light.spotAngle) {
     	   attenuation *= (theta - light.spotAngle) / (1.0 - light.spotAngle); // Bordure douce
   	 } else {
       	     attenuation = 0.0;
    	 }
    }
        }

        float diff =
            max(dot(normal, lightDir), 0.0);

        if (light.type == 1)
        {
            float dist =
                distance(light.position, fragPosition);

            attenuation =
                clamp(1.0 - (dist / light.range), 0.0, 1.0);
        }

        vec3 diffuse =
            light.color.rgb *
            diff *
            attenuation *
            light.intensity;

	    float shadow = 0.0;
        if (useShadows == 1 && light.type == 0) // Only directional light casts shadows
        {
            shadow = ShadowCalculation(fragPosLightSpace);
        }
        diffuse *= (1.0 - shadow);

        lighting += diffuse;
    }

    vec3 final =
        texelColor.rgb * lighting;

    finalColor =
        vec4(final, texelColor.a);
}