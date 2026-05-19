//--------------------------------------------------------------------------------------
// SpriteGBuffer.fx - Sprite geometry pass shader for deferred rendering
// Supports normal mapping for buildings, terrain, and units
// Writes to MRT: Albedo, Normal, Material, Depth
//--------------------------------------------------------------------------------------

// Matrix transformation (World-View-Projection)
float4x4 WVP : register(c0);

// Sprite texture (diffuse/albedo)
texture g_texture;

// Normal map (optional - for better lighting)
texture g_normalMap;

// Material map (optional - R=specular, G=roughness, B=AO, A=emissive)
texture g_materialMap;

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

sampler MaterialSampler = sampler_state {
    Texture = <g_materialMap>;
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
    float3 Tangent : TEXCOORD3;
    float3 Bitangent : TEXCOORD4;
};

// Vertex shader - outputs WorldPos, Normal, Tangent, Bitangent for GBuffer
VS_OUTPUT RenderSceneVS(VS_INPUT In) {
    VS_OUTPUT Out;
    
    float4 worldPos = mul(float4(In.Pos, 1.0f), WVP);
    Out.Pos = worldPos;
    Out.Tex = In.Tex;
    Out.Color = In.Color;
    
    // For isometric sprites facing camera:
    // Normal points toward camera (0, 0, 1) in view space
    // Tangent is along sprite's right edge (1, 0, 0)
    // Bitangent is along sprite's up edge (0, 1, 0)
    // These are in screen-space, transformed to world space
    Out.Normal = float3(0.0f, 0.0f, 1.0f);
    Out.Tangent = float3(1.0f, 0.0f, 0.0f);
    Out.Bitangent = float3(0.0f, 1.0f, 0.0f);
    
    // Store world position for depth reconstruction
    Out.WorldPos = In.Pos;
    
    return Out;
}

// For terrain tiles - use computed normals from height
VS_OUTPUT TerrainVS(VS_INPUT In) {
    VS_OUTPUT Out;
    
    float4 worldPos = mul(float4(In.Pos, 1.0f), WVP);
    Out.Pos = worldPos;
    Out.Tex = In.Tex;
    Out.Color = In.Color;
    
    // Terrain normal - usually flat (facing up in world space)
    Out.Normal = float3(0.0f, 0.0f, 1.0f);
    Out.Tangent = float3(1.0f, 0.0f, 0.0f);
    Out.Bitangent = float3(0.0f, 1.0f, 0.0f);
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

// Decode normal from [0,1] to [-1,1] range
float3 DecodeNormal(float3 encoded) {
    return encoded * 2.0f - 1.0f;
}

// Build TBN matrix for normal mapping
float3x3 BuildTBN(float3 N, float3 T, float3 B) {
    return float3x3(T, B, N);
}

// Sprite normal mapping - handles front-facing billboards
PS_OUTPUT RenderScenePS(VS_OUTPUT In) {
    PS_OUTPUT Out;
    
    // Sample diffuse texture
    float4 texColor = tex2D(SpriteSampler, In.Tex);
    float4 finalColor = texColor * In.Color;
    
    // RT0: Albedo (RGB = color, A = alpha)
    Out.Albedo = finalColor;
    
    // Check if we have a normal map
    float3 normal = In.Normal;
    #ifdef HAS_NORMAL_MAP
    float3 encodedNormal = tex2D(NormalSampler, In.Tex).rgb;
    normal = DecodeNormal(encodedNormal);
    
    // Transform from tangent space to world space
    float3x3 TBN = BuildTBN(In.Normal, In.Tangent, In.Bitangent);
    normal = normalize(mul(normal, TBN));
    #endif
    
    // RT1: Normal - encode to [0,1] range
    Out.Normal = float4(normal * 0.5f + 0.5f, 1.0f);
    
    // RT2: Material - R=specular, G=roughness, B=AO, A=emissive
    float specular = 0.2f;
    float roughness = 0.5f;
    float ao = 1.0f;
    float emissive = 0.0f;
    
    #ifdef HAS_MATERIAL_MAP
    float4 matSample = tex2D(MaterialSampler, In.Tex);
    specular = matSample.r;
    roughness = matSample.g;
    ao = matSample.b;
    emissive = matSample.a;
    #endif
    
    Out.Material = float4(specular, roughness, ao, emissive);
    
    // RT3: Depth - use linear depth
    float depth = In.Pos.w;
    Out.Depth = float4(depth, depth, depth, 1.0f);
    
    // Discard fully transparent pixels
    clip(finalColor.a - 0.01f);
    
    return Out;
}

// Technique for sprite geometry pass
technique SpriteGBufferTech {
    pass P0 {
        VertexShader = compile vs_3_0 RenderSceneVS();
        PixelShader = compile ps_3_0 RenderScenePS();
        
        AlphaBlendEnable = FALSE;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        CullMode = NONE;
        
        RenderTarget[0] = NULL;
        RenderTarget[1] = NULL;
        RenderTarget[2] = NULL;
        RenderTarget[3] = NULL;
    }
}

// Technique for terrain (flat normals)
technique TerrainGBufferTech {
    pass P0 {
        VertexShader = compile vs_3_0 TerrainVS();
        PixelShader = compile ps_3_0 RenderScenePS();
        
        AlphaBlendEnable = FALSE;
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        CullMode = NONE;
        
        RenderTarget[0] = NULL;
        RenderTarget[1] = NULL;
        RenderTarget[2] = NULL;
        RenderTarget[3] = NULL;
    }
}