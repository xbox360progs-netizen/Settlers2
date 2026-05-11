// Xbox 360 Constant Buffer Sprite Shader
// Unified register layout for global constants and instance data
// Xbox 360 compatible: uses vs_2_0/ps_2_0 shader profiles

// === GLOBAL CONSTANTS (c0-c10) ===
float4x4 matOrtho : register(c0);
float4 g_GlobalTime : register(c4); // Time, can be used for animations
float4 g_GlobalColor : register(c5); // Global tint color

sampler2D g_texture : register(s0);

// === INSTANCE DATA (c20-c200) ===
// Current sprite data (2 registers: [PosSize] and [UVRect])
// Moved to c20 to avoid conflicts with global constants
float4 g_PosSize : register(c20);  // x, y, width, height
float4 g_UVRect  : register(c21);  // u0, v0, u1, v1
float4 g_SpriteColor : register(c22); // Per-sprite color tint

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

    // Color: combine global color with sprite-specific color
    output.color = g_GlobalColor * g_SpriteColor;

    return output;
}

float4 SpriteConstantInstPS(VS_OUTPUT input) : COLOR0 {
    float4 texColor = tex2D(g_texture, input.uv);
    return texColor * input.color;
}

technique SpriteBatchTech {
    pass P0 {
        VertexShader = compile vs_2_0 SpriteConstantInstVS();
        PixelShader  = compile ps_2_0 SpriteConstantInstPS();

        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        ZEnable = FALSE;
        CullMode = NONE;
    }
}
