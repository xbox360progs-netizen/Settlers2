//--------------------------------------------------------------------------------------
// DeferredLighting.fx - Deferred lighting pass shader
// Samples from G-Buffer and computes final lit image
//--------------------------------------------------------------------------------------

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

float4 RenderScenePS(VS_OUTPUT In) : COLOR0 {
    float2 uv = In.Tex;

    float4 posData = tex2D(GBufferPosSampler, uv);
    float4 normalData = tex2D(GBufferNormalSampler, uv);
    float4 albedo = tex2D(GBufferAlbedoSampler, uv);
    float4 specData = tex2D(GBufferSpecSampler, uv);
    float depth = tex2D(GBufferDepthSampler, uv).r;

    if (depth > 0.9999f) {
        return float4(ambientColor * 0.5f + 0.2f, 1.0f);
    }

    float3 worldPos = posData.rgb;
    float3 normal = normalData.rgb * 2.0f - 1.0f;

    float gloss = specData.a;
    float specStrength = specData.r;

    float3 viewDir = normalize(float3(0.0f, 0.0f, 1.0f));

    float NdotL = max(dot(normal, -lightDir), 0.0f);

    float3 diffuse = albedo.rgb * NdotL * lightColor;

    float3 halfDir = normalize(-lightDir + viewDir);
    float NdotH = max(dot(normal, halfDir), 0.0f);
    float specPower = exp2(10.0f * gloss + 2.0f);
    float specular = pow(NdotH, specPower) * specStrength;

    float3 finalColor = ambientColor + diffuse + specular * lightColor;

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