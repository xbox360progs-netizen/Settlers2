// Xbox 360 Constant Buffer Sprite Shader
// 16 sprites per batch using array in registers c10-c41 (32 registers total)

float4x4 matOrtho : register(c0);
sampler2D g_texture : register(s0);

// Array of 16 sprites, each using 2 registers: [PosSize] and [UVRect]
// Total: 32 registers (c10-c41)
float4 g_SpriteData[32] : register(c10);

struct VS_INPUT {
    float3 basePos : POSITION;     // Standard 0 to 1 quad
    float  instanceID : TEXCOORD1; // Instance ID (0-15) for selecting sprite data
};

struct VS_OUTPUT {
    float4 pos   : POSITION;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

VS_OUTPUT SpriteConstantInstVS(VS_INPUT input) {
    VS_OUTPUT output;

    // Calculate index into sprite data array (2 registers per sprite)
    int idx = input.instanceID * 2;

    // Get position/size for this instance
    float4 posSize = g_SpriteData[idx];     // x, y, width, height
    float4 uvRect = g_SpriteData[idx + 1];  // u0, v0, u1, v1

    // Position: scale and translate the base quad
    float2 finalPos = input.basePos.xy * posSize.zw + posSize.xy;
    output.pos = mul(float4(finalPos, 0.0f, 1.0f), matOrtho);

    // UV mapping: lerp based on basePos
    output.uv.x = lerp(uvRect.x, uvRect.z, input.basePos.x);
    output.uv.y = lerp(uvRect.y, uvRect.w, input.basePos.y);

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
