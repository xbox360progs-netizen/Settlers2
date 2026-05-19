//--------------------------------------------------------------------------------------
// SpriteGBuffer.fx - Sprite geometry pass shader for deferred rendering
// Writes to MRT: Albedo, Normal, Material, Depth
//--------------------------------------------------------------------------------------

// Matrix transformation (World-View-Projection)
float4x4 WVP : register(c0);

// Sprite texture
texture g_texture;

// Normal map (optional - for better lighting)
texture g_normalMap;

// Samplers
sampler SpriteSampler = sampler_state {
    Texture = <g_texture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

sampler NormalSampler = sampler_state {
    Texture = <g_normalMap>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

// Vertex structure matches SpriteVertex (32 bytes)
struct VS_INPUT {
    float3 Pos   : POSITION;
    float2 Tex   : TEXCOORD0;
    float4 Color : COLOR0;
    float2 Padding : TEXCOORD1;
};

struct VS_OUTPUT {
    float4 Pos   : POSITION;
    float4 Color : COLOR0;
    float2 Tex   : TEXCOORD0;
    float3 WorldPos : TEXCOORD1;
    float3 Normal : TEXCOORD2;
};

// Vertex shader - outputs WorldPos and Normal for GBuffer
VS_OUTPUT RenderSceneVS(VS_INPUT In) {
    VS_OUTPUT Out;
    
    float4 worldPos = mul(float4(In.Pos, 1.0f), WVP);
    Out.Pos = worldPos;
    Out.Tex = In.Tex;
    Out.Color = In.Color;
    
    // Approximate sprite normal (flat sprite faces camera)
    // For proper normal maps, this would come from vertex data
    Out.Normal = float3(0.0f, 0.0f, 1.0f);
    
    // Store world position for depth reconstruction
    Out.WorldPos = In.Pos;
    
    return Out;
}

// Pixel shader - writes to 4 render targets (MRT)
struct PS_OUTPUT {
    float4 Albedo : COLOR0;
    float4 Normal : COLOR1;
    float4 Material : COLOR2;
    float4 Depth : COLOR3;
};

PS_OUTPUT RenderScenePS(VS_OUTPUT In) {
    PS_OUTPUT Out;
    
    // Sample texture
    float4 texColor = tex2D(SpriteSampler, In.Tex);
    float4 finalColor = texColor * In.Color;
    
    // RT0: Albedo (RGB = color, A = alpha)
    Out.Albedo = finalColor;
    
    // RT1: Normal - encode to [0,1] range
    float3 normal = In.Normal * 0.5f + 0.5f;
    Out.Normal = float4(normal, 1.0f);
    
    // RT2: Material - R = specular strength, G = glossiness, B = unused, A = flags
    // Default: weak specular, medium gloss
    Out.Material = float4(0.2f, 0.5f, 0.0f, 0.0f);
    
    // RT3: Depth - encode depth in red channel
    // Use linear depth based on screen position
    float depth = In.Pos.w;
    Out.Depth = float4(depth, depth, depth, 1.0f);
    
    // Discard fully transparent pixels (no geometry to write)
    clip(finalColor.a - 0.01f);
    
    return Out;
}

// Technique for sprite geometry pass
technique SpriteGBufferTech {
    pass P0 {
        VertexShader = compile vs_3_0 RenderSceneVS();
        PixelShader = compile ps_3_0 RenderScenePS();
        
        // Disable blending for geometry pass - we write to all pixels
        AlphaBlendEnable = FALSE;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        CullMode = NONE;
        
        // MRT setup - 4 render targets
        RenderTarget[0] = NULL;
        RenderTarget[1] = NULL;
        RenderTarget[2] = NULL;
        RenderTarget[3] = NULL;
    }
}