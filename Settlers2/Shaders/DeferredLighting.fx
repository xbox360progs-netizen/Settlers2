//--------------------------------------------------------------------------------------
// DeferredLighting.fx - Deferred lighting pass shader
// Supports: Directional, Point, and Spot lights
//--------------------------------------------------------------------------------------

#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 8

// G-Buffer textures from MRT
texture g_gBufferPos : register(c0);
texture g_gBufferNormal : register(c1);
texture g_gBufferAlbedo : register(c2);
texture g_gBufferSpec : register(c3);
texture g_gBufferDepth : register(c4);

sampler GBufferPosSampler = sampler_state {
    Texture = <g_gBufferPos>;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
    AddressU = Clamp;
    AddressV = Clamp;
};

sampler GBufferNormalSampler = sampler_state {
    Texture = <g_gBufferNormal>;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
    AddressU = Clamp;
    AddressV = Clamp;
};

sampler GBufferAlbedoSampler = sampler_state {
    Texture = <g_gBufferAlbedo>;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
    AddressU = Clamp;
    AddressV = Clamp;
};

sampler GBufferSpecSampler = sampler_state {
    Texture = <g_gBufferSpec>;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
    AddressU = Clamp;
    AddressV = Clamp;
};

sampler GBufferDepthSampler = sampler_state {
    Texture = <g_gBufferDepth>;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
    AddressU = Clamp;
    AddressV = Clamp;
};

// World-View-Projection matrix
float4x4 WVP : register(c0);

// Screen resolution for UV calculation
float2 screenResolution : register(c1);

// Light direction (placeholder - can be replaced with point/spot lights)
float3 lightDir : register(c2) = float3(0.5f, -0.7f, 0.5f);
float3 lightColor : register(c3) = float3(1.0f, 0.95f, 0.9f);

// Ambient light
float3 ambientColor : register(c4) = float3(0.1f, 0.1f, 0.15f);

// Debug mode: 0=normal, 1=albedo, 2=normal, 3=depth, 4=specular, 5=lighting
int debugMode : register(c5) = 0;

// Point lights
float3 pointLightPositions[MAX_POINT_LIGHTS] : register(c10);
float3 pointLightColors[MAX_POINT_LIGHTS] : register(c26);
float pointLightIntensities[MAX_POINT_LIGHTS] : register(c42);
float pointLightRadii[MAX_POINT_LIGHTS] : register(c58);
int pointLightCount = 0;

// Spot lights
float3 spotLightPositions[MAX_SPOT_LIGHTS] : register(c74);
float3 spotLightDirections[MAX_SPOT_LIGHTS] : register(c82);
float3 spotLightColors[MAX_SPOT_LIGHTS] : register(c90);
float spotLightIntensities[MAX_SPOT_LIGHTS] : register(c98);
float spotLightRadii[MAX_SPOT_LIGHTS] : register(c106);
float spotLightInnerAngles[MAX_SPOT_LIGHTS] : register(c114);
float spotLightOuterAngles[MAX_SPOT_LIGHTS] : register(c122);
int spotLightCount = 0;

