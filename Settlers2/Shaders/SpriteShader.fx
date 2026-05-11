//--------------------------------------------------------------------------------------
// SpriteShader.fx - Финальная версия для твоего рендера
//--------------------------------------------------------------------------------------

// Матрица трансформации (World-View-Projection)
float4x4 WVP : register(c0);

// Текстура спрайта
texture SpriteTexture;

// Сэмплер
sampler SpriteSampler = sampler_state {
    Texture = <SpriteTexture>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

// Структура входных данных
struct VS_INPUT {
    float3 Pos   : POSITION;  // Changed from float4 to float3 to match Renderer vertex declaration
    float2 Tex   : TEXCOORD0;
    float4 Color : COLOR0;
};

struct VS_OUTPUT {
    float4 Pos   : POSITION;
    float2 Tex   : TEXCOORD0;
    float4 Color : COLOR0;
};

// Вертексный шейдер
VS_OUTPUT RenderSceneVS(VS_INPUT In) {
    VS_OUTPUT Out;
    Out.Pos = mul(In.Pos, WVP);
    Out.Tex = In.Tex;
    Out.Color = In.Color;
    return Out;
}

// Пиксельный шейдер
float4 RenderScenePS(VS_OUTPUT In) : COLOR0 {
    float4 texColor = tex2D(SpriteSampler, In.Tex);
    return texColor * In.Color;
}

// Техника, которую ищет твой ShaderManager (SpriteBatchTech)
technique SpriteBatchTech {
    pass P0 {
        VertexShader = compile vs_3_0 RenderSceneVS();
        PixelShader  = compile ps_3_0 RenderScenePS();

        // Render states managed by Renderer, not set in shader
        // AlphaBlendEnable = TRUE;
        // DestBlend = INVSRCALPHA;
        // SrcBlend = SRCALPHA;
        // ZEnable = FALSE;
    }
}