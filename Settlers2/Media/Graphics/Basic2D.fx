//--------------------------------------------------------------------------------------
// Basic 2D Shader
//--------------------------------------------------------------------------------------

float4x4 matWVP : register(c0);

struct VS_IN {
    float3 pos : POSITION;
    float4 col : COLOR0;  // Будет передаваться как DWORD, но шейдер преобразует
    float2 uv : TEXCOORD0;
};

struct VS_OUT {
    float4 pos : POSITION;
    float4 col : COLOR;
    float2 uv : TEXCOORD0;
};

VS_OUT VS_Basic2D(VS_IN input) {
    VS_OUT output;
    output.pos = mul(float4(input.pos, 1.0f), matWVP);
    // Преобразуем DWORD цвет в float4
    output.col = input.col;  // DirectX автоматически преобразует DWORD->float4
    output.uv = input.uv;
    return output;
}

sampler2D tex : register(s0);

float4 PS_Basic2D(VS_OUT input) : COLOR {
    float4 color = tex2D(tex, input.uv) * input.col;
    clip(color.a - 0.05f);  // Добавим alpha test для безопасности
    return color;
}

technique Basic2D {
    pass P0 {
        VertexShader = compile vs_2_0 VS_Basic2D();
        PixelShader = compile ps_2_0 PS_Basic2D();
    }
}
