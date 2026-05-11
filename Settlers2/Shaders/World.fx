// World.fx
float4x4 gViewProj : register(c0); // Матрица камеры
texture g_Texture; // Текстура

sampler2D texSampler : register(s0) {
    Texture = <g_Texture>;
    MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = LINEAR;
    AddressU = CLAMP; AddressV = CLAMP;
};

struct VS_INPUT {
    float3 pos   : POSITION;  // x, y, z
    float4 color : COLOR0;    // DWORD color
    float2 uv    : TEXCOORD0; // u, v
};

struct VS_OUTPUT {
    float4 pos   : POSITION;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

VS_OUTPUT VS_Main(VS_INPUT input) {
    VS_OUTPUT output;
    // Трансформируем мировые координаты в экранные через камеру
    output.pos = mul(float4(input.pos, 1.0f), gViewProj);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

float4 PS_Main(VS_OUTPUT input) : COLOR0 {
    return tex2D(texSampler, input.uv) * input.color;
}

technique WorldTech {
    pass P0 {
        VertexShader = compile vs_3_0 VS_Main();
        PixelShader  = compile ps_3_0 PS_Main();
        ZEnable = TRUE;
        ZWriteEnable = TRUE;
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
    }
}