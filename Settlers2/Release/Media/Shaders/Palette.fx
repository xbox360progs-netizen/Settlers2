// Palette.fx
float4x4 gViewProj : register(c0);
sampler2D texSampler : register(s0);    // Основная текстура (индексированная)
sampler1D paletteSampler : register(s1); // Текстура-палитра (цвета игрока)

struct VS_OUTPUT {
    float4 pos   : POSITION;
    float2 uv    : TEXCOORD0;
};

float4 RenderPalettePS(VS_OUTPUT input) : COLOR0 {
    float4 index = tex2D(texSampler, input.uv);
    // Берем цвет из палитры на основе яркости или красного канала текстуры
    return tex1D(paletteSampler, index.r);
}

technique PaletteTech {
    pass P0 {
        VertexShader = compile vs_3_0 RenderSceneVS(); // Используем VS из World.fx
        PixelShader  = compile ps_3_0 RenderPalettePS();
    }
}