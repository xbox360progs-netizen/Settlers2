float4x4 matOrtho;
texture2D g_texture;

sampler2D textureSampler = sampler_state {
    Texture = <g_texture>;
    MinFilter = LINEAR; MagFilter = LINEAR;
    AddressU = CLAMP; AddressV = CLAMP;
};

struct VS_IN {
    float3 Pos   : POSITION;
    float4 Color : COLOR0;
    float2 Tex   : TEXCOORD0;
};

struct VS_OUT {
    float4 Pos   : POSITION;
    float4 Color : COLOR0;
    float2 Tex   : TEXCOORD0;
};

VS_OUT SpriteVS(VS_IN In) {
    VS_OUT Out;
    Out.Pos = mul(float4(In.Pos, 1.0f), matOrtho);
    Out.Color = In.Color;
    Out.Tex = In.Tex;
    return Out;
}

float4 SpritePS(VS_OUT In) : COLOR {
    // DEBUG: Output only vertex color to exclude texture issues
    return In.Color;
    // return tex2D(textureSampler, In.Tex) * In.Color;
}

technique SpriteBatchTech {
    pass P0 {
        VertexShader = compile vs_3_0 SpriteVS();
        PixelShader  = compile ps_3_0 SpritePS();
    }
}