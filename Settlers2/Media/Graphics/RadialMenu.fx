float4x4 WorldViewProjection;
float4 MenuParams : register(c4);
float4 CenterParams : register(c5);

float4 InnerColor;
float4 OuterColor;
float4 HighlightColor;
float4 LineColor;
float4 CenterInnerColor;
float4 CenterOuterColor;

struct VS_INPUT {
    float4 Pos : POSITION;
    float2 UV  : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 Pos : POSITION;
    float2 UV  : TEXCOORD0;
    float2 CenterUV : TEXCOORD1;
};

VS_OUTPUT MainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Pos = mul(input.Pos, WorldViewProjection);
    output.UV = input.UV;
    output.CenterUV = input.UV * 2.0 - 1.0;
    return output;
}

float4 MainPS(VS_OUTPUT input) : COLOR0
{
    float2 pos = input.CenterUV;
    pos.y = -pos.y;

    float dist = length(pos);
    float PI = 3.14159265;

    if (dist <= CenterParams.x) {
        float centerGradient = saturate(dist / max(CenterParams.x, 0.0001));
        float4 centerColor = lerp(CenterInnerColor, CenterOuterColor, centerGradient);
        float centerEdge = 1.0 - smoothstep(CenterParams.x - CenterParams.y, CenterParams.x, dist);
        centerColor.a *= centerEdge;
        return centerColor;
    }

    if (dist < MenuParams.z || dist > MenuParams.w) {
        return float4(0, 0, 0, 0);
    }

    float gradientFactor = saturate((dist - MenuParams.z) / max(MenuParams.w - MenuParams.z, 0.0001));
    float4 sectorColor = lerp(InnerColor, OuterColor, gradientFactor);

    float angle = atan2(pos.y, pos.x);
    float normAngle = frac((angle + PI * 0.5 + PI) / (2.0 * PI));
    float currentSector = floor(normAngle * MenuParams.x);
    float sectorMask = 1.0 - step(0.5, abs(currentSector - MenuParams.y));
    sectorColor = lerp(sectorColor, HighlightColor, sectorMask * 0.85);

    float sectorPhase = frac(normAngle * MenuParams.x);
    float sectorLine = 1.0 - smoothstep(0.0, CenterParams.z, min(sectorPhase, 1.0 - sectorPhase));
    float innerLine = 1.0 - smoothstep(0.0, CenterParams.z * 1.5, abs(dist - MenuParams.z));
    float outerLine = 1.0 - smoothstep(0.0, CenterParams.z * 1.5, abs(dist - MenuParams.w));
    float centerRing = 1.0 - smoothstep(0.0, CenterParams.z * 1.5, abs(dist - CenterParams.x));

    float ringGlow = saturate(1.0 - smoothstep(MenuParams.w, MenuParams.w + CenterParams.w, dist));
    sectorColor.rgb += HighlightColor.rgb * ringGlow * 0.08;

    float lineMask = saturate(max(sectorLine, max(innerLine, max(outerLine, centerRing))));
    float4 finalColor = lerp(sectorColor, LineColor, lineMask * LineColor.a);
    return finalColor;
}

technique RadialMenu {
    pass P0 {
        VertexShader = compile vs_3_0 MainVS();
        PixelShader  = compile ps_3_0 MainPS();
        AlphaBlendEnable = TRUE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
    }
}
