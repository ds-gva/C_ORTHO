#version 330 core

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;
in float vType;
in vec2 vWorldPos;  // World position for lighting

uniform sampler2D uTexture;

// Lighting uniforms
uniform int uLightingEnabled;
uniform int uAdaptiveLights;  // Scale point lights based on ambient brightness
uniform vec3 uAmbient;
uniform int uLightCount;
uniform vec2 uLightPos[16];
uniform vec3 uLightColor[16];
uniform float uLightRadius[16];
uniform float uLightIntensity[16];

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    vec4 baseColor = texColor * vColor;
    float alpha = baseColor.a;

    // --- TYPE 1: SOLID CIRCLE ---
    if (vType > 0.9 && vType < 1.1) {
        float dist = distance(vTexCoord, vec2(0.5));
        float delta = 0.01;
        alpha = smoothstep(0.5, 0.5 - delta, dist);
    }
    
    // TYPE 2: HOLLOW CIRCLE (for collision visibility)
    else if (vType > 1.9 && vType < 2.1) {
        float dist = distance(vTexCoord, vec2(0.5));
        float delta = 0.01;
        float border = 0.05;
        float outer = smoothstep(0.5, 0.5 - delta, dist);
        float inner = smoothstep(0.5 - border, 0.5 - border - delta, dist);
        alpha = outer - inner;
    }

    // TYPE 3: HOLLOW RECT (for collision visibility)
    else if (vType > 2.9 && vType < 3.1) {
        float border = 0.05;
        float mask = 0.0;
        if (vTexCoord.x < border || vTexCoord.x > 1.0 - border) mask = 1.0;
        if (vTexCoord.y < border || vTexCoord.y > 1.0 - border) mask = 1.0;
        alpha = mask;
    }

    // === LIGHTING CALCULATION ===
    
    vec3 finalLight;
    if (uLightingEnabled == 0) {

        finalLight = vec3(1.0); // Full brightness

    } else {
        finalLight = uAmbient;
        
        // Calculate ambient brightness for adaptive scaling
        float ambientBrightness = (uAmbient.r + uAmbient.g + uAmbient.b) / 3.0;
        
        // Adaptive scaling: point lights contribute less when ambient is bright
        // At ambient=0: scale=1.0 (full contribution)
        // At ambient=1: scale≈0.2 (minimal contribution)
        float adaptiveScale = 1.0;
        if (uAdaptiveLights == 1) {
            adaptiveScale = 1.0 / (1.0 + ambientBrightness * 4.0);
        }
        
        if (uLightCount > 0) {
            for (int i = 0; i < uLightCount; i++) {
                float dist = distance(vWorldPos, uLightPos[i]);
                
                // Smooth falloff: 1 at center, 0 at radius edge
                float attenuation = 1.0 - smoothstep(0.0, uLightRadius[i], dist);
                
                // Add this light's contribution (scaled if adaptive is on)
                finalLight += uLightColor[i] * attenuation * uLightIntensity[i] * adaptiveScale;
            }
        }
    }
    
    // Clamp to prevent over-saturation (remove clamp for bloom effects later)
    finalLight = clamp(finalLight, 0.0, 1.5);
    
    // Apply lighting to the base color
    vec3 litColor = baseColor.rgb * finalLight;
    
    // Final output
    FragColor = vec4(litColor, alpha);
    
    if (FragColor.a < 0.01) discard;
}