struct VS_INPUT {
    float3 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

VS_OUTPUT RenderSceneVS(VS_INPUT In) {
    VS_OUTPUT Out;
    Out.Pos = mul(float4(In.Pos, 1.0f), WVP);
    Out.Tex = In.Tex;
    return Out;
}

// PBR-like lighting calculation
float3 CalculateLight(float3 lightDir, float3 lightColor, float lightIntensity,
                     float3 normal, float3 albedo, float gloss, float specStrength,
                     float3 worldPos, float3 viewDir) {
    float NdotL = max(dot(normal, -lightDir), 0.0f);
    
    float3 diffuse = albedo * NdotL * lightColor * lightIntensity;
    
    float3 halfDir = normalize(-lightDir + viewDir);
    float NdotH = max(dot(normal, halfDir), 0.0f);
    float specPower = exp2(10.0f * gloss + 2.0f);
    float specular = pow(NdotH, specPower) * specStrength;
    
    return diffuse + specular * lightColor * lightIntensity;
}

// Point light attenuation
float GetPointLightAttenuation(float3 lightPos, float3 worldPos, float radius) {
    float3 diff = lightPos - worldPos;
    float dist = length(diff);
    float atten = 1.0 - saturate(dist / radius);
    return atten * atten;
}

// Spot light factor
float GetSpotLightFactor(float3 lightPos, float3 lightDir, float3 worldPos,
                         float innerAngle, float outerAngle) {
    float3 toLight = normalize(lightPos - worldPos);
    float cosAngle = dot(toLight, -lightDir);
    
    float inner = cos(innerAngle);
    float outer = cos(outerAngle);
    
    return smoothstep(outer, inner, cosAngle);
}

float4 RenderScenePS(VS_OUTPUT In) : COLOR0 {
    float2 uv = In.Tex;

    float4 posData = tex2D(GBufferPosSampler, uv);
    float4 normalData = tex2D(GBufferNormalSampler, uv);
    float4 albedo = tex2D(GBufferAlbedoSampler, uv);
    float4 specData = tex2D(GBufferSpecSampler, uv);
    float depth = tex2D(GBufferDepthSampler, uv).r;

    // DEBUG VIEWS
    if (debugMode == 1) {
        return float4(albedo.rgb, 1.0f);
    }
    if (debugMode == 2) {
        float3 n = normalData.rgb * 0.5f + 0.5f;
        return float4(n, 1.0f);
    }
    if (debugMode == 3) {
        return float4(depth, depth, depth, 1.0f);
    }
    if (debugMode == 4) {
        return float4(specData.rgb, 1.0f);
    }

    if (depth > 0.9999f) {
        return float4(ambientColor * 0.5f + 0.2f, 1.0f);
    }

    float3 worldPos = posData.rgb;
    float3 normal = normalData.rgb * 2.0f - 1.0f;

    float gloss = specData.a;
    float specStrength = specData.r;

    float3 viewDir = normalize(float3(0.0f, 0.0f, 1.0f));

    float3 totalLighting = ambientColor;

    // Directional light
    totalLighting += CalculateLight(lightDir, lightColor, 1.0f, normal, albedo.rgb, gloss, specStrength, worldPos, viewDir);

    // Point lights
    for (int i = 0; i < pointLightCount; i++) {
        float3 toLight = pointLightPositions[i] - worldPos;
        float3 lightDirN = normalize(toLight);
        
        float atten = GetPointLightAttenuation(pointLightPositions[i], worldPos, pointLightRadii[i]);
        
        if (atten > 0.0f) {
            totalLighting += CalculateLight(lightDirN, pointLightColors[i], pointLightIntensities[i],
                                           normal, albedo.rgb, gloss, specStrength, worldPos, viewDir) * atten;
        }
    }

    // Spot lights
    for (int i = 0; i < spotLightCount; i++) {
        float3 toLight = spotLightPositions[i] - worldPos;
        float3 lightDirN = normalize(toLight);
        
        float atten = GetPointLightAttenuation(spotLightPositions[i], worldPos, spotLightRadii[i]);
        float spot = GetSpotLightFactor(spotLightPositions[i], spotLightDirections[i], worldPos,
                                       spotLightInnerAngles[i], spotLightOuterAngles[i]);
        
        if (atten > 0.0f && spot > 0.0f) {
            totalLighting += CalculateLight(lightDirN, spotLightColors[i], spotLightIntensities[i],
                                           normal, albedo.rgb, gloss, specStrength, worldPos, viewDir) * atten * spot;
        }
    }

    float3 finalColor = totalLighting * albedo.rgb;

    // debugMode == 5 shows lighting only (no albedo)
    if (debugMode == 5) {
        return float4(totalLighting, 1.0f);
    }

    return float4(finalColor, 1.0f);
}

technique LightingTech {
    pass P0 {
        VertexShader = compile vs_3_0 RenderSceneVS();
        PixelShader = compile ps_3_0 RenderScenePS();

        AlphaBlendEnable = FALSE;
        ZEnable = FALSE;
        ZWriteEnable = FALSE;
        CullMode = NONE;
    }
}