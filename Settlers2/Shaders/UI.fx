// UI.fx
float4x4 gScreenProj : register(c4); // Ортогональная матрица (экран) - сдвинули с c0 на c4
texture g_texture; // Явное объявление текстуры

sampler2D texSampler : register(s0) {
    Texture = <g_texture>;
    MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = LINEAR;
    AddressU = CLAMP; AddressV = CLAMP;
};

struct VS_INPUT {
    float3 pos   : POSITION;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 pos   : POSITION;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

VS_OUTPUT VS_Main(VS_INPUT input) {
    VS_OUTPUT output;
    // Прямое наложение на экран
    output.pos = mul(float4(input.pos, 1.0f), gScreenProj);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

float4 PS_Main(VS_OUTPUT input) : COLOR0 {
    float4 texColor = tex2D(texSampler, input.uv);
    return texColor * input.color;
}

technique UITech {
    pass P0 {
        ZEnable = FALSE; // UI не должен проверять глубину мира
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        VertexShader = compile vs_3_0 VS_Main();
        PixelShader  = compile ps_3_0 PS_Main();
    }
}