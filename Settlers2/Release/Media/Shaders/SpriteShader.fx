// Xbox 360 Sprite Shader - Simple version

// matOrtho at c0
float4x4 matOrtho : register(c0);

// Texture
sampler2D g_texture : register(s0);

// Input: already in screen coordinates
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

VS_OUTPUT SpriteVS(VS_INPUT input) {
    VS_OUTPUT output;

    // Half pixel offset for proper alignment
    float2 screenPos = input.pos.xy - 0.5f;

    output.pos = mul(float4(screenPos, 0.0f, 1.0f), matOrtho);
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

float4 SpritePS(VS_OUTPUT input) : COLOR0 {
    float4 texColor = tex2D(g_texture, input.uv);
    return texColor * input.color;
}

technique SpriteBatchTech {
    pass P0 {
        VertexShader = compile vs_3_0 SpriteVS();
        PixelShader  = compile ps_3_0 SpritePS();
        AlphaBlendEnable = TRUE;
        SrcBlend         = SRCALPHA;
        DestBlend        = INVSRCALPHA;
        ZEnable          = FALSE;
        CullMode         = NONE;
    }
}