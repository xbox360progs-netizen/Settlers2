//--------------------------------------------------------------------------------------
// FontShader.fx - Для отрисовки текста и цифр ресурсов
//--------------------------------------------------------------------------------------

float4x4 WVP : register(c0);
texture SpriteTexture; // Текстура шрифта (атлас глифов)

sampler FontSampler = sampler_state {
    Texture = <SpriteTexture>;
    MinFilter = Linear;
    MagFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VS_INPUT {
    float4 Pos   : POSITION;
    float2 Tex   : TEXCOORD0;
    float4 Color : COLOR0;
};

struct VS_OUTPUT {
    float4 Pos   : POSITION;
    float2 Tex   : TEXCOORD0;
    float4 Color : COLOR0;
};

VS_OUTPUT RenderVS(VS_INPUT In) {
    VS_OUTPUT Out;
    Out.Pos = mul(In.Pos, WVP);
    Out.Tex = In.Tex;
    Out.Color = In.Color;
    return Out;
}

float4 RenderPS(VS_OUTPUT In) : COLOR0 {
    // ВАЖНО: У шрифта берем альфа-канал и умножаем на цвет текста
    float4 texColor = tex2D(FontSampler, In.Tex);
    return float4(In.Color.rgb, texColor.a * In.Color.a);
}

technique SpriteBatchTech { // Имя техники должно совпадать с тем, что ищет ShaderManager
    pass P0 {
        VertexShader = compile vs_3_0 RenderVS();
        PixelShader  = compile ps_3_0 RenderPS();
        
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        ZEnable = FALSE;
    }
}