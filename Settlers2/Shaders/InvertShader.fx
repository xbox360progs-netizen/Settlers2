// Invert Color Shader - inverts texture colors
float4x4 matOrtho;
texture2D g_texture;

sampler2D textureSampler = sampler_state {
    Texture = <g_texture>;
    MinFilter = LINEAR; MagFilter = LINEAR;
    AddressU = CLAMP; AddressV = CLAMP;
};

struct VS_OUT {
    float4 Pos   : POSITION;
    float4 Color : COLOR0;
    float2 Tex   : TEXCOORD0;
};

VS_OUT SpriteVS(uint vId : INDEX) {
    VS_OUT Out;
    float4 rawPos, rawCol, rawTex;
    asm {
        vfetch rawPos, vId, position0
        vfetch rawCol, vId, color0
        vfetch rawTex, vId, texcoord0
    };
    Out.Pos = mul(float4(rawPos.x, rawPos.y, 0.0f, 1.0f), matOrtho);
    Out.Color = rawCol;
    Out.Tex = rawTex.xy;
    return Out;
}

float4 SpritePS(VS_OUT In) : COLOR {
    float4 texColor = tex2D(textureSampler, In.Tex);
    // Invert colors
    return float4(1.0 - texColor.r, 1.0 - texColor.g, 1.0 - texColor.b, texColor.a) * In.Color;
}

technique InvertTech {
    pass P0 {
        VertexShader = compile vs_3_0 SpriteVS();
        PixelShader  = compile ps_3_0 SpritePS();
    }
}
