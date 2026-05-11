//--------------------------------------------------------------------------------------
// SpriteShader.fx - Fixed version for sprite rendering
//--------------------------------------------------------------------------------------

// Матрица трансформации (World-View-Projection)
float4x4 WVP : register(c0);

// Текстура спрайта
texture g_texture;

// Сэмплер
sampler SpriteSampler = sampler_state {
    Texture = <g_texture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

// Структура входных данных
struct VS_INPUT {
    float3 Pos   : POSITION;
    float4 Color : COLOR0;
    float2 Tex   : TEXCOORD0;
    float2 Padding : TEXCOORD1;
};

struct VS_OUTPUT {
    float4 Pos   : POSITION;
    float4 Color : COLOR0;
    float2 Tex   : TEXCOORD0;
};

// Вертексный шейдер
VS_OUTPUT RenderSceneVS(VS_INPUT In) {
    VS_OUTPUT Out;
    // FIXED: Multiply float4 by matrix (convert float3 to float4)
    Out.Pos = mul(float4(In.Pos, 1.0f), WVP);
    Out.Tex = In.Tex;
    Out.Color = In.Color;
    return Out;
}

// Пиксельный шейдер
float4 RenderScenePS(VS_OUTPUT In) : COLOR0 {
    float4 texColor = tex2D(SpriteSampler, In.Tex);
    return texColor * In.Color;
}

// Техника для спрайтов (SpriteBatchTech)
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
