//--------------------------------------------------------------------------------------
// SpriteShader.fx - Fixed version for sprite rendering
//--------------------------------------------------------------------------------------

// Matrix transformation (World-View-Projection)
float4x4 WVP : register(c0);

// Sprite texture - MUST match parameter name "g_texture" from C++ code
texture g_texture;

// Sampler
sampler SpriteSampler = sampler_state {
    Texture = <g_texture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

// Input vertex structure - MUST match SpriteVertex in Renderer.h
// SpriteVertex: float3 Pos (12), float2 Tex (8), D3DCOLOR Color (4), float2 Padding (8) = 32 bytes
struct VS_INPUT {
    float3 Pos   : POSITION;      // 12 bytes (matches C++ float x,y,z)
    float2 Tex   : TEXCOORD0;     // 8 bytes (matches C++ float u,v)
    float4 Color : COLOR0;        // 16 bytes (D3DCOLOR expands to float4 in shader)
    float2 Padding : TEXCOORD1;   // 8 bytes (matches C++ float padding[2])
};

struct VS_OUTPUT {
    float4 Pos   : POSITION;
    float4 Color : COLOR0;
    float2 Tex   : TEXCOORD0;
};

// Vertex shader
VS_OUTPUT RenderSceneVS(VS_INPUT In) {
    VS_OUTPUT Out;
    // Convert float3 to float4 for matrix multiplication
    Out.Pos = mul(float4(In.Pos, 1.0f), WVP);
    Out.Tex = In.Tex;
    Out.Color = In.Color;
    return Out;
}

// Pixel shader
float4 RenderScenePS(VS_OUTPUT In) : COLOR0 {
    float4 texColor = tex2D(SpriteSampler, In.Tex);
    // Multiply texture color with vertex color
    return texColor * In.Color;
}

// Technique that ShaderManager looks for (SpriteBatchTech)
technique SpriteBatchTech {
    pass P0 {
        VertexShader = compile vs_3_0 RenderSceneVS();
        PixelShader  = compile ps_3_0 RenderScenePS();

        // Alpha blending states
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        ZEnable = FALSE;
        CullMode = NONE;
    }
}
