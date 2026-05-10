// Xbox 360 Constant Buffer Sprite Shader
// Each sprite drawn individually with data in c10-c11 registers

float4x4 matOrtho : register(c0);
sampler2D g_texture : register(s0);

// Current sprite data (2 registers: [PosSize] and [UVRect])
float4 g_PosSize : register(c10);  // x, y, width, height
float4 g_UVRect  : register(c11);  // u0, v0, u1, v1

struct VS_INPUT {
    float3 basePos : POSITION; // Standard 0 to 1 quad
};

struct VS_OUTPUT {
    float4 pos   : POSITION;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

VS_OUTPUT SpriteConstantInstVS(VS_INPUT input) {
    VS_OUTPUT output;

    // Position: scale and translate the base quad
    float2 finalPos = input.basePos.xy * g_PosSize.zw + g_PosSize.xy;
    output.pos = mul(float4(finalPos, 0.0f, 1.0f), matOrtho);

    // UV mapping: lerp based on basePos
    output.uv.x = lerp(g_UVRect.x, g_UVRect.z, input.basePos.x);
    output.uv.y = lerp(g_UVRect.y, g_UVRect.w, input.basePos.y);

    // Color - white for now
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);

    return output;
}

float4 SpriteConstantInstPS(VS_OUTPUT input) : COLOR0 {
    float4 texColor = tex2D(g_texture, input.uv);
    return texColor * input.color;
}

technique SpriteConstantInstancedTech {
    pass P0 {
        VertexShader = compile vs_3_0 SpriteConstantInstVS();
        PixelShader  = compile ps_3_0 SpriteConstantInstPS();

        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        ZEnable = FALSE;
        CullMode = NONE;
    }
}
