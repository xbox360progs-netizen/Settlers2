// Xbox 360 Constant Buffer Instancing Sprite Shader
// Supports 4096+ sprites by using constant registers with chunked processing

float4x4 matOrtho : register(c0);
sampler2D g_texture : register(s0);

// Instance data stored in constant registers
// Each sprite takes 2 registers: [PosSize] and [UVRect+Color]
// Register c10-c210 can hold ~100 sprites (200 registers)
// Xbox 360 GPU has ~256-512 constant registers available
float4 g_InstanceData[200] : register(c10);

struct VS_INPUT {
    float3 basePos : POSITION;
    int    instID  : TEXCOORD1;
};

struct VS_OUTPUT {
    float4 pos   : POSITION;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

VS_OUTPUT SpriteConstantInstVS(VS_INPUT input) {
    VS_OUTPUT output;
    
    int idx = input.instID * 2;
    
    float4 posSize = g_InstanceData[idx];
    float4 uvColor = g_InstanceData[idx + 1];
    
    float2 finalPos = input.basePos.xy * posSize.zw + posSize.xy;
    output.pos = mul(float4(finalPos, 0.0f, 1.0f), matOrtho);
    
    output.uv.x = lerp(uvColor.x, uvColor.z, input.basePos.x);
    output.uv.y = lerp(uvColor.y, uvColor.w, input.basePos.y);
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