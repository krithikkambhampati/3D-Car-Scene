#version 330 core
// ============================================================
//  fragment.glsl  –  Multi-light Blinn-Phong shading.
//  Supports up to MAX_LIGHTS point lights + optional texture.
// ============================================================

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;

// Material uniforms
uniform vec4      uColor;        // base RGBA color (used when uUseTexture == 0)
uniform int       uUseTexture;   // 1 = sample uTex, 0 = use uColor
uniform sampler2D uTex;
uniform float     uShininess;    // specular shininess exponent

// Lighting uniforms
uniform float uAmbient;          // ambient intensity (0..1)
uniform vec3  uViewPos;          // camera world position
uniform int   uNumLights;        // active lights (max 12)
uniform vec3  uLightPos[12];     // light world positions
uniform vec3  uLightColor[12];   // light colors (RGB)
uniform int   uSpotStart;        // index where building spotlights begin
uniform int   uSpotCount;        // number of active building spotlights
uniform int   uStreetStart;      // index where street lights begin
uniform int   uStreetCount;      // number of street lights in active list
uniform int   uCarStart;         // index where car headlights begin
uniform int   uCarCount;         // number of active car headlights
uniform vec3  uCarLightDir[2];   // normalized beam directions for active car headlights
uniform vec3  uSpotLightDir[12]; // normalized beam directions for active spotlights

void main() {
    // Resolve base color
    vec3 texColor = texture(uTex, vTexCoord).rgb;
    vec3 baseColor = (uUseTexture == 1)
                   ? (texColor * mix(vec3(1.0), uColor.rgb, 0.26))
                   : uColor.rgb;
    float alpha    = uColor.a;

    vec3 norm      = normalize(vNormal);
    vec3 viewDir   = normalize(uViewPos - vFragPos);

    // --- Ambient ---
    vec3 result = uAmbient * vec3(0.92, 0.94, 0.98) * baseColor;

    // --- For each active light: diffuse + specular (Blinn-Phong) ---
    for (int i = 0; i < uNumLights && i < 12; i++) {
        vec3  L       = uLightPos[i] - vFragPos;
        float dist    = length(L);
        vec3  lightDir = normalize(L);

        // Quadratic attenuation (constant + linear + quadratic)
        float attn = 1.0 / (1.0 + 0.04 * dist + 0.003 * dist * dist);

        bool isStreet = (i >= uStreetStart) && (i < (uStreetStart + uStreetCount));
        bool isSpot   = (i >= uSpotStart) && (i < (uSpotStart + uSpotCount));
        bool isCar    = (i >= uCarStart) && (i < (uCarStart + uCarCount));
        if (isSpot) {
            // Swinging rooftop lights behave like directed cones toward the floor.
            int si = clamp(i - uSpotStart, 0, 11);
            vec3 beamDir = normalize(uSpotLightDir[si]);
            vec3 toFragFromLight = normalize(vFragPos - uLightPos[i]);
            float theta = dot(toFragFromLight, beamDir);
            float cone = smoothstep(0.60, 0.90, theta);

            float radialXZ = length(uLightPos[i].xz - vFragPos.xz);
            float local = exp(-radialXZ * radialXZ * 0.022);
            float below = clamp((uLightPos[i].y - vFragPos.y) / max(uLightPos[i].y, 0.001), 0.0, 1.0);

            attn *= cone * local * (0.45 + 1.05 * below);
        }
        if (isStreet) {
            // Keep street lights focused mostly under the pole, not washing the arena.
            float radialXZ = length(uLightPos[i].xz - vFragPos.xz);
            float below    = clamp((uLightPos[i].y - vFragPos.y) / max(uLightPos[i].y, 0.001), 0.0, 1.0);
            float focus    = exp(-radialXZ * radialXZ * 0.085) * (0.42 + 0.85 * below);
            attn *= focus;
        }
        if (isCar) {
            // Headlights are directional cones, not omni/circular bulbs.
            float radialXZ = length(uLightPos[i].xz - vFragPos.xz);
            float yDiff    = abs(vFragPos.y - uLightPos[i].y);
            float local    = exp(-radialXZ * radialXZ * 0.060) * exp(-yDiff * 0.95);

            int ci = i - uCarStart;
            ci = clamp(ci, 0, 1);
            vec3 beamDir = normalize(uCarLightDir[ci]);
            vec3 toFragFromLight = normalize(vFragPos - uLightPos[i]);

            float theta = dot(toFragFromLight, beamDir);
            // Smooth cone: full in center, soft edge, almost zero outside.
            float cone = smoothstep(0.64, 0.92, theta);

            // Extra forward reach and side suppression.
            float forwardBoost = smoothstep(0.58, 0.98, theta);
            attn *= local * cone * (0.80 + 1.20 * forwardBoost);
        }

        // Diffuse
        float diff  = max(dot(norm, lightDir), 0.0);

        // Specular (Blinn-Phong half-vector)
        vec3  H     = normalize(lightDir + viewDir);
        float spec  = pow(max(dot(norm, H), 0.0), uShininess);

        // Balanced highlights: visible lights without overexposure.
        float diffTerm = diff * 1.10;
        float specTerm = spec * 0.48;
        vec3  lightContribution = attn * uLightColor[i] * (diffTerm * baseColor + specTerm * vec3(1.0));

        // Slight additive tint so colored lights read on the ground.
        if (isSpot) {
            lightContribution += attn * uLightColor[i] * (0.22 * diffTerm);
        }
        if (isStreet) {
            lightContribution += attn * uLightColor[i] * (0.16 * diffTerm);
        }

        result += lightContribution;
    }

    // Gentle filmic response to keep contrast and avoid a flat look.
    vec3 mapped = vec3(1.0) - exp(-result * 1.16);
    vec3 color = pow(mapped, vec3(1.0 / 2.10));

    // Lift contrast and saturation so colored lights read better.
    color = (color - vec3(0.5)) * 1.22 + vec3(0.5);
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, 1.22);
    color = clamp(color, 0.0, 1.0);

    FragColor = vec4(color, alpha);
}
