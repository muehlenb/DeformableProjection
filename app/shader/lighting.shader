/** The ambient color: */
uniform vec4 ambient_color = vec4(0.0, 0.0, 0.0, 1.0);

/** The texture of the mesh which is used for rendering: */
uniform sampler2D materialColorTexture;

/** Should the texture be used? */
uniform bool useMaterialColorTexture = false;

/** The shininess of the mesh: */
uniform float shininess = 0;

// Structure for point lights:
struct Light {
    vec4 position;
	float range;
    vec4 color;
};

// Predefined point lights:
uniform Light lights[10];

// Number of lights that should actually be used:
uniform int light_num = 0;

/**
 * Blinn-Phong-Lightingmodel implementation.
 * Requires: normal_from_vs, position_from_vs & texCoord_from_vs.
 */
vec4 calculateLight(vec4 albedoColor) {
	// Using ights[j].range as d_max

    vec3 k = albedoColor.rgb;

    // Multiply the materialColorTexture with k, if it's available:
    if (useMaterialColorTexture){
        k *= texture(materialColorTexture, texCoord_from_vs).rgb;
    }

    vec3 I_out_temp = vec3(0.0);
    vec3 normal = normalize(normal_from_vs);

    for (int j = 0; j < light_num; j++){
        Light light = lights[j];
        // Distance from light:
        float dist = distance(light.position.xyz, position_from_vs.xyz);

        // Skip if out of the lights range:
        if (dist >= light.range) continue;

        // Diffuse light:
        vec3 l_j = normalize(light.position.xyz - position_from_vs.xyz);
        float lightMax = max(0.0, dot(normal, l_j));

        // Specular light:
        vec3 h_j = normalize(l_j - normalize(position_from_vs.xyz));
        float lightHalf = pow(max(0.0, dot(normal, h_j)), shininess);

        // Light strength weakening by distance: 
        float lightIntensity = pow(1.0 - (dist / light.range), 2);

        vec3 lightBase = k * light.color.xyz * lightIntensity;

        I_out_temp += lightBase * lightMax  + lightBase * lightHalf;
    }
	// I_out:
    return vec4(k * ambient_color.xyz + I_out_temp, 1.0);
}
