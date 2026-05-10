// Xbox 360 Hardware Instancing Sprite Shader
// Optimized for maximum performance with two-stream vertex input

float4x4 matOrtho : register(c0);
sampler2D g_texture : register(s0);

struct VS_INPUT {
    float3 basePos : POSITION;      // From Stream 0 (0 to 1)
    float4 instPos : TEXCOORD1;    // From Stream 1 (x, y, w, h)
    float4 instUV  : TEXCOORD2;    // From Stream 1 (u0, v0, u1, v1)
    float4 color   : COLOR0;       // From Stream 1
};

struct VS_OUTPUT {
    float4 pos   : POSITION;
    float4 color : COLOR0;
    float2 uv    : TEXCOORD0;
};

VS_OUTPUT SpriteInstVS(VS_INPUT input) {
    VS_OUTPUT output;

    // Scale and translate the base quad using instance data
    float2 finalPos = input.basePos.xy * input.instPos.zw + input.instPos.xy;
    output.pos = mul(float4(finalPos - 0.5f, 0.0f, 1.0f), matOrtho);

    // Map UVs: basePos.x is 0 or 1, so it picks u0 or u1
    output.uv.x = lerp(input.instUV.x, input.instUV.z, input.basePos.x);
    output.uv.y = lerp(input.instUV.y, input.instUV.w, input.basePos.y);
    
    output.color = input.color;
    return output;
}

float4 SpriteInstPS(VS_OUTPUT input) : COLOR0 {
    float4 texColor = tex2D(g_texture, input.uv);
    return texColor * input.color;
}

technique SpriteInstancedTech {
    pass P0 {
        VertexShader = compile vs_3_0 SpriteInstVS();
        PixelShader  = compile ps_3_0 SpriteInstPS();
        
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        ZEnable = FALSE;
        CullMode = NONE;
    }
}
