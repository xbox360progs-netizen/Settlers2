// Grayscale Shader - converts texture to grayscale
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
    // Convert to grayscale
    float gray = dot(texColor.rgb, float3(0.299, 0.587, 0.114));
    return float4(gray, gray, gray, texColor.a) * In.Color;
}

technique GrayscaleTech {
    pass P0 {
        VertexShader = compile vs_3_0 SpriteVS();
        PixelShader  = compile ps_3_0 SpritePS();
    }
}
