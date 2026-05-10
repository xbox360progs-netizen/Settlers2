// Xbox 360 Optimized Sprite Shader
// Standard rendering path only (instancing removed - uint : INDEX crashes XDK compiler)

// Constants (Registers)
// c0-c3 typically occupied by projection matrix
float4x4 matOrtho : register(c0);

// Texture
sampler2D g_texture : register(s0);

// Data structures for standard rendering
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

// --- Standard Vertex Shader ---
VS_OUTPUT SpriteVS(VS_INPUT input) {
    VS_OUTPUT output;

    // Half-pixel offset fix for Direct3D 9 (critical for Xbox 360)
    // D3D9 pixel coordinates are offset by 0.5 from texture coordinates
    float4 pos = float4(input.pos, 1.0f);
    pos.xy -= 0.5f;
    
    // Transform coordinates to screen space
    output.pos = mul(pos, matOrtho);

    // Pass color and UV
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

// --- Pixel Shader ---
float4 SpritePS(VS_OUTPUT input) : COLOR0 {
    float4 texColor = tex2D(g_texture, input.uv);
    return texColor * input.color;
}

// --- Technique with Single Pass ---
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
